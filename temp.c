#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "runtime.h"

int main()
{
	goto _main;

getBool:
	R[0] = (int)getBool();
	R[1] = MM[R[SP]];
	MM[R[SP]] = R[0];
	goto *(void*)R[1];
getInt:
	R[0] = getInt();
	R[1] = MM[R[SP]];
	MM[R[SP]] = R[0];
	goto *(void*)R[1];
getString:
	getString(tmpString);
	R[0] = (int)tmpString;
	R[1] = MM[R[SP]];
	MM[R[SP]] = R[0];
	goto *(void*)R[1];
getFloat:
	tmpFloat = getFloat();
	memcpy(&R[0], &tmpFloat, sizeof(float));
	R[1] = MM[R[SP]];
	MM[R[SP]] = R[0];
	goto *(void*)R[1];
putBool:
	R[0] = MM[R[SP] - 1];
	R[1] = putBool((bool)R[0]);
	R[2] = MM[R[SP]];
	MM[R[SP]] = R[1];
	goto *(void*)R[2];
putInt:
	R[0] = MM[R[SP] - 1];
	R[1] = putInt(R[0]);
	R[2] = MM[R[SP]];
	MM[R[SP]] = R[1];
	goto *(void*)R[2];
sqrt:
	R[0] = MM[R[SP] - 1];
	tmpFloat = sqrt(R[0]);
	memcpy(&R[1],&tmpFloat,sizeof(float));
	R[2] = MM[R[SP]];
	MM[R[SP]] = R[1];
	goto *(void*)R[2];
putString:
	R[0] = MM[R[SP] - 1];
	R[1] = putString((char *)R[0]);
	R[2] = MM[R[SP]];
	MM[R[SP]] = R[1];
	goto *(void*)R[2];
putFloat:
	R[0] = MM[R[SP] - 1];
	memcpy(&tmpFloat, &R[0], sizeof(float));
	R[1] = putFloat(tmpFloat);
	R[2] = MM[R[SP]];
	MM[R[SP]] = R[1];
	goto *(void*)R[2];
_main:
	R[0] = 1;
	MM[R[0]] = (int)&&getInt;
	R[0] = 4;
	MM[R[0]] = (int)&&putBool;
	R[0] = 8;
	MM[R[0]] = (int)&&putFloat;
	R[0] = 3;
	MM[R[0]] = (int)&&getFloat;
	R[0] = 7;
	MM[R[0]] = (int)&&putString;
	R[0] = 0;
	MM[R[0]] = (int)&&getBool;
	R[0] = 5;
	MM[R[0]] = (int)&&putInt;
	R[0] = 6;
	MM[R[0]] = (int)&&sqrt;
	R[0] = 2;
	MM[R[0]] = (int)&&getString;
	R[SP] = STACK_START;
	R[FP] = STACK_START;
	MM[R[SP]] = (int)&&_end;
	goto main;
_end:
	return MM[R[SP]];
}
