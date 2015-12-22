#include <glib.h>

typedef enum  
{
	INTEGER = 0x0,
	BOOLEAN = 0x01,
	FLOAT = 0x02,
	STRING = 0x04
} data_t;

typedef enum 
{
	VARIABLE,
	ARRAY,
	FUNCTION
} structure_t;

typedef struct 
{
	structure_t st;
	data_t dt;
	int size;
	GList *params;
} symbol_t;

typedef struct  
{
	char* id;
	symbol_t st;
	long addr;
} symbol;
