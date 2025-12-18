#pragma once

#include "Memory/StatArena.h"
#include "Types.h"

typedef enum DiagType {
	DiagUnexpectedToken
} DiagType;

typedef enum DiagArgType {
	DiagArgChar,
	DiagArgString,
} DiagArgType;

typedef struct DiagArg {
	DiagArgType Type;
	union {
		char Char;
		const char* String;
	};
} DiagArg;

typedef struct Diag {
	DiagType Type;
	usz SourceLine;
	usz SourceLinePos;
	DiagArg Args[1];
} Diag;

typedef struct DiagState {
	StatArena Arena;
	StatArenaMark Mark;
} DiagState;

extern DiagState g_diagState;

Result DiagInit();
void DiagEmit(DiagType type, usz sourceLine, usz sourceLinePos, usz argCount, ...);
void DiagReport();
Result DiagDeinit();

#define DIAG_ARG_CHAR(x) ((DiagArg){ DiagArgChar, { .Char = (x) } })
#define DIAG_ARG_STRING(x) ((DiagArg){ DiagArgString, { .String = (x) } })
