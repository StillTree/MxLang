#pragma once

#include "Memory/DynArena.h"
#include "Result.h"

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
	BindingScope* CurScope;
} TypeChecker;

Result TypeCheckerInit();
Result TypeCheckerSymbolBind();
Result TypeCheckerDeinit();

extern TypeChecker g_typeChecker;
