#include "Tokenizer.h"

#include "Memory/Arena.h"

Tokenizer g_tokenizer = {0};

static char TokenizerConsume()
{
	return *g_tokenizer.LexemeCurrent++;
}

Result TokenizerInit(const char* source)
{
	Result result = ArenaInit(&g_tokenizer.ArenaTokens);
	if (result) {
		return result;
	}

	g_tokenizer.SourceLine = 1;
	g_tokenizer.Source = source;
	g_tokenizer.LexemeCurrent = source;
	g_tokenizer.LexemeStart = source;

	return ResOk;
}

Result TokenizerScan()
{
	return ResOk;
}
