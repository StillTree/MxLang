#pragma once

#include "Memory/StatArena.h"
#include "Types.h"

static constexpr usz MAX_DIAG_ARGS = 2;

typedef enum DiagType { DiagExpectedToken, DiagUnexpectedToken, DiagExpectedTokenAfter } DiagType;

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
	DiagArg Args[MAX_DIAG_ARGS];
} Diag;

typedef struct DiagState {
	StatArena Arena;
	StatArenaMark Mark;
} DiagState;

Result DiagInit();
void DiagEmit(DiagType type, usz sourceLine, usz sourceLinePos, const DiagArg* args, usz argCount);
void DiagReport();
Result DiagDeinit();

extern DiagState g_diagState;

#define DIAG_ARG_CHAR(x) ((DiagArg) { DiagArgChar, { .Char = (x) } })
#define DIAG_ARG_STRING(x) ((DiagArg) { DiagArgString, { .String = (x) } })

#define DIAG_EMIT0(type, sourceLine, sourceLinePos) DiagEmit((type), (sourceLine), (sourceLinePos), nullptr, 0)

#define DIAG_EMIT(type, sourceLine, sourceLinePos, ...)                                                                                    \
	do {                                                                                                                                   \
		static_assert(sizeof((const DiagArg[]) { __VA_ARGS__ }) / sizeof(DiagArg) <= MAX_DIAG_ARGS, "Too many diagnostic arguments");      \
		DiagEmit((type), (sourceLine), (sourceLinePos), (const DiagArg[]) { __VA_ARGS__ },                                                 \
			sizeof((const DiagArg[]) { __VA_ARGS__ }) / sizeof(DiagArg));                                                                  \
	} while (false)
