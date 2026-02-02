/*
MxLang - SourceManager.h
Autor: Alexander Dębowski (293472)
Data: 27.12.2025
*/

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
