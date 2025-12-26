#pragma once

#include "Memory/DynArena.h"
#include "Mx.h"
#include "Result.h"

typedef struct Interpreter {
	DynArena MxArena;
	Mx** VarTable;
} Interpreter;

Result InterpreterInit();
void InterpreterInterpret();
Result InterpreterDeinit();
