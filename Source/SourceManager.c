/*
MxLang - SourceManager.c
Autor: Alexander Dębowski (293472)
Data: 27.12.2025
*/

#include "SourceManager.h"

#include "Diagnostics.h"
#include <stdio.h>
#include <stdlib.h>

Source g_source = { 0 };

static Result ReadEntireFile(const char* filePath, const char** contents, usz* length)
{
	FILE* file = fopen(filePath, "rb");

	if (!file) {
		return ResCouldNotOpenFile;
	}

	fseek(file, 0, SEEK_END);
	isz fileSize = ftell(file);

	if (fileSize < 0) {
		fclose(file);
		return ResCouldNotOpenFile;
	}

	*length = (usz)fileSize;

	fseek(file, 0, SEEK_SET);

	char* fileBuffer = malloc((usz)fileSize + 1);
	if (!fileBuffer) {
		fclose(file);
		return ResOutOfMemory;
	}

	usz readBytes = fread(fileBuffer, 1, (usz)fileSize, file);
	fileBuffer[readBytes] = '\0';

	*contents = fileBuffer;
	fclose(file);

	return ResOk;
}

void SourceInit(const char* name)
{
	DIAG_PANIC_ON_ERR(ReadEntireFile(name, &g_source.Source, &g_source.SourceLength));

	for (usz i = 0; i < g_source.SourceLength; ++i) {
		if (g_source.Source[i] != '\n') {
			continue;
		}

		++g_source.LineCount;
	}

	// The last line might not end with a \n
	++g_source.LineCount;

	g_source.Lines = (const char**)calloc(g_source.LineCount, sizeof(const char*));
	g_source.FileName = name;
}

void SourceDeinit()
{
	free((void*)g_source.Lines);
	free((void*)g_source.Source);
}
