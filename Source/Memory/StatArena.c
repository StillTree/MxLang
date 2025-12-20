#include "Memory/StatArena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Result StatArenaBlockNew(StatArenaBlock** block, usz itemSizeBytes)
{
	usz capacityBytes = itemSizeBytes * 64;
	usz blockSizeBytes = sizeof(StatArenaBlock) + capacityBytes;

	*block = malloc(blockSizeBytes);

	if (!*block) {
		return ResOutOfMemory;
	}

	(*block)->NextBlock = nullptr;
	(*block)->PrevBlock = nullptr;
	(*block)->CapacityBytes = capacityBytes;
	(*block)->NextBytes = (*block)->Data;

	// printf("New block! capacity = %lu addr = %p\n", (*block)->CapacityBytes, (void*)*block);

	return ResOk;
}

static void StatArenaBlockFreeChain(StatArenaBlock* block)
{
	if (!block) {
		printf("Warn: Attempted to free a 'nullptr' block chain!\n");
	}

	while (block) {
		StatArenaBlock* next = block->NextBlock;
		free(block);
		// printf("Freed block! addr = %p\n", (void*)block);
		block = next;
	}
}

Result StatArenaInit(StatArena* arena, usz itemSizeBytes)
{
	if (!arena || itemSizeBytes > 128) {
		return ResInvalidParams;
	}

	arena->ItemSizeBytes = itemSizeBytes;

	return StatArenaBlockNew(&arena->Blocks, arena->ItemSizeBytes);
}

Result StatArenaMarkSet(StatArena* arena, StatArenaMark* mark)
{
	if (!arena || !mark) {
		return ResInvalidParams;
	}

	StatArenaBlock* tail = arena->Blocks;
	// An arena will always have at least one block with free memory available, so this is safe
	while (tail->NextBlock) {
		tail = tail->NextBlock;
	}

	if (tail->NextBytes >= tail->Data + tail->CapacityBytes) {
		Result result = StatArenaBlockNew(&tail->NextBlock, arena->ItemSizeBytes);
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

Result StatArenaMarkUndo(StatArena* arena, StatArenaMark* mark)
{
	if (!arena || !mark) {
		return ResInvalidParams;
	}

	if (mark->Blocks->NextBlock) {
		StatArenaBlockFreeChain(mark->Blocks->NextBlock);
		mark->Blocks->NextBlock = nullptr;
	}

	mark->Blocks->NextBytes = mark->ByteMark;

	// printf("Mark undid! new arena tail = %p, bytes next = %p\n", (void*)mark->Blocks, mark->Blocks->NextBytes);

	return ResOk;
}

Result StatArenaAlloc(StatArena* arena, void** buffer)
{
	if (!arena || !buffer) {
		return ResInvalidParams;
	}

	StatArenaBlock* tail = arena->Blocks;
	// An arena will always have at least one block with free memory available, so this is safe
	while (tail->NextBlock) {
		tail = tail->NextBlock;
	}

	if (tail->NextBytes + arena->ItemSizeBytes <= tail->Data + tail->CapacityBytes) {
		*buffer = tail->NextBytes;
		tail->NextBytes += arena->ItemSizeBytes;

		// printf("Allocation from %p! next bytes = %p, from block = %p\n", (void*)arena, tail->NextBytes, (void*)tail);
		return ResOk;
	}

	Result result = StatArenaBlockNew(&tail->NextBlock, arena->ItemSizeBytes);
	if (result) {
		return result;
	}

	tail->NextBlock->PrevBlock = tail;
	tail = tail->NextBlock;
	*buffer = tail->NextBytes;
	tail->NextBytes += arena->ItemSizeBytes;
	// printf("Allocation! next bytes = %p, from block = %p\n", tail->NextBytes, (void*)tail);

	return ResOk;
}

Result StatArenaAllocZeroed(StatArena* arena, void** buffer)
{
	Result result = StatArenaAlloc(arena, buffer);
	if (result) {
		return result;
	}

	memset(*buffer, 0, arena->ItemSizeBytes);
	return ResOk;
}

Result StatArenaIterNext(StatArena* arena, StatArenaIter* iter)
{
	if (!iter->Item) {
		if (arena->Blocks->Data >= arena->Blocks->NextBytes) {
			return ResEndOfIteration;
		}

		iter->Item = arena->Blocks->Data;
		iter->CurBlock = arena->Blocks;
		return ResOk;
	}

	if ((u8*)iter->Item + arena->ItemSizeBytes < iter->CurBlock->NextBytes) {
		iter->Item = (u8*)iter->Item + arena->ItemSizeBytes;
		return ResOk;
	}

	if (iter->CurBlock->NextBlock) {
		iter->CurBlock = iter->CurBlock->NextBlock;
		iter->Item = iter->CurBlock->Data;
		return ResOk;
	}

	return ResEndOfIteration;
}

Result StatArenaIterBack(StatArena* arena, StatArenaIter* iter)
{
	if (!iter->Item) {
		return ResEndOfIteration;
	}

	if ((u8*)iter->Item - arena->ItemSizeBytes < iter->CurBlock->Data) {
		if (!iter->CurBlock->PrevBlock) {
			return ResEndOfIteration;
		}

		iter->CurBlock = iter->CurBlock->PrevBlock;
		iter->Item = iter->CurBlock->Data + iter->CurBlock->CapacityBytes - arena->ItemSizeBytes;
		return ResOk;
	}

	iter->Item = (u8*)iter->Item - arena->ItemSizeBytes;
	return ResOk;
}

Result StatArenaDeinit(StatArena* arena)
{
	if (!arena) {
		return ResInvalidParams;
	}

	StatArenaBlockFreeChain(arena->Blocks);

	arena->Blocks = nullptr;

	return ResOk;
}
