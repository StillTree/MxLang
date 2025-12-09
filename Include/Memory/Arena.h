#pragma once

#include "Types.h"
#include "Result.h"

constexpr usz ARENA_BLOCK_DEFAULT_CAPACITY = 1024;

typedef struct ArenaBlock {
	struct ArenaBlock* NextBlock;
	usz CapacityBytes;
	u8* NextBytes;
	u8 Data[];
} ArenaBlock;

typedef struct ArenaMark {
	ArenaBlock* Blocks;
	// "Points" to the first block
	u8* ByteMark;
} ArenaMark;

typedef struct Arena {
	ArenaBlock* Blocks;
} Arena;

Result ArenaInit(Arena* arena);
Result ArenaAlloc(Arena* arena, void** buffer, usz size);
Result ArenaAllocZeroed(Arena* arena, void** buffer, usz size);
Result ArenaMarkSet(Arena* arena, ArenaMark* mark);
Result ArenaMarkUndo(Arena* arena, ArenaMark* mark);
Result ArenaDeinit(Arena* arena);
