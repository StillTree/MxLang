/*
MxLang - Main.c
Autor: Alexander Dębowski (293472)
Data: 02.02.2026
*/

#include "Diagnostics.h"
#include "Interpreter.h"
#include "Parser.h"
#include "SourceManager.h"
#include "Tokenizer.h"
#include "TypeChecker.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[])
{
	srand((u32)time(0));

	printf("MxLang v" MX_VERSION "\nAutor: Alexander Dębowski (293472)\n\n");

	if (argc < 2) {
		fprintf(stderr, "A 'fileName' argument is required\n");
		return 1;
	}

	if (argc > 2) {
		printf("Ignoring redundant arguments. Pwovided %d too many\n", argc - 2);
	}

	if (DiagInit()) {
		fprintf(stderr, "An unrecoverable internal error occured while initializing diagnostic support\n");
		return 1;
	}

	SourceInit(argv[1]);

	TokenizerInit();

	ParserInit();

	ParserParse();

	usz errCount = DiagReport();

	TypeCheckerInit();

	TypeCheckerSymbolBind();

	errCount += DiagReport();

	if (errCount <= 0) {
		TypeCheckerTypeCheck();

		errCount += DiagReport();
	} else {
		fprintf(stderr, "Error(s) emitted. Stopping now\n");
		goto deinit;
	}

	if (errCount <= 0) {
		InterpreterInit();

		InterpreterInterpret();

		InterpreterDeinit();
	} else {
		fprintf(stderr, "Error(s) emitted. Stopping now\n");
		goto deinit;
	}

deinit:
	TypeCheckerDeinit();

	ParserDeinit();

	TokenizerDeinit();

	SourceDeinit();

	if (DiagDeinit()) {
		fprintf(stderr, "An unrecoverable internal error occured while deinitializing diagnostic support\n");
		return 1;
	}

	return 0;
}
