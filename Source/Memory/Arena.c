#include "Memory/Arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Result ArenaBlockNew(ArenaBlock** block, usz elemSizeBytes)
{
	usz blockSize = sizeof(ArenaBlock) + (elemSizeBytes * ARENA_BLOCK_DEFAULT_ELEM_CAPACITY);

	*block = malloc(blockSize);

	if (!*block) {
		return ResErr;
	}

	(*block)->Next = nullptr;
	(*block)->ElemCapacity = ARENA_BLOCK_DEFAULT_ELEM_CAPACITY;
	(*block)->ElemNext = 0;

	// printf("New block! capacity = %lu addr = %p\n", (*block)->ElemCapacity, (void*)*block);

	return ResOk;
}

static void ArenaBlockFreeChain(ArenaBlock* block)
{
	if (!block) {
		printf("Warn: Attempted to free a 'nullptr' block chain!\n");
	}

	while (block) {
		ArenaBlock* next = block->Next;
		free(block);
		// printf("Freed block! addr = %p\n", (void*)block);
		block = next;
	}
}

Result ArenaInit(Arena* arena, usz elemSizeBytes)
{
	if (!arena) {
		return ResErr;
	}

	if (elemSizeBytes <= 0) {
		return ResErr;
	}

	arena->ElemSizeBytes = elemSizeBytes;
	arena->BlockCount = 1;

	return ArenaBlockNew(&arena->Blocks, elemSizeBytes);
}

Result ArenaMarkSet(Arena* arena, ArenaMark* mark)
{
	if (!arena || !mark) {
		return ResErr;
	}

	ArenaBlock* tail = arena->Blocks;
	// An arena will always have at least one block with free memory available, so this is safe
	while (tail->Next) {
		tail = tail->Next;
	}

	if (tail->ElemNext >= tail->ElemCapacity) {
		Result result = ArenaBlockNew(&tail->Next, arena->ElemSizeBytes);
		if (result) {
			return result;
		}

		tail = tail->Next;
	}

	mark->Blocks = tail;
	mark->ElemMark = tail->ElemNext;

	// printf("Mark set! block addr = %p, elem mark = %lu\n", (void*)mark->Blocks, mark->ElemMark);

	return ResOk;
}

Result ArenaMarkUndo(Arena* arena, ArenaMark* mark)
{
	if (!arena || !mark) {
		return ResErr;
	}

	ArenaBlockFreeChain(mark->Blocks->Next);

	mark->Blocks->ElemNext = mark->ElemMark;
	mark->Blocks->Next = nullptr;

	// printf("Mark undid! new arena tail = %p, new elem next = %lu\n", (void*)mark->Blocks, mark->Blocks->ElemNext);

	return ResOk;
}

Result ArenaAlloc(Arena* arena, void** buffer)
{
	if (!arena) {
		return ResErr;
	}

	ArenaBlock* tail = arena->Blocks;
	// An arena will always have at least one block with free memory available, so this is safe
	while (tail->Next) {
		tail = tail->Next;
	}

	if (tail->ElemNext < tail->ElemCapacity) {
		*buffer = tail->Data + (tail->ElemNext * arena->ElemSizeBytes);
		++tail->ElemNext;

		// printf("Allocation! new elem next = %lu, from block = %p\n", tail->ElemNext, (void*)tail);
		return ResOk;
	}

	Result result = ArenaBlockNew(&tail->Next, arena->ElemSizeBytes);
	if (result) {
		return result;
	}

	tail = tail->Next;
	*buffer = tail->Data + (tail->ElemNext * arena->ElemSizeBytes);
	++tail->ElemNext;
	// printf("Allocation! new elem next = %lu, from block = %p\n", tail->ElemNext, (void*)tail);

	return ResOk;
}

Result ArenaAllocZeroed(Arena* arena, void** buffer)
{
	Result result = ArenaAlloc(arena, buffer);
	if (result) {
		return result;
	}

	memset(*buffer, 0, arena->ElemSizeBytes);
	return ResOk;
}

Result ArenaDeinit(Arena* arena)
{
	if (!arena) {
		return ResErr;
	}

	ArenaBlockFreeChain(arena->Blocks);

	arena->Blocks = nullptr;
	arena->BlockCount = 0;
	arena->ElemSizeBytes = 0;

	return ResOk;
}
