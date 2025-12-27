#pragma once

#include "Memory/DynArena.h"
#include "Mx.h"
#include "Parser.h"

typedef struct Interpreter {
	DynArena MxArena;
	Mx** VarTable;
} Interpreter;

Mx* InterpreterAllocMx(usz height, usz width);
Mx* InterpreterEval(ASTNode* node);

void InterpreterInit();
void InterpreterInterpret();
void InterpreterDeinit();
