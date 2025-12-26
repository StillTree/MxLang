#pragma once

#include "Memory/DynArena.h"
#include "Memory/StatArena.h"
#include "Result.h"

typedef struct Interpreter {
	DynArena DataArena;
	StatArena VarTableArena;
} Interpreter;

Result InterpreterInit();
Result InterpreterInterpret();
Result InterpreterDeinit();
