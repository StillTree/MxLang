#include <stdio.h>
#include <stdlib.h>
#include "Result.h"
#include "Tokenizer.h"

Result ReadEntireFile(const char* filePath, const char** contents);

int main(int argc, char* argv[])
{
	printf("MxLang v" MX_VERSION "\n\n");

	if (argc < 2) {
		// TODO: Report errors meaningfully
		return -1;
	}

	const char* sourceCode;
	Result result = ReadEntireFile(argv[1], &sourceCode);
	if (result) {
		// TODO: Report errors meaningfully
		return -1;
	}

	printf("File contents:\n%s\n", sourceCode);

	TokenizerInit(sourceCode);
	TokenizerScan();

	printf("Scanned!\n\n");

	Token* t = (Token*)g_tokenizer.ArenaTokens.Blocks->Data;
	while (t < (Token*)g_tokenizer.ArenaTokens.Blocks->NextBytes) {
		printf("Token! %lu:%lu\n", t->SourceLine, t->SourceLinePos);

		if (t->Type == TokenNumber) {
			printf("Token!: type %u %lf\n", t->Type, t->Number);
		} else if (t->Type == TokenIdentifier) {
			printf("Token!: type %u '%.*s'\n", t->Type, 1, t->Lexeme);
		}
		++t;
	}

	free((void*)sourceCode);

	return 0;
}

Result ReadEntireFile(const char* filePath, const char** contents)
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

	fseek(file, 0, SEEK_SET);

	char* fileBuffer = malloc((usz)fileSize + 1);
	if (!fileBuffer) {
		fclose(file);
		return ResOutOfMemory;
	}

	usz readSize = fread(fileBuffer, 1, (usz)fileSize, file);
	fileBuffer[readSize] = '\0';

	*contents = fileBuffer;
	fclose(file);

	return ResOk;
}
