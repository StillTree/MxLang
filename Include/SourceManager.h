#pragma once

#include "Types.h"
#include "Result.h"

typedef struct Source {
	const char* FileName;
	const char* Source;
	usz SourceLength;
	const char** Lines;
	usz LineCount;
} Source;

Result SourceInit(const char* name);
void SourceDeinit();

extern Source g_source;
