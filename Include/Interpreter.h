#pragma once

#include "Memory/DynArena.h"
#include "Mx.h"
#include "Parser.h"
#include "Result.h"

typedef struct Interpreter {
	DynArena MxArena;
	Mx** VarTable;
} Interpreter;

Mx* InterpreterAllocMx(usz height, usz width);
Mx* InterpreterEval(ASTNode* node);

Result InterpreterInit();
void InterpreterInterpret();
Result InterpreterDeinit();
