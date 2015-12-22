#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fsm.h"
#include "keyword.h"
#include "scanner.h"
#include "parser.h"

static bool gDebug = false;
parser parse_status;

void report(char *str, report_t type)
{
	switch (type)
	{
		case INFO:
			printf("[INFO] %s\n",str);
			break;
		case WARN:
			printf("[WARN] %s\n",str);
			break;
		case ERROR:
			printf("!!! [ERROR] %s\n",str);
			parse_status.error =true;
			parse_status.errorCount++;		
			break;
		default:
			break;
	}
}

void doScan(scanner *scn, struct fsm_object *obj)
{
	char msg[256];
	fsm_init(obj);

	obj->fsm_arg_value = (void **)scn;

    	fsm_default(obj,start);
   	fsm_add(obj,"id",id);
	fsm_add(obj,"number",number);
	fsm_add(obj,"float",floatl); 
	fsm_add(obj,"alpha",alpha);

	fsm_main(obj);
	if (scn->currentToken.tid == T_EOF)
	{
		report("Scan complete",INFO);
		sprintf(msg,"Hash table filled with %d keys",g_hash_table_size(scn->table));
		report(msg,INFO);
	}
	else
	{
		if (gDebug)
		{
			if (scn->currentToken.tid < 25)
			{
				if (scn->currentToken.tid == T_INTCONST)
				{
					sprintf(msg,"[DEBUG] Token enum = %s with value = %d on line %d",names[scn->currentToken.tid],scn->currentToken.value.intVal,scn->curr_line);
					report(msg,INFO);
					return;
				}
				
				if (scn->currentToken.tid == T_FLOATCONST)
				{
					sprintf(msg,"[DEBUG] Token enum = %s with value = %f on line %d",names[scn->currentToken.tid],scn->currentToken.value.doubleVal,scn->curr_line);
					report(msg,INFO);
					return;
				}
					
				if (scn->currentToken.tid == T_STRINGCONST)
				{
					sprintf(msg,"[DEBUG] Token enum = %s with value %s on line %d",names[scn->currentToken.tid],scn->currentToken.value.string,scn->curr_line);
					report(msg,INFO);
					return;
				}
					
				else
				{
					sprintf(msg,"[DEBUG] Token enum %s on line %d",names[scn->currentToken.tid],scn->curr_line);
					report(msg,INFO);
					return;
				}
			}
			else
			{
				sprintf(msg,"[DEBUG] Token = %c on line %d",scn->currentToken.tid,scn->curr_line);
				report(msg,INFO);
			}
		}
	}	
	
}

token_t scan(scanner *scn, struct fsm_object *obj) 
{
	if (parse_status.pScan) {
		parse_status.pScan = false;
	} else {
		doScan(scn,obj);
	}

	return scn->currentToken.tid;
}

bool tokenIs(scanner *scn, struct fsm_object *obj, token_t tid) {
	if ( scan(scn,obj) == tid ) {
		return true;
	}
	parse_status.pScan = true;
	return false;
}


bool argument_list(scanner *scn,struct fsm_object *obj, GList **args, GList **regs)
{
	char msg[256];
	data_t dt;
	bool global;
	int size,addr,i;
	symbol *tSymbol;
	symbol_t *tSymbol_t;
	parse_status.array_id = NULL;
	
	if (expression(scn,obj,&dt)) {
		if (parse_status.array_id != NULL)
		{
			findSymbol(parse_status.array_id,&global,&tSymbol);
			*args = g_list_append(*args,(gpointer)&(tSymbol->st));
			size = tSymbol->st.size;
			addr = parse_status.reg - 1;
			*regs = g_list_append(*regs,GINT_TO_POINTER(parse_status.reg));
			for (i = 1; i < size; i++) {
				parse_status.reg++;
				fprintf(parse_status.file,"\tR[%d] = R[%d] + 1;\n",addr,addr);
				fprintf(parse_status.file,"\tR[%d] = MM[R[%d]];\n",parse_status.reg,addr);
				*regs = g_list_append(*regs,GINT_TO_POINTER(parse_status.reg));
			}
		} else {
			tSymbol_t = (symbol_t *)calloc(1,sizeof(symbol_t));
			tSymbol_t->st = VARIABLE;
			tSymbol_t->dt = dt;
			tSymbol_t->size = 1;
			*args = g_list_append(*args,(gpointer)tSymbol_t);
			*regs = g_list_append(*regs,GINT_TO_POINTER(parse_status.reg));
		}

		if ( tokenIs(scn,obj,T_COMMA) && !argument_list(scn,obj,args,regs)) {
			sprintf(msg,"Syntax error in argument list near line %d",scn->curr_line);
			report(msg,ERROR);
			return false;
		}
		return true;
	}
	return false;	
}

bool checkParams(GList *params, GList *args)
{
	GList *it;
	GList *it2;
	symbol_t *st;
	symbol_t *st2;
	it = params;
	it2 = args;

	if (params == args ) {
		return true;
	}

	if (params == NULL || args == NULL)
	{
		return false;
	}

	while (it->next != NULL && it2->next != NULL) {
		st = (symbol_t *)it->data;
		st2 = (symbol_t *)it2->data;
		if (st->st != st2->st) return false;
		if (st->dt != st2->dt) return false;
		if (st->size != st2->size) return false;
		it = it->next;
		it2 = it2->next;
	}
	return true;
}

bool function_call(scanner *scn,struct fsm_object *obj, GList **regs)
{
	char msg[256];
	GList *args = NULL;
	GList *params;
	bool global;
	symbol *tSymbol;
	char *id = (char *)calloc(strlen(scn->currentToken.value.string),sizeof(char));
	strcpy(id,scn->currentToken.value.string);
	//scan(scn,obj);

	if (tokenIs(scn,obj,T_LPAREN)) {
		argument_list(scn,obj,&args,regs);
		//scan(scn,obj);
		if (tokenIs(scn,obj,T_RPAREN)) {
			if ( findSymbol(id,&global,&tSymbol)) {
				 params = tSymbol->st.params;
				if (!checkParams(params,args)) {
					sprintf(msg,"Arguments types do no match function signature near line %d",scn->curr_line);
					report(msg,ERROR);
					return false;
				}
				return true;
			}
		}
	}
	return false;
}
void getMemLoc(int offset, bool global)
{
	parse_status.reg++;
	fprintf(parse_status.file,"\tR[%d] = %d;\n",parse_status.reg,offset);
	
	if (!global) {
		fprintf(parse_status.file,"\tR[%d] = R[%d] + R[FP];\n",parse_status.reg,parse_status.reg);
	}
}

void printTable(gpointer key, gpointer value, gpointer data)
{
	printf("[DEBUG] Table level: %d key: %s addr: %d \n",parse_status.level,(char *)key,(int )((symbol *)value)->addr);
}

bool findSymbol(char *id, bool *global, symbol **tSymbol)
{
	*global = true;
	GHashTable *currTable; 
	GList *it = g_list_last(parse_status.localSymbolTables);

	//while (it != NULL) {
	
		currTable = (GHashTable *)(it->data);

		if (gDebug) {
			printf("---Looking for %s---\n",id); 
			g_hash_table_foreach(currTable,printTable,NULL);
		}
		if ( g_hash_table_lookup_extended(currTable,id,NULL,NULL) )
		{
			*tSymbol = (symbol *)g_hash_table_lookup(currTable,id);
			*global = false;
			return true;
		}
	//	it = it->prev;
	//}

	currTable = parse_status.gSymbolTable;

	if ( g_hash_table_lookup_extended(currTable,id,NULL,NULL))
	{
		*tSymbol = (symbol *)g_hash_table_lookup(currTable,id);
		return true; 
	}

	return false;
}

bool name(scanner *scn, struct fsm_object *obj, bool *hasIndex)
{
	data_t dt;
	char msg[256];
	//scan(scn,obj);
	if (tokenIs(scn,obj,T_LBOX) && expression(scn,obj,&dt)) {
		if (dt != BOOLEAN && dt != INTEGER) {
			sprintf(msg,"Array indexes must be integers near line %d",scn->curr_line);
			report(msg,ERROR);
			return false;
		}
		*hasIndex = true;

		//scan(scn,obj);
		if (tokenIs(scn,obj,T_RBOX)) {
			return true;
		}
	}
	return true;	
}

void callerBegin(GList *regs, char *id)
{
	GList *it = g_list_last(regs);


	int addr;
	while (it != regs && it != NULL) {
		fprintf(parse_status.file,"\tR[SP] = R[SP] + 1;\n");
		fprintf(parse_status.file,"\tMM[R[SP]] = R[%d];\n",GPOINTER_TO_INT(it->data));
		it = it->prev;
	}
	if (it != NULL)
	{ 
		fprintf(parse_status.file,"\tR[SP] = R[SP] + 1;\n");
		fprintf(parse_status.file,"\tMM[R[SP]] = R[%d];\n",GPOINTER_TO_INT(it->data));
	}
	addr = parse_status.reg++;
	
	fprintf(parse_status.file,"\tR[%d] = (int)&&%s%d;\n",parse_status.reg,id,parse_status.fCounter);
	fprintf(parse_status.file,"\tR[SP] = R[SP] + 1;\n");
	fprintf(parse_status.file,"\tMM[R[SP]] = R[%d];\n",parse_status.reg);
	fprintf(parse_status.file,"\tgoto *(void *)R[%d];\n",addr);
}

void callerEnd(GList *regs, char *id)
{
	parse_status.reg = 0;
	fprintf(parse_status.file,"%s%d:\n",id,parse_status.fCounter);
	fprintf(parse_status.file,"\tR[%d] = MM[R[SP]];\n", parse_status.reg);
	fprintf(parse_status.file,"\tR[SP] = R[SP] - 1;\n");
	parse_status.fCounter++;

	if (regs == NULL)
	{	
		//
	}

	if (g_list_length(regs) > 0) {
		fprintf(parse_status.file,"\tR[SP] = R[SP] - %d;\n",g_list_length(regs));
	}
}

bool factor(scanner *scn, struct fsm_object *obj, data_t *dt, bool negate)
{
	char msg[256];
	char *id;
	bool index = false;
	GList *regs = NULL;
	symbol *tSymbol;
	int idx;
	bool global;
	int addr;
	bool functionCall;
	if (tokenIs(scn,obj,T_LPAREN)) {
		if ( expression(scn,obj,dt) ) {
			//scan(scn,obj);
			if (tokenIs(scn,obj,T_RPAREN)) {
				return true;
			}
		}
	}
		negate = tokenIs(scn,obj,T_MINUS);
		if (tokenIs(scn,obj,T_IDENTIFIER)) {
			id = (char *)calloc(strlen(scn->currentToken.value.string),sizeof(char));
			strcpy(id,scn->currentToken.value.string);
			functionCall = function_call(scn,obj,&regs);
			if (functionCall || name(scn,obj,&index)) {
				idx = parse_status.reg;
				global = false;
				if (!findSymbol(id,&global,&tSymbol)) {
					sprintf(msg,"No such symbol '%s' found near line %d",id,scn->curr_line);
					report(msg,ERROR);
					return false;
				}

				addr = tSymbol->addr;
				*dt = tSymbol->st.dt;
				getMemLoc(addr,global);

				if (index) {
					fprintf(parse_status.file,"\tR[%d] = R[%d] %c R[%d];\n",parse_status.reg,parse_status.reg,
						(addr < 0 ? '-' : '+'),idx);
				}

				parse_status.reg++;
				fprintf(parse_status.file,"\tR[%d] = MM[R[%d]];\n",parse_status.reg,parse_status.reg - 1);
				if (negate) {
					parse_status.reg++;
					fprintf(parse_status.file,"\tR[%d] = -1*R[%d];\n",parse_status.reg,parse_status.reg - 1);
				}
				if (functionCall) {
					callerBegin(regs,id);
					callerEnd(regs,id);
				}

				if (tSymbol->st.st == ARRAY && !index) {
					parse_status.array_id = id;
				}
			}
		}
	
//		case T_INTCONST:
		else if (tokenIs(scn,obj,T_INTCONST)) {
			fprintf(parse_status.file,"\tR[%d] = %c%d;\n",++(parse_status.reg),negate ? '-' : ' ',scn->currentToken.value.intVal);
			*dt = INTEGER;
		}
//		case T_FLOATCONST:
		else if (tokenIs(scn,obj,T_FLOATCONST) ){
			fprintf(parse_status.file,"\ttmpFloat = %c%f;\n",negate ? '-' : ' ',scn->currentToken.value.doubleVal);
			fprintf(parse_status.file,"\tmemcpy(&R[%d], &tmpFloat, sizeof(float));\n",++(parse_status.reg));
			*dt = FLOAT;
		}
//		case T_STRINGCONST:
		else if (tokenIs(scn,obj,T_STRINGCONST)) {
			if (!negate) {
				*dt = STRING;
				fprintf(parse_status.file,"\tR[%d] = (int)\"%s\";\n",++(parse_status.reg),scn->currentToken.value.string);
			} else {
				sprintf(msg,"Negative string found near line %d",scn->curr_line);
				report(msg,ERROR);
				return false;
			}
		}
//		case T_BOOLCONST:
		else if(tokenIs(scn,obj,T_BOOLCONST)) {
			if (!negate) {
				*dt = BOOLEAN;
				fprintf(parse_status.file,"\tR[%d] = %d;\n", ++(parse_status.reg),scn->currentToken.value.boolVal);
			} else {
				sprintf(msg,"Negative boolean found near line %d",scn->curr_line);
				report(msg,ERROR);
				return false;
			}
		}
//		default:
		else {
			//sprintf(msg,"Unexpected token found in expression found near line %d",scn->curr_line);
			//report(msg,ERROR);
			return false;
		}
	return true;
}
bool typeCheck(data_t *dt1, data_t *dt2)
{
	if (*dt1 == *dt2) return true;

	if (*dt1 == INTEGER) {
		if (*dt2 == FLOAT) return true;
		if (*dt2 == BOOLEAN) return true;
		else return false;
	} else if (*dt1 == FLOAT) {
		if (*dt2 == INTEGER) return true;
		else return false;
	} else {
		return false;
	}
	
	return false;
}
			
char * dt2string(data_t *dt)
{
	switch (*dt)
	{
		case INTEGER:
			return "INTEGER";
		case FLOAT:
			return "FLOAT";
		case STRING:
			return "STRING";
		case BOOLEAN:
			return "BOOLEAN";
		default:
			return "UNKNOWN";
	}
}

void doOp(int op1, int op2, char op, bool fp1, bool fp2)
{
	parse_status.reg++;
	if (fp1 || fp2) {
		if (fp1) {
			fprintf(parse_status.file,"\tmemcpy(&tmpFloat, &R[%d], sizeof(float));\n",op1);
		} else {
			fprintf(parse_status.file,"\ttmpFloat = R[%d];\n",op1);
		}
		if (fp2) {
			fprintf(parse_status.file,"\tmemcpy(&tmpFloat2, &R[%d], sizeof(float));\n",op2);
		} else {
			fprintf(parse_status.file,"\ttmpFloat2 = R[%d];\n",op2);
		}

		fprintf(parse_status.file,"\ttmpFloat = tmpFloat %c tmpFloat2;\n",op);
		fprintf(parse_status.file,"\tmemcpy(&R[%d], &tmpFloat,sizeof(float));\n",parse_status.reg);
	}
	else {
		fprintf(parse_status.file,"\tR[%d] = R[%d] %c R[%d];\n",parse_status.reg,op1,op,op2);
	}
}

bool term_next(scanner *scn, struct fsm_object *obj, data_t *dt, char *op)
{
	data_t dt2;
	int op1, op2;
	char thisOp;
	bool finalTerm;
	char msg[256];
	token tToken;
	//scan(scn,obj);
	if (tokenIs(scn,obj,T_STAR) || tokenIs(scn,obj,T_DIVIDE)) {
		*op = scn->currentToken.value.string[0];
		if ( factor(scn,obj,dt,false) ) { 
			op1 = parse_status.reg;
			finalTerm = !term_next(scn,obj,&dt2,&thisOp);
			op2 = parse_status.reg;
			if (!finalTerm) {
				if (!typeCheck(dt,&dt2))
				{
					sprintf(msg,"%s not compatible with %s",dt2string(dt),dt2string(&dt2));
					report(msg,ERROR);
					return false;
				}
				doOp(op1,op2,thisOp,*dt == FLOAT,dt2 == FLOAT);
			}

			return true;
		}
	}
	return false;	
}

bool term(scanner *scn, struct fsm_object *obj, data_t *dt)
{
	data_t dt1, dt2;
	int op1, op2;
	char msg[256];
	char op;

	if ( factor(scn,obj, &dt1,false) ) {
		op1 = parse_status.reg;
		if ( term_next(scn,obj,&dt2,&op) ) {
			if (!typeCheck(&dt1,&dt2) || parse_status.array_id != NULL)
			{
				sprintf(msg,"%s not compatible with %s",dt2string(&dt1),dt2string(&dt2));
				report(msg,ERROR);
				return false;
			}

			op2 = parse_status.reg;
			doOp(op1,op2,op,dt1 == FLOAT, dt2 == FLOAT);
		}
		*dt = dt1;
		return true;		
	}
	return false;
}

bool checkRelType(data_t *dt1, data_t *dt2)
{
	if (*dt1 == FLOAT || *dt2 == FLOAT) return false;
	if (*dt1 == STRING || *dt2 == STRING) return false;

	return true;
}

void doRelOp(int op1, int op2, char *op)
{
	parse_status.reg++;
	fprintf(parse_status.file,"\tR[%d] = R[%d] %s R[%d];\n",parse_status.reg,op1,op,op2);
}



bool relation_next(scanner *scn, struct fsm_object *obj, data_t *dt, char *op)
{
	data_t dt2;
	bool valToken = false;
	int op1,op2;
	bool finalRel;
	char thisOp[3];
	char msg[256];
	
	if (tokenIs(scn,obj,T_EQUAL) ||
		tokenIs(scn,obj,T_NOTEQUAL) ||
		tokenIs(scn,obj,T_LTEQ) ||
		tokenIs(scn,obj,T_GTEQ) ||
		tokenIs(scn,obj,T_LESSTHAN) ||
		tokenIs(scn,obj,T_GREATTHAN) ) {
			valToken = true;
	}	
	
	if (valToken) {
		strncpy(op,scn->currentToken.value.string,3*sizeof(char));
		if ( term(scn,obj,&dt2) ) {
			op1 = parse_status.reg;
			finalRel = !relation_next(scn,obj,&dt2,thisOp);
			op2 = parse_status.reg;
			if (!finalRel) {
				if (!checkRelType(dt,&dt2)) {
					sprintf(msg,"%s is not compatible with %s near line %d",dt2string(dt),dt2string(&dt2),scn->curr_line);
					report(msg,ERROR);
					return false;
				}
				doRelOp(op1,op2,thisOp);
			}
			*dt = dt2;
			return true;
		}
	}
	return false;
}

	

bool relation(scanner *scn, struct fsm_object *obj, data_t *dt)
{
	data_t dt1, dt2;
	int op1,op2;
	char msg[256];
	char op[3];


	if ( term(scn,obj, &dt1) ) {
		op1 = parse_status.reg;
		if (relation_next(scn,obj,&dt2,op)) {
			if (!checkRelType(&dt1,&dt2) || parse_status.array_id != NULL) {
				sprintf(msg,"%s is not compatible with %s",dt2string(&dt1),dt2string(&dt2));
				report(msg,ERROR);
				return false;
			}
			op2 = parse_status.reg;
			doRelOp(op1,op2,op);
			*dt = BOOLEAN;
		}
		else {
			*dt = dt1;
		}
		
		return true;	
	}
	
	return false;
}

bool checkArithType(data_t *dt1, data_t *dt2)
{

	if (*dt1 == STRING || *dt2 == STRING) return false;
	if (*dt1 == BOOLEAN || *dt2 == BOOLEAN) return false;

	return true;
}

bool arithOp_next(scanner *scn, struct fsm_object *obj, data_t *dt, char *op)
{
	int op1,op2;
	char thisOp;
	bool finalOp;
	data_t dt2;
	char msg[256];

	if (tokenIs(scn,obj,T_PLUS) || tokenIs(scn,obj,T_MINUS)) {
		*op = scn->currentToken.value.string[0];
		if ( relation(scn,obj,dt) ) {
			op1 = parse_status.reg;
			finalOp = !arithOp_next(scn,obj,&dt2,&thisOp);
			op2 = parse_status.reg;
			if (!finalOp) {
				if (!checkArithType(dt,&dt2)) {
					sprintf(msg,"%s not compatible with %s near line %d",dt2string(dt),dt2string(&dt2),scn->curr_line);
					report(msg,ERROR);
					return false;
				}
				doOp(op1,op2,thisOp,false,false);
			}
			*dt = dt2;
			return true;
		}
	}
	return false;
}

bool arithOp(scanner *scn, struct fsm_object *obj, data_t *dt)
{
	data_t dt1, dt2;
	int op1,op2;
	char op;
	char msg[256];

	if ( relation(scn,obj,&dt1) ) {
		op1 = parse_status.reg;
		if ( arithOp_next(scn,obj,&dt2,&op)) {
			if (!checkArithType(&dt1,&dt2) || parse_status.array_id != NULL)
			{
				sprintf(msg,"%s not compatible with %s near line %d",dt2string(&dt1),dt2string(&dt2),scn->curr_line);
				report(msg,ERROR);
				return false;
			}
			op2 = parse_status.reg;
			doOp(op1,op2,op,false,false);
		}
		*dt = dt1;
		return true;	
	}
	return false;
}

bool expression_next(scanner *scn, struct fsm_object *obj, data_t *dt, char *op)
{
	data_t dt2;
	int op1, op2;
	char thisOp;
	bool finalExp;
	char msg[256];

	//scan(scn,obj);

	if (tokenIs(scn,obj,T_AND) || tokenIs(scn,obj,T_OR)) {
		*op = scn->currentToken.value.string[0];
		if ( relation(scn,obj,dt) ) {
			op1 = parse_status.reg;
			finalExp = !expression_next(scn,obj,&dt2,&thisOp);
			op2 = parse_status.reg;
			if (!finalExp) {
				if (!(*dt == dt2 && *dt == INTEGER)) {
					sprintf(msg,"Operands must be integers near line %d",scn->curr_line);
					report(msg,ERROR);
					return false;
				}
				doOp(op1,op2,thisOp,false,false);
				*dt = dt2;
			}
			return true;
		}
	}
	
	return false; 
}

bool expression(scanner *scn, struct fsm_object *obj, data_t *dt)
{
	data_t dt1, dt2;
	bool isNot = false;
	int op1, op2, result;
	char op; 
	char msg[256];
	//scan(scn,obj);
	if (tokenIs(scn,obj,T_NOT)) {
		isNot = true;
		//scan(scn,obj);
	}
	if ( arithOp(scn,obj,&dt1) ) {
		op1 = parse_status.reg;
		if (expression_next(scn,obj,&dt2,&op)) {
			if(!typeCheck(&dt1,&dt2) || parse_status.array_id != NULL) {
				sprintf(msg,"%s is not compatible with %s near line %d",dt2string(&dt1),dt2string(&dt2),scn->curr_line);
				report(msg,ERROR);
				return false;
			}
			op2 = parse_status.reg;
			doOp(op1,op2,op,false,false);
		}
		*dt = dt1;
		if (isNot && *dt == INTEGER)
		{
			result = parse_status.reg++;
			fprintf(parse_status.file,"\tR[%d] = ~R[%d];\n",parse_status.reg,result);
		}
		return true;
	}
	return false;	
}
	

bool destination(scanner *scn, struct fsm_object *obj, symbol ** tSymbol, int *index, bool *global)
{
	char msg[256];
	data_t dt; 

	//scan(scn,obj);
if (tokenIs(scn,obj,T_IDENTIFIER)) {
	strcpy(parse_status.curr_id,scn->currentToken.value.string);
	if (tokenIs(scn,obj,T_LBOX)) {
		if (expression(scn,obj,&dt)) {
			if (dt != INTEGER) {
				sprintf(msg,"Array indexes must be integers near line %d",scn->curr_line);
				report(msg, ERROR);
				return false;
			}
			// scan(scn,obj);
			if (!tokenIs(scn,obj,T_RBOX))
			{
				sprintf(msg,"Malformed destination expression found near line %d",scn->curr_line);
				report(msg,ERROR);
				return false;
			}
			*index = parse_status.reg;
		}
	}
	if (!findSymbol(parse_status.curr_id,global,tSymbol) ) {
		sprintf(msg,"Symbol %s not found near line %d",parse_status.curr_id,scn->curr_line);
		report(msg,ERROR);
		return false;
	}
	return true;
	}
	return false;
}

void stack_clean(gpointer key, gpointer value, gpointer numLocals)
{
	symbol *tSymbol = (symbol *)value;
	int *locs = (int *)numLocals;

	int addr = tSymbol->addr;
	if (addr > 0) {
		if ((tSymbol->st.st == VARIABLE) || (tSymbol->st.st == FUNCTION)) {
			(*locs)++;
		} else if (tSymbol->st.st == ARRAY) {
			(*locs) += tSymbol->st.size;
		}
	}
}
			 

bool assignment(scanner *scn, struct fsm_object *obj)
{
	char msg[256];
	data_t dt;
	symbol *s;
	bool global;
	int index = -1;
	int result,addr,addrReg;
	int numLocals = 0;
	//scan(scn,obj);

	//switch(scn->currentToken.tid)
	//{
	//	case T_IDENTIFIER:
			if ( destination(scn,obj,&s,&index,&global) || parse_status.error) {
				//scan(scn,obj);
				if (!tokenIs(scn,obj,T_ASSIGN)) {
					sprintf(msg,"Expecting assignment token near line %d",scn->curr_line);
					report(msg,ERROR);
					return false;
				}
				if (!expression(scn,obj,&dt)) {
					sprintf(msg,"Bad expression in assignment near line %d",scn->curr_line);
					report(msg,ERROR);
					return false;
				}

				if(!typeCheck(&(s->st.dt),&dt)) {
					sprintf(msg,"Bad assignment due to incompatible types near line %d",scn->curr_line);
					report(msg,ERROR);
					return false;
				}

				if (!strcmp(s->id,(char *)g_list_nth_data(parse_status.functions,parse_status.level))) {
					g_hash_table_foreach((GHashTable *)(g_list_last(parse_status.localSymbolTables)->data), stack_clean,&numLocals);
					if (numLocals > 0) {
						fprintf(parse_status.file,"\tR[SP] = R[SP] - %d;\n",numLocals);
					}
					calleeEnd(true);
					fprintf(parse_status.file," // %s\n",s->id);
					g_list_nth(parse_status.hReturn,parse_status.level)->data = GINT_TO_POINTER(true);
				}
				else {
					result = parse_status.reg;
					addr = s->addr;
					getMemLoc(addr,global);
					addrReg = parse_status.reg;
					if (index != -1) {
						fprintf(parse_status.file,"\tR[%d] = R[%d] %c R[%d];\n",addrReg,addrReg,(addr < 0 ? '-' : '+'),index);
					}
					fprintf(parse_status.file,"\tMM[R[%d]] = R[%d];\n",addrReg,result);
				}
				return true;
			}
			return false;		
			//break;
		/* default:
			sprintf(msg,"Bad destination statement found near line %d",scn->curr_line);
			report(msg,ERROR);
			return false;
			break;
	}*/
}

bool for_declare(scanner *scn, struct fsm_object *obj)
{
	char msg[256];
	data_t dt;
	int labelnum;

	if ( tokenIs(scn,obj,T_FOR) && assignment(scn,obj) ) {
		labelnum = parse_status.fCounter++;
		fprintf(parse_status.file,"beginfor%d: \n",labelnum);
		if (expression(scn,obj,&dt)) {
			fprintf(parse_status.file,"\tif (!R[%d]) goto endfor%d;\n",parse_status.reg,labelnum);
			if (dt != BOOLEAN && dt != INTEGER) {
				sprintf(msg,"Conditional expression must be BOOLEAN found near line %d",scn->curr_line);
				report(msg,ERROR);
				return false;
			}
			while (statement(scn,obj)) {
				//scan(scn,obj);
				if (!tokenIs(scn,obj,T_SEMICOLON)) {
					sprintf(msg,"Expecting a semicolon near line %d",scn->curr_line);
					report(msg,ERROR);
					return false;
				}
			}
			//scan(scn,obj);
			if (tokenIs(scn,obj, T_END)) {
				//scan(scn,obj);
				if (tokenIs(scn,obj,T_FOR)) {
					fprintf(parse_status.file,"\tgoto beginfor%d;\n",labelnum);
					fprintf(parse_status.file,"endfor%d:\n",labelnum);
					return true;
				} else {
					return false;
				}
			} 
		}
	}
	return false;
}

bool if_declare(scanner *scn, struct fsm_object *obj)
{
	char msg[256];
	data_t dt;
	int labelnum;
	
	if ( tokenIs(scn,obj,T_IF) && expression(scn,obj,&dt) ) {
		if (dt != BOOLEAN && dt != INTEGER) {
			sprintf(msg,"Conditional expressions must be BOOLEAN near line %d",scn->curr_line);
			report(msg,ERROR);
			return false;
		}
		labelnum = parse_status.fCounter++;
		fprintf(parse_status.file,"\tif (!R[%d]) goto else%d;\n",parse_status.reg,labelnum);
		//scan(scn,obj);
		if (tokenIs(scn,obj,T_THEN)) {
			if (statement(scn,obj)) {
				if (tokenIs(scn,obj,T_SEMICOLON)) {
					while (statement(scn,obj)) {
						//scan(scn,obj);
						if (!tokenIs(scn,obj,T_SEMICOLON)) {
							sprintf(msg,"Expected a semicolon near line %d",scn->curr_line);
							report(msg,ERROR);
							return false;
						}
					}
					fprintf(parse_status.file,"\tgoto endif%d;\n",labelnum);
					//scan(scn,obj);
					if (tokenIs(scn,obj,T_ELSE)) {
						fprintf(parse_status.file,"else%d:\n",labelnum);
						if (statement(scn,obj)) {
							//scan(scn,obj);
							if (tokenIs(scn,obj,T_SEMICOLON)) {
								while (statement(scn,obj)) {
									//scan(scn,obj);
									if (!tokenIs(scn,obj,T_SEMICOLON)) {
										sprintf(msg,"Expected a semicolon near line %d",scn->curr_line);
										report(msg,ERROR);
										return false;
									}
								}
							}
						}
					} else {
						fprintf(parse_status.file,"else%d:\n",labelnum);
					}
					fprintf(parse_status.file,"endif%d:\n",labelnum);
					//scan(scn,obj);
					if (tokenIs(scn,obj,T_END)) {
						//scan(scn,obj);
						if (tokenIs(scn,obj,T_IF)) {
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

bool statement(scanner *scn, struct fsm_object *obj)
{
	return assignment(scn,obj) || if_declare(scn,obj) || for_declare(scn,obj);
}

void store_function_addresses(gpointer key, gpointer value, gpointer user_data)
{
	symbol *tSymbol = (symbol *)value;
	int addr;
	if (tSymbol->st.st == FUNCTION) {
		addr = tSymbol->addr;
		parse_status.reg++;
		//fprintf(parse_status.file,"// %s\n",tSymbol->id);
		fprintf(parse_status.file,"\tR[%d] = %d;\n",parse_status.reg,addr);
		fprintf(parse_status.file,"\tR[%d] = R[%d] + R[FP];\n",parse_status.reg,parse_status.reg);
		fprintf(parse_status.file,"\tMM[R[%d]] = (int)&&%s;\n",parse_status.reg,(char *)key);
	}
}

bool var_declare(scanner *scn, struct fsm_object *obj, int addr, data_t *dt, symbol_t **st, bool *global)
{
	GHashTable *table;
	int tmpAddr;
	char id[256];
	char msg[256];
	int size;
	symbol * tSymbol;
	bzero(id,256);
	if (*global) {
		table = parse_status.gSymbolTable;
	} else {
		table = (GHashTable *)(g_list_last(parse_status.localSymbolTables)->data);
	}

	tmpAddr = *global ? parse_status.currentAddr++ : addr;
	if (tokenIs(scn,obj,T_IDENTIFIER)) {
		tSymbol = (symbol *)calloc(1,sizeof(symbol));
		tSymbol->id = (char *)calloc(strlen(scn->currentToken.value.string),sizeof(char));
		strcpy(tSymbol->id,scn->currentToken.value.string);
		if (tokenIs(scn,obj,T_LBOX) && tokenIs(scn,obj,T_INTCONST)) {
			size = scn->currentToken.value.intVal;
			if (tokenIs(scn,obj,T_RBOX)) {
				*st = (symbol_t *)calloc(1,sizeof(symbol_t));
				(*st)->st = ARRAY;
				(*st)->dt = *dt;
				(*st)->size = size;
				tSymbol->addr = tmpAddr;
				memcpy(&(tSymbol->st),*st,sizeof(symbol_t));
				g_hash_table_insert(table,tSymbol->id,(gpointer)tSymbol);
				return true;
			}
			sprintf(msg,"Expecting [ near line %d",scn->curr_line);
			report(msg,ERROR);
			return false;
		}
		*st = (symbol_t *)calloc(1,sizeof(symbol_t));
		(*st)->st = VARIABLE;
		(*st)->dt = *dt;
		(*st)->size = 1;
		//strcpy(tSymbol->id,id);
		tSymbol->addr = tmpAddr;
		memcpy(&(tSymbol->st), *st,sizeof(symbol_t));
		g_hash_table_insert(table,tSymbol->id,(gpointer)tSymbol);
		return true;
	}
	return false;			
}

bool declare(scanner *scn, struct fsm_object *obj, int addr, symbol_t **st, bool *global)
{
	data_t dt;
	if (tokenIs(scn,obj,T_GLOBAL))
	{
		if (parse_status.level == 0) {
			*global = true;
		} else {
			report("Global only valid at highest level.",WARN);
		}

	}
	if (tokenIs(scn,obj,T_INTEGER)) {
		dt = INTEGER;
		return function_declare(scn,obj,addr,dt,*global) || var_declare(scn,obj,addr,&dt,st,global);
	} 
	if (tokenIs(scn,obj,T_FLOAT)) {
		dt = FLOAT;
		return function_declare(scn,obj,addr,dt,*global) || var_declare(scn,obj,addr,&dt,st,global);
	}	
	if (tokenIs(scn,obj,T_STRING)) {
		dt = STRING;
		return function_declare(scn,obj,addr,dt,*global) || var_declare(scn,obj,addr,&dt,st,global);
	}
	if (tokenIs(scn,obj,T_BOOLEAN)) {
		dt = BOOLEAN;
		return function_declare(scn,obj,addr,dt,*global) || var_declare(scn,obj,addr,&dt,st,global);
	}
	return false;
}

int getLocalCount()
{
	int numLocals = 0;
	g_hash_table_foreach((GHashTable *)(g_list_last(parse_status.localSymbolTables)->data),stack_clean,&numLocals);
	return numLocals;
}

bool function_body(scanner *scn, struct fsm_object *obj, char *id)
{
	char msg[256];
	bool isFunction = false;
	int  addr = 3;
	bool global = false;
	symbol_t *tSymbol_t;
	symbol *tSymbol = (symbol *)calloc(1,sizeof(symbol));
	//GHashTable *table; = (GHashTable *)(g_list_last(parse_status.localSymbolTables)->data);
	while (declare(scn,obj,addr,&tSymbol_t,&global) || parse_status.error) {
		parse_status.error = false;
		
		//g_hash_table_insert(table,tSymbol->id,(gpointer)tSymbol);
		if (!tokenIs(scn,obj,T_SEMICOLON)) {
			sprintf(msg,"Expecting ; near line %d",scn->curr_line);
			report(msg,ERROR);
			return false;
		} else if (!global) {
			if (tSymbol_t->st == ARRAY) {
				addr += tSymbol_t->size;
			} else {
				addr++;
			}
		}
		global = false;

	}

	if (tokenIs(scn,obj,T_BEGIN)) {
		fprintf(parse_status.file,"%s_begin:\n",id);
		fprintf(parse_status.file,"\tR[SP] = R[SP] + %d;\n",getLocalCount());
		parse_status.reg = 0;
		
		g_hash_table_foreach((GHashTable *)(g_list_last(parse_status.localSymbolTables)->data),(GHFunc)store_function_addresses,NULL);
		//printf("[DEBUG] Function %s body --\n",id); 
		//g_hash_table_foreach((GHashTable *)(g_list_nth(parse_status.localSymbolTables,parse_status.level - 1)->data),(GHFunc)printTable,NULL); 
	


	while (statement(scn,obj) || parse_status.error)
	{
		parse_status.error = false;
		//scan(scn,obj);
		if(!tokenIs(scn,obj,T_SEMICOLON))
		{
			sprintf(msg,"Expecting a semicolon near line %d",scn->curr_line);
			report(msg,ERROR);
			return false;
		}
	} 

	if (tokenIs(scn,obj,T_END) && tokenIs(scn,obj,T_FUNCTION)) {
		if (!(bool) GPOINTER_TO_INT(g_list_nth_data(parse_status.hReturn,parse_status.level))) {
			sprintf(msg,"Expecting a return from function near line %d",scn->curr_line);
			report(msg,ERROR);
			return false;
		}
		parse_status.hReturn = g_list_delete_link(parse_status.hReturn,g_list_last(parse_status.hReturn));
		//free(g_list_last(parse_status.functions)->data);
		parse_status.functions = g_list_delete_link(parse_status.functions,g_list_last(parse_status.functions));
		return true;
	}
	}
	return false;
}

bool getDt(scanner *scn, struct fsm_object *obj, data_t *dt)
{
	if (tokenIs(scn,obj,T_INTEGER)) {
		*dt = INTEGER;
		return true;
	} else if (tokenIs(scn,obj,T_FLOAT)) {
		*dt = FLOAT;
		return true;
	} else if (tokenIs(scn,obj,T_BOOLEAN)) {
		*dt = BOOLEAN;
		return true;
	} else if (tokenIs(scn,obj,T_STRING)) {
		*dt = STRING;
		return true;
	} else {
		return false;
	}
}

GList *parameter_list(scanner *scn, struct fsm_object *obj, int  addr)
{
	char msg[256];
	symbol_t *tSymbol_t = NULL; 
	GList *params = NULL;
	bool isFunction = false;
	bool global = false;
	GHashTable *currSymbolTable;
	symbol *tSymbol;
	data_t dt;

	if (getDt(scn,obj,&dt) && var_declare(scn,obj,addr,&dt,&tSymbol_t,&global)) {
		// Update addr
		if (tSymbol_t->st == ARRAY)
		{
			addr -= tSymbol_t->size;
		}
		else
		{
			addr--;
		}

		if (tSymbol_t != NULL) {	
			params = g_list_append(params,(gpointer)tSymbol_t);
		}

				
		if (tokenIs(scn,obj,T_COMMA) && !parameter_list(scn,obj,addr)) {
			sprintf(msg,"Syntax error in parameter list found near line %d",scn->curr_line);
			report(msg,ERROR);
			return NULL;
		}
		return params;
	}

	return params;
}

void calleeBegin()
{
	fprintf(parse_status.file,"\tR[SP] = R[SP] + 1;\n");
	fprintf(parse_status.file,"\tMM[R[SP]] = R[FP];\n");
	fprintf(parse_status.file,"\tR[FP] = R[SP] - 1;\n");
}

bool function_declare(scanner *scn, struct fsm_object *obj, int addr, data_t dt, bool global)
{
	char msg[256];
	long fAddr;
	bool retVal;
	GList *params;
	GHashTable *table;
	symbol * tSymbol; 
	symbol_t * tSymbol_t;
	if (tokenIs(scn,obj,T_FUNCTION) && tokenIs(scn,obj,T_IDENTIFIER)) {
		parse_status.level++;
		table =  g_hash_table_new(g_str_hash,g_str_equal);
		parse_status.localSymbolTables = g_list_append(parse_status.localSymbolTables,(gpointer)(table));
		tSymbol = (symbol *)calloc(1,sizeof(symbol));
		tSymbol_t = (symbol_t *)calloc(1,sizeof(symbol_t));
		// Build the symbol for function
		tSymbol->id = (char *)calloc(strlen(scn->currentToken.value.string),sizeof(char));
		strcpy(tSymbol->id,scn->currentToken.value.string);
	     if (tokenIs(scn,obj,T_LPAREN)) {	
		params = parameter_list(scn,obj,-1);
		if (tokenIs(scn,obj,T_RPAREN)) {
			tSymbol_t->st = FUNCTION;
			tSymbol_t->dt = dt;
			tSymbol_t->size = 0;
			tSymbol_t->params = params;
			memcpy(&(tSymbol->st),tSymbol_t,sizeof(symbol_t));
			
			if (global) {
				table = parse_status.gSymbolTable;
				fAddr = parse_status.currentAddr++; 
			} else {
				fAddr = 2;
			}

			tSymbol->addr = fAddr;

			// Add symbol to current table		
			g_hash_table_insert(table,tSymbol->id,(gpointer)tSymbol);
			symbol * tSymbol2 = (symbol *)calloc(1,sizeof(symbol));
			tSymbol2->addr = addr;
			tSymbol2->id = tSymbol->id;
			memcpy(&(tSymbol2->st),&(tSymbol->st),sizeof(symbol_t)); 

			// If we are deeper than first level, add symbol to table above us
			if (parse_status.level != 0 && !global) {
				table = (GHashTable *)(g_list_nth(parse_status.localSymbolTables,parse_status.level - 1)->data);
				g_hash_table_insert(table,tSymbol->id,(gpointer)tSymbol2);
				//printf("[DEBUG] -- Function %s high level\n",tSymbol->id);
				//g_hash_table_foreach(table,printTable,NULL);	
				
			}

			parse_status.functions = g_list_append(parse_status.functions,(gpointer)(tSymbol->id));
			parse_status.hReturn = g_list_append(parse_status.hReturn,GINT_TO_POINTER(false));

			fprintf(parse_status.file,"%s:\n",tSymbol->id);
			calleeBegin();
			fprintf(parse_status.file,"\tgoto %s_begin;\n",tSymbol->id);

			retVal =  function_body(scn,obj,tSymbol->id);
			//printf("[DEBUG] -- Function %s\n",tSymbol->id);
			//g_hash_table_foreach(table,printTable,NULL);	
			parse_status.localSymbolTables = g_list_remove_link(parse_status.localSymbolTables,g_list_last(parse_status.localSymbolTables));
			parse_status.level--;
		}
	} else {
		retVal = false;
	}

	} else {
		retVal = false;
	}

	return retVal;
}

void calleeEnd(bool restorePointers)
{
	int ret = parse_status.reg;
	parse_status.reg++;
	
	if (restorePointers)
	{
		fprintf(parse_status.file,"\tR[FP] = MM[R[SP]];\n");
		fprintf(parse_status.file,"\tR[SP] = R[SP] - 1;\n");
	}

	fprintf(parse_status.file,"\tR[%d] = MM[R[SP]];\n", parse_status.reg);
	fprintf(parse_status.file,"\tMM[R[SP]] = R[%d];\n", ret);
	fprintf(parse_status.file,"\tgoto *(void*)R[%d];\n",parse_status.reg);

	parse_status.reg = 0;
}

void parse_init()
{
	symbol *tSymbol;
	symbol_t *tSymbol_t;
	parse_status.gSymbolTable = g_hash_table_new(g_str_hash,g_str_equal);
	parse_status.currentAddr = 0;
	parse_status.reg = 0;
	parse_status.level = -1;
	parse_status.pScan = false;
	parse_status.error = false;
	parse_status.errorCount = 0;
	
	// getBool - runtime
	tSymbol = (symbol *)calloc(1,sizeof(symbol));
	tSymbol->id = "getBool";
	tSymbol->st.st = FUNCTION;
	tSymbol->st.dt = BOOLEAN;
	tSymbol->st.size = 1;
	tSymbol->addr = parse_status.currentAddr++; 
	g_hash_table_insert(parse_status.gSymbolTable, "getBool", (gpointer)tSymbol);	
	
	fprintf(parse_status.file,"getBool:\n");
	fprintf(parse_status.file,"\tR[0] = (int)getBool();\n");
	calleeEnd(false);
	
	// getInt - runtime
	tSymbol = (symbol *)calloc(1,sizeof(symbol));
	tSymbol->id = "getInt";
	tSymbol->st.st = FUNCTION;
	tSymbol->st.dt = INTEGER;
	tSymbol->st.size = 1;
	tSymbol->addr = parse_status.currentAddr++;
	g_hash_table_insert(parse_status.gSymbolTable, "getInt", (gpointer)tSymbol);

	fprintf(parse_status.file,"getInt:\n");
	fprintf(parse_status.file,"\tR[0] = getInt();\n");
	calleeEnd(false);	

	// getString - runtime
	tSymbol = (symbol *)calloc(1,sizeof(symbol));
	tSymbol->id = "getString";
	tSymbol->st.st = FUNCTION;
	tSymbol->st.dt = STRING;
	tSymbol->st.size = 1;
	tSymbol->addr = parse_status.currentAddr++;
	g_hash_table_insert(parse_status.gSymbolTable, "getString", (gpointer)tSymbol);

	fprintf(parse_status.file,"getString:\n");
	fprintf(parse_status.file,"\tgetString(tmpString);\n");
	fprintf(parse_status.file,"\tR[0] = (int)tmpString;\n");
	calleeEnd(false);

	// getFloat - runtime
	tSymbol = (symbol *)calloc(1,sizeof(symbol));
	tSymbol->id = "getFloat";
	tSymbol->st.st = FUNCTION;
	tSymbol->st.dt = FLOAT;
	tSymbol->st.size = 1;
	tSymbol->addr = parse_status.currentAddr++;
	g_hash_table_insert(parse_status.gSymbolTable, "getFloat", (gpointer)tSymbol);

	fprintf(parse_status.file,"getFloat:\n");
	fprintf(parse_status.file,"\ttmpFloat = getFloat();\n");
	fprintf(parse_status.file,"\tmemcpy(&R[0], &tmpFloat, sizeof(float));\n");
	calleeEnd(false);	

	// putBool - runtime
	tSymbol = (symbol *)calloc(1,sizeof(symbol));
	tSymbol_t = (symbol_t *)calloc(1,sizeof(symbol_t));
	tSymbol->id = "putBool";
	tSymbol->st.st = FUNCTION;
	tSymbol->st.dt = INTEGER;
	tSymbol->st.size = 0;
	tSymbol_t->st = VARIABLE;
	tSymbol_t->dt = BOOLEAN;
	tSymbol_t->size = 1;
	tSymbol->st.params = g_list_append(tSymbol->st.params,(gpointer)tSymbol_t);
	tSymbol->addr = parse_status.currentAddr++;
	g_hash_table_insert(parse_status.gSymbolTable, "putBool", (gpointer)tSymbol);

	parse_status.reg = 1;
	fprintf(parse_status.file,"putBool:\n");
	fprintf(parse_status.file,"\tR[0] = MM[R[SP] - 1];\n");
	fprintf(parse_status.file,"\tR[1] = putBool((bool)R[0]);\n");
	calleeEnd(false);

	// putInt - runtume
	tSymbol = (symbol *)calloc(1,sizeof(symbol));
	tSymbol_t = (symbol_t *)calloc(1,sizeof(symbol));
	tSymbol->id = "putInt";
	tSymbol->st.st = FUNCTION;
	tSymbol->st.dt = INTEGER;
	tSymbol->st.size = 0;
	tSymbol_t->st = VARIABLE;
	tSymbol_t->dt = INTEGER;
	tSymbol_t->size = 1;
	tSymbol->st.params = g_list_append(tSymbol->st.params,(gpointer)tSymbol_t);
	tSymbol->addr = parse_status.currentAddr++;
	g_hash_table_insert(parse_status.gSymbolTable, "putInt",(gpointer)tSymbol);

	parse_status.reg = 1;
	fprintf(parse_status.file,"putInt:\n");
	fprintf(parse_status.file,"\tR[0] = MM[R[SP] - 1];\n");
	fprintf(parse_status.file,"\tR[1] = putInt(R[0]);\n");
	calleeEnd(false);

	// sqrt - runtime
	tSymbol = (symbol *)calloc(1,sizeof(symbol));
	tSymbol_t = (symbol_t *)calloc(1,sizeof(symbol_t));
	tSymbol->id = "sqrt";
	tSymbol->st.st = FUNCTION;
	tSymbol->st.dt = FLOAT;	 
	tSymbol->st.size = 0;
	tSymbol_t->st = VARIABLE;
	tSymbol_t->dt = FLOAT;
	tSymbol_t->size = 1;
	tSymbol->st.params = g_list_append(tSymbol->st.params,(gpointer)tSymbol_t);
	tSymbol->addr = parse_status.currentAddr++;
	g_hash_table_insert(parse_status.gSymbolTable, "sqrt",(gpointer)tSymbol);

	parse_status.reg = 1;
	fprintf(parse_status.file,"sqrt:\n");
	fprintf(parse_status.file,"\tR[0] = MM[R[SP] - 1];\n");
	fprintf(parse_status.file,"\ttmpFloat = sqrt(R[0]);\n");
	fprintf(parse_status.file,"\tmemcpy(&R[1],&tmpFloat,sizeof(float));\n");
	calleeEnd(false);

	// putString - runtime
	tSymbol = (symbol *)calloc(1,sizeof(symbol));
	tSymbol_t = (symbol_t *)calloc(1,sizeof(symbol_t));
	tSymbol->id = "putString";
	tSymbol->st.st = FUNCTION;
	tSymbol->st.dt = INTEGER;	 
	tSymbol->st.size = 0;
	tSymbol_t->st = VARIABLE;
	tSymbol_t->dt = STRING;
	tSymbol_t->size = 1;
	tSymbol->st.params = g_list_append(tSymbol->st.params,(gpointer)tSymbol_t);
	tSymbol->addr = parse_status.currentAddr++;
	g_hash_table_insert(parse_status.gSymbolTable, "putString",(gpointer)tSymbol);

	parse_status.reg = 1;
	fprintf(parse_status.file,"putString:\n");
	fprintf(parse_status.file,"\tR[0] = MM[R[SP] - 1];\n");
	fprintf(parse_status.file,"\tR[1] = putString((char *)R[0]);\n");
	calleeEnd(false);

	// putFloat - runtime
	tSymbol = (symbol *)calloc(1,sizeof(symbol));
	tSymbol_t = (symbol_t *)calloc(1,sizeof(symbol_t));
	tSymbol->id = "putFloat";
	tSymbol->st.st = FUNCTION;
	tSymbol->st.dt = INTEGER;	 
	tSymbol->st.size = 0;
	tSymbol_t->st = VARIABLE;
	tSymbol_t->dt = FLOAT;
	tSymbol_t->size = 1;
	tSymbol->st.params = g_list_append(tSymbol->st.params,(gpointer)tSymbol_t);
	tSymbol->addr = parse_status.currentAddr++;
	g_hash_table_insert(parse_status.gSymbolTable, "putFloat",(gpointer)tSymbol);

	parse_status.reg = 1;
	fprintf(parse_status.file,"putFloat:\n");
	fprintf(parse_status.file,"\tR[0] = MM[R[SP] - 1];\n");
	fprintf(parse_status.file,"\tmemcpy(&tmpFloat, &R[0], sizeof(float));\n");
	fprintf(parse_status.file,"\tR[1] = putFloat(tmpFloat);\n");
	calleeEnd(false);

//	g_hash_table_foreach(parse_status.gSymbolTable,printTable,NULL);
}
void setGlobalFunctions(gpointer key, gpointer value, gpointer user)
{
	symbol * tSymbol = (symbol *)value;

	if (tSymbol->st.st == FUNCTION) {
		fprintf(parse_status.file,"\tR[0] = %d;\n",(int )tSymbol->addr);
		fprintf(parse_status.file,"\tMM[R[0]] = (int)&&%s;\n",tSymbol->id);
	}
}
int main(int argc, char **argv)
{
	struct fsm_object obj;
	scanner scn;
	data_t dt;
	char msg[256];

    	if (!init(argc,argv,&scn,&gDebug)) { return -1; }
    
	parse_status.file = fopen("temp.c", "w");
	if (parse_status.file == NULL)
	{
		report("Couldn't open temporary file",ERROR);
		return 0;
	}

	fprintf(parse_status.file, "#include <stdbool.h>\n");
	fprintf(parse_status.file, "#include <math.h>\n");
	fprintf(parse_status.file, "#include <string.h>\n");
	fprintf(parse_status.file, "#include <stdio.h>\n");
	fprintf(parse_status.file, "#include \"runtime.h\"\n\n");
	fprintf(parse_status.file, "int main()\n");
	fprintf(parse_status.file, "{\n");
	fprintf(parse_status.file,"\tgoto _main;\n");
	fprintf(parse_status.file,"\n");

	parse_init(); 

	while (scn.currentToken.tid != T_EOF)
	{
		scan(&scn,&obj);
		
		switch(scn.currentToken.tid)
		{
			case T_INTEGER:
				dt = INTEGER;
				break;
			case T_FLOAT:
				dt = FLOAT;
				break;
			case T_BOOLEAN:
				dt = BOOLEAN;
				break;
			case T_STRING:
				dt = STRING;
				break;
			case T_EOF:
				continue;
			default:
				//sprintf(msg,"Unexpected token found near line %d",scn.curr_line);
				//report(msg,ERROR);
				continue;
		}
		//scan(&scn,&obj);
		//if (tokenIs(scn,obj,T_FUNCTION)) {
			function_declare(&scn,&obj,dt,0,false);
		//} else {
		//	sprintf(msg,"Expecting FUNCTION keyword near line %d",scn.curr_line);
		//	report(msg,ERROR);
		//	continue;
		//}
	}

	fprintf(parse_status.file,"_main:\n");

	g_hash_table_foreach(parse_status.gSymbolTable,setGlobalFunctions,NULL);

	fprintf(parse_status.file,"\tR[SP] = STACK_START;\n");
	fprintf(parse_status.file,"\tR[FP] = STACK_START;\n");
	fprintf(parse_status.file,"\tMM[R[SP]] = (int)&&_end;\n");
	fprintf(parse_status.file,"\tgoto main;\n");
	fprintf(parse_status.file,"_end:\n");
	fprintf(parse_status.file,"\treturn MM[R[SP]];\n");
	fprintf(parse_status.file,"}\n");
	
	report("Parse complete",INFO);
	fclose(scn.fp);
	fclose(parse_status.file);
    	return 0;
}

