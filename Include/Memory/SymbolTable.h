#pragma once

#include "Memory/DynArena.h"
#include "Types.h"

typedef struct SymbolView {
	usz SymbolLength;
	const char* Symbol;
} SymbolView;

typedef struct SymbolTableEntry {
	u64 Hash;
	usz KeyLength;
	char Key[];
} SymbolTableEntry;

typedef struct SymbolTable {
	SymbolTableEntry** Entries;
	usz Capacity;
	usz EntryCount;
	DynArena Arena;
} SymbolTable;

Result SymbolTableInit(SymbolTable* table);
Result SymbolTableContains(SymbolTable* table, const char* key, usz keyLength);
Result SymbolTableAdd(SymbolTable* table, const char* key, usz keyLength, SymbolView* internedSymbol);
Result SymbolTableDeinit(SymbolTable* table);
