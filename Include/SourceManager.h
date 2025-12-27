#pragma once

#include "Types.h"

typedef struct Source {
	const char* FileName;
	const char* Source;
	usz SourceLength;
	const char** Lines;
	usz LineCount;
} Source;

void SourceInit(const char* name);
void SourceDeinit();

extern Source g_source;
