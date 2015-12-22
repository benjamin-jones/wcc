
#define NUM_KEYS 14
#define NUM_OPS 20

static char *keywords[] = 
{
	"function",
	"begin",
	"end",
	"global",
	"integer",
	"float",
	"boolean",
	"string",
	"if",
	"then",
	"else",
	"for",
	"true",
	"false"
};

static char *operators[] =
{
    "+",
    "-",
    "*",
    "/",
    "(",
    ")",
    "[",
    "]",
    ",",
    "|",
    "&",
    ":=",
    "<",
    ">",
    ">=",
    "<=",
    "==",
    "!=",
    "not",
    ";"
};

static char *names[] =
{
	"T_FUNCTION",
	"T_BEGIN",
	"T_END",
	"T_GLOBAL",
	"T_INTEGER",
	"T_INTCONST",
	"T_FLOAT",
	"T_FLOATCONST",
	"T_BOOLEAN",
	"T_BOOLCONST",
	"T_STRING",
	"T_STRINGCONST",
	"T_IF",
	"T_THEN",
	"T_ELSE",
	"T_FOR",
	"T_IDENTIFIER",
	"T_ASSIGN",
	"T_GTEQ",
	"T_LTEQ",
	"T_EQUAL",
	"T_NOTEQUAL",
	"T_NOT",
	"T_EOF",
	"T_UNKNOWN"
};
 

