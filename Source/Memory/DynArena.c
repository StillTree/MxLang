#include "Memory/DynArena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Result DynArenaBlockNew(DynArenaBlock** block)
{
	usz blockSize = sizeof(DynArenaBlock) + DYN_ARENA_BLOCK_DEFAULT_CAPACITY;

	*block = malloc(blockSize);

	if (!*block) {
		return ResOutOfMemory;
	}

	(*block)->NextBlock = nullptr;
	(*block)->CapacityBytes = DYN_ARENA_BLOCK_DEFAULT_CAPACITY;
	(*block)->NextBytes = (*block)->Data;

	// printf("New block! capacity = %lu addr = %p\n", (*block)->CapacityBytes, (void*)*block);

	return ResOk;
}

static void DynArenaBlockFreeChain(DynArenaBlock* block)
{
	if (!block) {
		printf("Warn: Attempted to free a 'nullptr' block chain!\n");
	}

	while (block) {
		DynArenaBlock* next = block->NextBlock;
		free(block);
		// printf("Freed block! addr = %p\n", (void*)block);
		block = next;
	}
}

Result DynArenaInit(DynArena* arena)
{
	if (!arena) {
		return ResInvalidParams;
	}

	return DynArenaBlockNew(&arena->Blocks);
}

Result DynArenaMarkSet(DynArena* arena, DynArenaMark* mark)
{
	if (!arena || !mark) {
		return ResInvalidParams;
	}

	DynArenaBlock* tail = arena->Blocks;
	// An arena will always have at least one block with free memory available, so this is safe
	while (tail->NextBlock) {
		tail = tail->NextBlock;
	}

	if (tail->NextBytes >= tail->Data + tail->CapacityBytes) {
		Result result = DynArenaBlockNew(&tail->NextBlock);
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

Result DynArenaMarkUndo(DynArena* arena, DynArenaMark* mark)
{
	if (!arena || !mark) {
		return ResInvalidParams;
	}

	if (mark->Blocks->NextBlock) {
		DynArenaBlockFreeChain(mark->Blocks->NextBlock);
		mark->Blocks->NextBlock = nullptr;
	}

	mark->Blocks->NextBytes = mark->ByteMark;

	// printf("Mark undid! new arena tail = %p, bytes next = %p\n", (void*)mark->Blocks, mark->Blocks->NextBytes);

	return ResOk;
}

Result DynArenaAlloc(DynArena* arena, void** buffer, usz size)
{
	if (!arena || !buffer || size <= 0) {
		return ResInvalidParams;
	}

	DynArenaBlock* tail = arena->Blocks;
	// An arena will always have at least one block with free memory available, so this is safe
	while (tail->NextBlock) {
		tail = tail->NextBlock;
	}

	if (tail->NextBytes + size <= tail->Data + tail->CapacityBytes) {
		*buffer = tail->NextBytes;
		tail->NextBytes += size;

		// printf("Allocation from %p! next bytes = %p, from block = %p\n", (void*)arena, tail->NextBytes, (void*)tail);
		return ResOk;
	}

	// TODO: Check if allocation size exceeds block size, then do something
	if (size > DYN_ARENA_BLOCK_DEFAULT_CAPACITY) {
		return ResUnimplemented;
	}

	Result result = DynArenaBlockNew(&tail->NextBlock);
	if (result) {
		return result;
	}

	tail = tail->NextBlock;
	*buffer = tail->NextBytes;
	tail->NextBytes += size;
	// printf("Allocation! next bytes = %p, from block = %p\n", tail->NextBytes, (void*)tail);

	return ResOk;
}

Result DynArenaAllocZeroed(DynArena* arena, void** buffer, usz size)
{
	Result result = DynArenaAlloc(arena, buffer, size);
	if (result) {
		return result;
	}

	memset(*buffer, 0, size);
	return ResOk;
}

Result DynArenaDeinit(DynArena* arena)
{
	if (!arena) {
		return ResInvalidParams;
	}

	DynArenaBlockFreeChain(arena->Blocks);

	arena->Blocks = nullptr;

	return ResOk;
}
