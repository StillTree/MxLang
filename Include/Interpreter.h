/*
MxLang - Interpreter.h
Autor: Alexander Dębowski (293472)
Data: 28.12.2025
*/

#pragma once

#include "Memory/DynArena.h"
#include "Mx.h"
#include "Parser.h"

typedef struct Interpreter {
	DynArena MxArena;
	Mx** VarTable;
} Interpreter;

[[noreturn]] void InterpreterPanic();
Mx* InterpreterAllocMx(usz height, usz width);
Mx* InterpreterEval(ASTNode* node);

void InterpreterInit();
void InterpreterInterpret();
void InterpreterDeinit();
