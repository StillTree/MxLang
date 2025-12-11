#include "Memory/Arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Result ArenaBlockNew(ArenaBlock** block)
{
	usz blockSize = sizeof(ArenaBlock) + ARENA_BLOCK_DEFAULT_CAPACITY;

	*block = malloc(blockSize);

	if (!*block) {
		return ResOutOfMemory;
	}

	(*block)->NextBlock = nullptr;
	(*block)->CapacityBytes = blockSize;
	(*block)->NextBytes = (*block)->Data;

	// printf("New block! capacity = %lu addr = %p\n", (*block)->CapacityBytes, (void*)*block);

	return ResOk;
}

static void ArenaBlockFreeChain(ArenaBlock* block)
{
	if (!block) {
		printf("Warn: Attempted to free a 'nullptr' block chain!\n");
	}

	while (block) {
		ArenaBlock* next = block->NextBlock;
		free(block);
		// printf("Freed block! addr = %p\n", (void*)block);
		block = next;
	}
}

Result ArenaInit(Arena* arena)
{
	if (!arena) {
		return ResInvalidParams;
	}

	return ArenaBlockNew(&arena->Blocks);
}

Result ArenaMarkSet(Arena* arena, ArenaMark* mark)
{
	if (!arena || !mark) {
		return ResInvalidParams;
	}

	ArenaBlock* tail = arena->Blocks;
	// An arena will always have at least one block with free memory available, so this is safe
	while (tail->NextBlock) {
		tail = tail->NextBlock;
	}

	if (tail->NextBytes >= tail->Data + tail->CapacityBytes) {
		Result result = ArenaBlockNew(&tail->NextBlock);
		if (result) {
			return result;
		}

		tail = tail->NextBlock;
	}

	mark->Blocks = tail;
	mark->ByteMark = tail->NextBytes;

	// printf("Mark set! block addr = %p, byte mark = %p\n", (void*)mark->Blocks, mark->ByteMark);

	return ResOk;
}

Result ArenaMarkUndo(Arena* arena, ArenaMark* mark)
{
	if (!arena || !mark) {
		return ResInvalidParams;
	}

	ArenaBlockFreeChain(mark->Blocks->NextBlock);

	mark->Blocks->NextBytes = mark->ByteMark;
	mark->Blocks->NextBlock = nullptr;

	// printf("Mark undid! new arena tail = %p, bytes next = %p\n", (void*)mark->Blocks, mark->Blocks->NextBytes);

	return ResOk;
}

Result ArenaAlloc(Arena* arena, void** buffer, usz size)
{
	if (!arena || !buffer || size <= 0) {
		return ResInvalidParams;
	}

	ArenaBlock* tail = arena->Blocks;
	// An arena will always have at least one block with free memory available, so this is safe
	while (tail->NextBlock) {
		tail = tail->NextBlock;
	}

	if (tail->NextBytes + size <= tail->Data + tail->CapacityBytes) {
		*buffer = tail->NextBytes;
		tail->NextBytes += size;

		// printf("Allocation! next bytes = %p, from block = %p\n", tail->NextBytes, (void*)tail);
		return ResOk;
	}

	// TODO: Check if allocation size exceeds block size, then do something
	if (size > ARENA_BLOCK_DEFAULT_CAPACITY) {
		return ResUnimplemented;
	}

	Result result = ArenaBlockNew(&tail->NextBlock);
	if (result) {
		return result;
	}

	tail = tail->NextBlock;
	*buffer = tail->NextBytes;
	tail->NextBytes += size;
	// printf("Allocation! next bytes = %p, from block = %p\n", tail->NextBytes, (void*)tail);

	return ResOk;
}

Result ArenaAllocZeroed(Arena* arena, void** buffer, usz size)
{
	Result result = ArenaAlloc(arena, buffer, size);
	if (result) {
		return result;
	}

	memset(*buffer, 0, size);
	return ResOk;
}

Result ArenaDeinit(Arena* arena)
{
	if (!arena) {
		return ResInvalidParams;
	}

	ArenaBlockFreeChain(arena->Blocks);

	arena->Blocks = nullptr;

	return ResOk;
}
