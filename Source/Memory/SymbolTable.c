#include "Memory/SymbolTable.h"

#include <stdlib.h>
#include <string.h>

// FNV-1a
// TOOD: Consider robin hood hashing
static u64 HashString(const char* str, usz strLength)
{
	u64 hash = 14695981039346656037UL;

	for (usz i = 0; i < strLength; ++i) {
		hash ^= (u64)str[i];
		hash *= 1099511628211UL;
	}

	return hash;
}

static Result SymbolTableExpand(SymbolTable* table)
{
	if (!table) {
		return ResInvalidParams;
	}

	usz oldCapacity = table->Capacity;
	table->Capacity *= 2;
	SymbolTableEntry** newEntries = (SymbolTableEntry**)calloc(table->Capacity, sizeof(SymbolTableEntry*));
	if (!newEntries) {
		return ResOutOfMemory;
	}

	for (usz i = 0; i < oldCapacity; ++i) {
		SymbolTableEntry* entry = table->Entries[i];
		if (!entry) {
			continue;
		}

		usz destIndex = entry->Hash & (table->Capacity - 1);

		while (newEntries[destIndex]) {
			destIndex = (destIndex + 1) & (table->Capacity - 1);
		}
		newEntries[destIndex] = entry;
	}

	free((void*)table->Entries);
	table->Entries = newEntries;

	return ResOk;
}

Result SymbolTableInit(SymbolTable* table)
{
	if (!table) {
		return ResInvalidParams;
	}

	Result result = ArenaInit(&table->Arena);
	if (result) {
		return result;
	}

	table->Capacity = 128;
	table->EntryCount = 0;

	table->Entries = (SymbolTableEntry**)calloc(table->Capacity, sizeof(SymbolTableEntry*));
	if (!table->Entries) {
		return ResErr;
	}

	return ResOk;
}

Result SymbolTableContains(SymbolTable* table, const char* key, usz keyLength)
{
	if (!table || !key) {
		return ResInvalidParams;
	}

	u64 hash = HashString(key, keyLength);
	u64 i = hash & (table->Capacity - 1);

	while (table->Entries[i]) {
		if (table->Entries[i]->Hash == hash && keyLength == table->Entries[i]->KeyLength
			&& memcmp(key, table->Entries[i]->Key, table->Entries[i]->KeyLength) == 0) {
			return ResOk;
		}

		i = (i + 1) & (table->Capacity - 1);
	}

	return ResNotFound;
}

Result SymbolTableAdd(SymbolTable* table, const char* key, usz keyLength, const char** internedPtr)
{
	if (!table || !key) {
		return ResInvalidParams;
	}

	// We expand the table when we filled it up to 75%
	if ((table->EntryCount + 1) * 100 / table->Capacity > 75) {
		Result result = SymbolTableExpand(table);
		if (result) {
			return result;
		}
	}

	u64 hash = HashString(key, keyLength);
	usz i = hash & (table->Capacity - 1);

	while (table->Entries[i]) {
		if (table->Entries[i]->Hash == hash && keyLength == table->Entries[i]->KeyLength
			&& memcmp(key, table->Entries[i]->Key, table->Entries[i]->KeyLength) == 0) {
			*internedPtr = table->Entries[i]->Key;
			return ResOk;
		}

		i = (i + 1) & (table->Capacity - 1);
	}

	SymbolTableEntry* newEntry;
	Result result = ArenaAlloc(&table->Arena, (void**)&newEntry, sizeof(SymbolTableEntry) + keyLength);
	if (result) {
		return result;
	}

	newEntry->Hash = hash;
	newEntry->KeyLength = keyLength;
	memcpy(newEntry->Key, key, keyLength);
	table->Entries[i] = newEntry;

	++table->EntryCount;

	*internedPtr = newEntry->Key;

	return ResOk;
}

Result SymbolTableDeinit(SymbolTable* table)
{
	if (!table) {
		return ResInvalidParams;
	}

	free((void*)table->Entries);
	table->Capacity = 0;
	table->EntryCount = 0;

	return ArenaDeinit(&table->Arena);
}
