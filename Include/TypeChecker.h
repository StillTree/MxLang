#pragma once

#include "MxShape.h"
#include "Memory/DynArena.h"
#include "Memory/StatArena.h"

typedef struct TypeCheckingEntry {
	MxShape Shape;
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

void TypeCheckerInit();
void TypeCheckerSymbolBind();
void TypeCheckerTypeCheck();
void TypeCheckerDeinit();

extern TypeChecker g_typeChecker;
