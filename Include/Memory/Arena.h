#pragma once

#include "Types.h"
#include "Result.h"

constexpr usz ARENA_BLOCK_DEFAULT_ELEM_CAPACITY = 4;

typedef struct ArenaBlock {
	struct ArenaBlock* Next;
	usz ElemCapacity;
	usz ElemNext;
	u8 Data[];
} ArenaBlock;

typedef struct ArenaMark {
	ArenaBlock* Blocks;
	// "Points" to the first block
	usz ElemMark;
} ArenaMark;

typedef struct Arena {
	ArenaBlock* Blocks;
	usz BlockCount;
	usz ElemSizeBytes;
} Arena;

Result ArenaInit(Arena* arena, usz elemSizeBytes);
Result ArenaAlloc(Arena* arena, void** buffer);
Result ArenaAllocZeroed(Arena* arena, void** buffer);
Result ArenaMarkSet(Arena* arena, ArenaMark* mark);
Result ArenaMarkUndo(Arena* arena, ArenaMark* mark);
Result ArenaDeinit(Arena* arena);
