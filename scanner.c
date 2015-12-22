/*
 * Author: Benjamin D Jones
 * File: scanner.c
 * Purpose: This implements a hand-rolled lexical analyzer utilizing deterministic finite 
 *          automata. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include "fsm.h"
#include "scanner.h"
#include "keyword.h"

unsigned int getTokenType(char *str)
{
	unsigned int len;
	unsigned int i;
	char c;
	len = strlen(str);

	// If string is single character token_t is ASCII value
	if (len == 1) { c = *str; return (int)c; };

	if (!strcmp(str,"function"))
	{
		return T_FUNCTION;
	}
	
	if (!strcmp(str,"begin"))
	{
		return T_BEGIN;
	}

	if (!strcmp(str,"end"))
	{
		return T_END;
	}

	if (!strcmp(str,"global"))
	{
		return T_GLOBAL;
	}

	if (!strcmp(str,"integer"))
	{
		return T_INTEGER;
	}

	if (!strcmp(str,"float"))
	{
		return T_FLOAT;
	}

	if (!strcmp(str,"boolean"))
	{
		return T_BOOLEAN;
	}

	if (!strcmp(str,"string"))
	{
		return T_STRING;
	}

	if (!strcmp(str,"if"))
	{
		return T_IF;
	}

	if (!strcmp(str,"then"))
	{
		return T_THEN;
	}

	if (!strcmp(str,"else"))
	{
		return T_ELSE;
	}

	if (!strcmp(str,"for"))
	{
		return T_FOR;
	}

	if(!strcmp(str,"true"))
	{
		return T_BOOLCONST;
	}

	if(!strcmp(str,"false"))
	{
		return T_BOOLCONST;
	}

	if (!strcmp(str,":="))
	{
		return T_ASSIGN;
	}	

	if (!strcmp(str,">="))
	{	
		return T_GTEQ;
	}

	if (!strcmp(str,"<="))
	{
		return T_LTEQ;
	}

	if (!strcmp(str,"=="))
	{
		return T_EQUAL;
	}

	if (!strcmp(str,"!="))
	{
		return T_NOTEQUAL;
	}

	if (!strcmp(str,"not"))
	{
		return T_NOT;
	}

	return T_UNKNOWN;
}	

void initTable(scanner *scn)
{
	unsigned int i;
	char msg[256];
	scn->table = g_hash_table_new(g_str_hash,g_str_equal);
	for (i = 0; i < NUM_KEYS; i++)
	{
		g_hash_table_insert(scn->table, keywords[i], GINT_TO_POINTER(getTokenType(keywords[i])));
	}

	for (i = 0; i < NUM_OPS; i++)
	{
		g_hash_table_insert(scn->table, operators[i], GINT_TO_POINTER(getTokenType(operators[i])));
	}

	sprintf(msg,"Hash table filled with %d keys",g_hash_table_size(scn->table));
	report(msg,INFO);
}

bool init(int argc, char **argv, scanner *scn, bool *gDebug)
{
    int c;
    char * filename = NULL;

    while ((c = getopt (argc, argv, "df:")) != -1)
    {
       switch (c)
       {
           case 'f':
               filename = optarg;
               break;
	   case 'd':
	       *gDebug = true;
	       break;	
           default:
               fprintf(stderr, "Usage: [-f filename]\n");
	       return false;
       }
    }

    scn->fp = fopen(filename, "r");

    if ((filename ==  NULL) || (scn->fp == NULL))
    {
        printf("Couldn't open file...\n");
        return false;
    }

    scn->curr_line = 1;
    scn->bufLoc = scn->buf;
    initTable(scn);
    bzero(scn->buf,MAX_ID);
    return true;
}

void start(struct fsm_object *obj, int val, void **arg)
{
    scanner *scn = (scanner *)arg;	
    bzero(scn->buf,MAX_ID);
    bzero(scn->currentToken.value.string,MAX_ID);
    scn->bufLoc = scn->buf;
    /* state -> default */
    char c = getc(scn->fp);
    char msg[MAX_ID];
    unsigned int cnt = 0;	
    if (c == EOF)
    {
        report("EOF reached",INFO);
	scn->currentToken.tid = T_EOF;
        fsm_terminate(obj);
	return;
    }
    
	switch (c)
	{
		case '/':
			if ( (c = getc(scn->fp) == '/'))
			{
				while ( (c = getc(scn->fp)) != '\n') { continue; }
				scn->curr_line++;
			}
			else
			{
				ungetc(c,scn->fp);
				scn->currentToken.tid = 
					GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"/"));
				scn->currentToken.value.string[0] = '/';
				fsm_terminate(obj);
			}
			break;
		case '\n':
			scn->curr_line++;
			fsm_to_state(obj,"default",0,(void **)scn);
			break;
		case '+':
			scn->currentToken.tid = 
				GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"+"));
			scn->currentToken.value.string[0] = '+';
 			fsm_terminate(obj);
			break;
		case '-':
			scn->currentToken.tid = 
				GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"-"));
			scn->currentToken.value.string[0] = '-';
			fsm_terminate(obj);
			break;
		case '*':
			scn->currentToken.tid = 
				GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"*"));
			scn->currentToken.value.string[0] = '*';
			 fsm_terminate(obj);
			break;
		case '(':
			scn->currentToken.tid = 
				GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"("));
			scn->currentToken.value.string[0] = '(';
			 fsm_terminate(obj);
			break;
		case ')':
			scn->currentToken.tid = 
				GPOINTER_TO_INT(g_hash_table_lookup(scn->table,")"));
			scn->currentToken.value.string[0] = ')';
			fsm_terminate(obj);
			break;
		case '[':
			scn->currentToken.tid = 
				GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"["));
			scn->currentToken.value.string[0] = '[';
 			fsm_terminate(obj);
			break;
		case ']':
			scn->currentToken.tid = 
				GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"]"));
			scn->currentToken.value.string[0] = ']';
			fsm_terminate(obj);	
			break;
		case '"':
			cnt = 0;
			c = getc(scn->fp);
			while (c  != '"')
			{
				if (c == '\\')
				{
					c = getc(scn->fp);
					if (c == 'n') {
						*(scn->bufLoc) = '\\';
						scn->bufLoc++;
						cnt++;
					}
				}
				*(scn->bufLoc) = c;
				scn->bufLoc++;
				cnt++;
				if ( cnt == MAX_ID)
				{
					sprintf(msg,"String literal exceeds maximum length near line %d",scn->curr_line);
					report(msg,ERROR);
					scn->currentToken.tid = T_UNKNOWN;
					fsm_terminate(obj);
					return;
				}
				c = getc(scn->fp);	
			}
			scn->currentToken.tid = T_STRINGCONST;
			memcpy(scn->currentToken.value.string,scn->buf,strlen(scn->buf));
			fsm_terminate(obj);
			break;
		case ',':
			scn->currentToken.tid = 
				GPOINTER_TO_INT(g_hash_table_lookup(scn->table,","));
			scn->currentToken.value.string[0] = ',';
			fsm_terminate(obj);
			break;
		case '|':
			scn->currentToken.tid = 
				GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"|"));
			scn->currentToken.value.string[0] = '|';
			fsm_terminate(obj);
			break;
		case '&':
			scn->currentToken.tid = 
				GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"&"));
			scn->currentToken.value.string[0] = '&';
			fsm_terminate(obj);
			break;
		case '=':
			if ( (c = getc(scn->fp)) == '=')
			{
				scn->currentToken.tid =
					GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"=="));
				scn->currentToken.value.string[0] = '=';
				scn->currentToken.value.string[1] = '=';
				fsm_terminate(obj);
			}
			else
			{
				ungetc(c,scn->fp);
				scn->currentToken.tid = T_UNKNOWN;
				sprintf(msg,"Single = found near line %d",scn->curr_line);
				report(msg,ERROR);
				fsm_terminate(obj);
			}
			break;
		case ':':
			if ( (c = getc(scn->fp)) == '=')
			{
				scn->currentToken.tid =
					GPOINTER_TO_INT(g_hash_table_lookup(scn->table,":="));
				scn->currentToken.value.string[0] = ':';
				scn->currentToken.value.string[1] = '=';
				fsm_terminate(obj);
			}
			else
			{
				ungetc(c,scn->fp);
				scn->currentToken.tid = T_UNKNOWN;
				sprintf(msg,"Malformed assignment operator found near line %d",scn->curr_line);
				report(msg,ERROR);
				fsm_terminate(obj);
			}
			break;
		case '<':
			if ( (c = getc(scn->fp)) == '=')
			{
				scn->currentToken.tid =
					GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"<="));
				scn->currentToken.value.string[0] = '<';
				scn->currentToken.value.string[1] = '=';
				fsm_terminate(obj);
			}
			else
			{
				ungetc(c,scn->fp);
				scn->currentToken.tid = 
					GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"<"));
				scn->currentToken.value.string[0] = '<';
				fsm_terminate(obj);
			}
			break;
		case '>':
			if ( (c = getc(scn->fp)) == '=')
			{
				scn->currentToken.tid =
					GPOINTER_TO_INT(g_hash_table_lookup(scn->table,">="));
				scn->currentToken.value.string[0] = '>';
				scn->currentToken.value.string[1] = '=';
				fsm_terminate(obj);
			}
			else
			{
				ungetc(c,scn->fp);
				scn->currentToken.tid = 
					GPOINTER_TO_INT(g_hash_table_lookup(scn->table,">"));
				scn->currentToken.value.string[0] = '<';
				fsm_terminate(obj);
			}
			break;
		case '!':
			if ( (c = getc(scn->fp)) == '=')
			{
				scn->currentToken.tid =
					GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"!="));
				scn->currentToken.value.string[0] = '!';
				scn->currentToken.value.string[1] = '=';
				fsm_terminate(obj);
			}
			else
			{
				ungetc(c,scn->fp);
				scn->currentToken.tid = 
					GPOINTER_TO_INT(g_hash_table_lookup(scn->table,"!"));
				scn->currentToken.value.string[0] = '<';
				fsm_terminate(obj);
			}
			break;
		case ';':
			scn->currentToken.tid = 
				GPOINTER_TO_INT(g_hash_table_lookup(scn->table,";"));
			scn->currentToken.value.string[0] = ';';
			fsm_terminate(obj);
			break;
		case ' ':
			fsm_to_state(obj,"default",0,(void **)scn);
			break;
		case '\t':
			fsm_to_state(obj,"default",0,(void **)scn);
			break;
		default:
			*(scn->bufLoc) = c;
			scn->bufLoc++;
			fsm_to_state(obj,"id",0,(void **)scn);
			break;
	}
}
void floatl(struct fsm_object *obj, int val, void **arg)
{
	scanner *scn = (scanner *)arg;
	char c = getc(scn->fp);
	char msg[MAX_ID];

	if ( (c  >= '0') && (c <= '9'))
	{
		*(scn->bufLoc) = c;
		scn->bufLoc++;
		if ((scn->bufLoc - scn->buf) >= MAX_ID)
		{
			sprintf(msg,"Literal exceeds maximum length near line %d",scn->curr_line);
			report(msg,ERROR);
			scn->currentToken.tid = T_UNKNOWN; 
			fsm_terminate(obj);
			return;
		}
		fsm_to_state(obj,"float",0,(void **)scn);
		return;
	}

	if (c == '.')
	{
		sprintf(msg,"Invalid float literal near line %d",scn->curr_line);
		report(msg,ERROR);
		scn->currentToken.tid = T_UNKNOWN;
		fsm_terminate(obj);
		return;
	}
	if (c)
	{
		ungetc(c,scn->fp);
		scn->currentToken.tid = T_FLOATCONST;
		scn->currentToken.value.doubleVal = atof(scn->buf);
		fsm_terminate(obj);
		return;
	}
	sprintf(msg,"NULL byte received near line %d",scn->curr_line);
	report(msg,ERROR);
	scn->currentToken.tid = T_UNKNOWN;
	fsm_terminate(obj);
}

void number(struct fsm_object *obj, int val, void **arg)
{
	scanner *scn = (scanner *)arg;
	char c = getc(scn->fp);
	char msg[MAX_ID];

	if ( (c  >= '0') && (c <= '9'))
	{
		*(scn->bufLoc) = c;
		scn->bufLoc++;
		if ((scn->bufLoc - scn->buf) >= MAX_ID)
		{
			sprintf(msg,"Literal exceeds maximum length near line %d",scn->curr_line);
			report(msg,ERROR);
			scn->currentToken.tid = T_UNKNOWN; 
			fsm_terminate(obj);
			return;
		}
		fsm_to_state(obj,"number",0,(void **)scn);
		return;
	}

	if (c == '.')
	{
		*(scn->bufLoc) = c;
		scn->bufLoc++;
		fsm_to_state(obj,"float",0,(void **)scn);
		return;
	}

	if (c)
	{
		ungetc(c,scn->fp);
		scn->currentToken.tid = T_INTCONST;
		scn->currentToken.value.intVal = atoi(scn->buf);
		fsm_terminate(obj);
		return;
	}
	sprintf(msg,"NULL byte received near line %d",scn->curr_line);
	report(msg,ERROR);
	scn->currentToken.tid = T_UNKNOWN;
	fsm_terminate(obj);
}
			

void alpha(struct fsm_object *obj, int val, void **arg)
{
	scanner *scn = (scanner *)arg;
	char c;
	bool keepBuilding = true;
	char msg[MAX_ID];
	while (keepBuilding)
	{
		c = getc(scn->fp);
		
		if ( ( (c >= '0') && (c <= '9')) ||
		     ( (c >= 'a') && (c <= 'z')) ||
                     ( (c >= 'A') && (c <= 'Z')) )
		{
			*(scn->bufLoc) = c;
			scn->bufLoc++;
			if ( (scn->bufLoc - scn->buf) >= MAX_ID)
			{
				sprintf(msg,"Identifier exceeds maximum length near line %d",scn->curr_line);
				report(msg,ERROR);
				scn->currentToken.tid = T_UNKNOWN;
				fsm_terminate(obj);
				keepBuilding = false;
			}
		}
		else
		{
			keepBuilding = false;
			ungetc(c,scn->fp);		
			if (g_hash_table_lookup_extended(scn->table,scn->buf,NULL,(gpointer)&(scn->currentToken.tid)))
			{
				memcpy(scn->currentToken.value.string,scn->buf,strlen(scn->buf));
				if (!strcmp(scn->currentToken.value.string,"true")) {
					scn->currentToken.value.boolVal = true;
				} else if (!strcmp(scn->currentToken.value.string,"false")) {
					scn->currentToken.value.boolVal = false;
				}
				fsm_terminate(obj);
			}
			else
			{
				g_hash_table_insert(scn->table,scn->buf,GINT_TO_POINTER(T_IDENTIFIER));
				scn->currentToken.tid = T_IDENTIFIER;
				memcpy(scn->currentToken.value.string,scn->buf,strlen(scn->buf));
				fsm_terminate(obj);
			}
		}
	}
	 	
	fsm_terminate(obj);
}
void id(struct fsm_object *obj, int val, void **arg)
{
	char msg[256];
	scanner *scn = (scanner *)arg;
	if ( (scn->buf[0] >= '0') && (scn->buf[0] <= '9'))
	{
		fsm_to_state(obj,"number",0,(void **)scn);
		return;
	}

	if ( ((scn->buf[0] >= 'A') && (scn->buf[0] <= 'Z')) ||
		((scn->buf[0] >= 'a' ) && (scn->buf[0] <= 'z')))
	{
		fsm_to_state(obj,"alpha",0,(void **)scn);
		return;
	}
	
	sprintf(msg,"Unknown character %d encountered on line %d", scn->buf[0], scn->curr_line);
	report(msg,WARN);
	fsm_to_state(obj,"default",0,(void **)scn);
}

 
