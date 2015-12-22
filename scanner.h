/* Provides typedefs and func declarations for the lexer */

#define MAX_ID 256
#include <glib.h>

typedef enum bool
{
	false,
   	true
} bool;

unsigned int getTokenType(char *str);

typedef enum token_t 
{
	T_FUNCTION,
	T_BEGIN,
	T_END,
	T_GLOBAL,
	T_INTEGER,
	T_INTCONST,
	T_FLOAT,
	T_FLOATCONST,
	T_BOOLEAN,
	T_BOOLCONST,
	T_STRING,
	T_STRINGCONST,
	T_IF,
	T_THEN,
	T_ELSE,
	T_FOR,
	T_IDENTIFIER,
	T_ASSIGN,
	T_GTEQ,
	T_LTEQ,
	T_EQUAL,
	T_NOTEQUAL,
	T_NOT,
	T_EOF,
	T_UNKNOWN,
	T_PLUS = '+',
	T_MINUS = '-',
	T_STAR = '*',
	T_DIVIDE = '/',
	T_LPAREN = '(',
	T_RPAREN = ')',
	T_LBOX = '[',
	T_RBOX = ']',
	T_COMMA = ',',
	T_OR = '|',
	T_AND = '&',
	T_LESSTHAN = '<',
	T_GREATTHAN = '>',
	T_SEMICOLON = ';'

} token_t;		    

typedef enum report_t
{
	INFO,
	WARN,
	ERROR
} report_t;

typedef struct token
{
	token_t tid;
    	union {
    		char string[256];
    		int intVal;
		bool boolVal;
		double doubleVal;
    	} value; 
} token;


typedef struct scanner
{
    	token currentToken;
	unsigned int curr_line;
    	FILE *fp;
	char buf[MAX_ID];
	char *bufLoc;
	GHashTable *table;
} scanner;

//void report(char *str, report_t type);
void initTable(scanner *scn);
bool init(int argc, char **argv, scanner *scn,bool *gDebug);
void start(struct fsm_object *obj, int val, void **arg);
void floatl(struct fsm_object *obj, int val, void **arg);
void number(struct fsm_object *obj, int val, void **arg);
void alpha(struct fsm_object *obj, int val, void **arg);
void id(struct fsm_object *obj, int val, void **arg);


