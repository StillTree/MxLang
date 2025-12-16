#pragma once

#include "Types.h"
#include "Memory/Arena.h"

typedef struct SymbolTableEntry {
	u64 Hash;
	usz KeyLength;
	char Key[];
} SymbolTableEntry;

typedef struct SymbolTable {
	SymbolTableEntry** Entries;
	usz Capacity;
	usz EntryCount;
	Arena Arena;
} SymbolTable;


Result SymbolTableInit(SymbolTable* table);
/// Returns `ResOk` when found and `ResErr` when not.
Result SymbolTableContains(SymbolTable* table, const char* key, usz keyLength);
Result SymbolTableAdd(SymbolTable* table, const char* key, usz keyLength, const char** internedPtr);
Result SymbolTableDeinit(SymbolTable* table);
