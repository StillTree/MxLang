#pragma once

#include "Types.h"
#include "Result.h"

static constexpr usz DYN_ARENA_BLOCK_DEFAULT_CAPACITY = 2048;

typedef struct DynArenaBlock {
	struct DynArenaBlock* NextBlock;
	usz CapacityBytes;
	u8* NextBytes;
	alignas(8) u8 Data[];
} DynArenaBlock;

typedef struct DynArenaMark {
	DynArenaBlock* Blocks;
	// "Points" to the first block
	u8* ByteMark;
} DynArenaMark;

typedef struct DynArena {
	DynArenaBlock* Blocks;
} DynArena;

Result DynArenaInit(DynArena* arena);
Result DynArenaAlloc(DynArena* arena, void** buffer, usz size);
Result DynArenaAllocZeroed(DynArena* arena, void** buffer, usz size);
Result DynArenaMarkSet(DynArena* arena, DynArenaMark* mark);
Result DynArenaMarkUndo(DynArena* arena, DynArenaMark* mark);
Result DynArenaDeinit(DynArena* arena);
