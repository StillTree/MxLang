/*
MxLang - Memory/StatArena.h
Autor: Alexander Dębowski (293472)
Data: 20.12.2025
*/

#pragma once

#include "Types.h"
#include "Result.h"

typedef struct StatArenaBlock {
	struct StatArenaBlock* NextBlock;
	struct StatArenaBlock* PrevBlock;
	usz CapacityBytes;
	u8* NextBytes;
	alignas(8) u8 Data[];
} StatArenaBlock;

typedef struct StatArenaIter {
	void* Item;
	StatArenaBlock* CurBlock;
} StatArenaIter;

typedef struct StatArenaMark {
	StatArenaBlock* Blocks;
	// "Points" to the first block
	u8* ByteMark;
} StatArenaMark;

typedef struct StatArena {
	StatArenaBlock* Blocks;
	usz ItemSizeBytes;
} StatArena;

Result StatArenaInit(StatArena* arena, usz itemSizeBytes);
Result StatArenaAlloc(StatArena* arena, void** buffer);
Result StatArenaAllocZeroed(StatArena* arena, void** buffer);
Result StatArenaMarkSet(StatArena* arena, StatArenaMark* mark);
Result StatArenaMarkUndo(StatArena* arena, StatArenaMark* mark);
Result StatArenaIterNext(StatArena* arena, StatArenaIter* iter);
Result StatArenaIterBack(StatArena* arena, StatArenaIter* iter);
Result StatArenaDeinit(StatArena* arena);
