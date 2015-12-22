#include "symbol.h"

typedef struct parser
{
	FILE * file;
	GHashTable *gSymbolTable;
	GList *localSymbolTables;
	GList *functions;
	GList *hReturn;
	bool pScan;
	long currentAddr;
	int reg;
	int fCounter;
	char *array_id;
	bool error;
	int errorCount;
	int level;
	char curr_id[256];	
} parser;

void calleeEnd(bool rP);
bool expression(scanner *scn,struct fsm_object *obj,data_t *dt);
bool statement(scanner *scn,struct fsm_object *obj);
bool if_declare(scanner *scn,struct fsm_object *obj);
bool function_declare(scanner *scn, struct fsm_object *obj, int addr, data_t dt, bool global);
bool findSymbol(char *id, bool *global, symbol **tSymbol);
//symbol_t *global_declare(scanner *scn,struct fsm_object *obj, bool *isFunction);
//symbol_t *int_declare(scanner *scn,struct fsm_object *obj, bool *isFunction);
//symbol_t *float_declare(scanner *scn,struct fsm_object *obj, bool *isFunction);
//symbol_t *string_declare(scanner *scn,struct fsm_object *obj, bool *isFunction);
//symbol_t *bool_declare(scanner *scn,struct fsm_object *obj, bool *isFunction);
GList *parameter_list(scanner *scn,struct fsm_object *obj, int addr);

