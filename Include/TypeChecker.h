#pragma once

#include "Matrix.h"
#include "Memory/DynArena.h"
#include "Memory/StatArena.h"
#include "Result.h"

typedef struct TypeCheckingEntry {
	MatrixShape Shape;
	bool IsConst;
} TypeCheckingEntry;

typedef struct BindingEntry {
	const char* InternedName;
	usz ID;
} BindingEntry;

typedef struct BindingScope {
	struct BindingScope* Parent;
	usz EntryCount;
	BindingEntry Entries[];
} BindingScope;

typedef struct TypeChecker {
	DynArena BindingArena;
	BindingScope* CurBindingScope;
	TypeCheckingEntry* TypeCheckingTable;
	StatArena ShapeArena;
} TypeChecker;

Result TypeCheckerInit();
Result TypeCheckerSymbolBind();
void TypeCheckerTypeCheck();
Result TypeCheckerDeinit();

extern TypeChecker g_typeChecker;
