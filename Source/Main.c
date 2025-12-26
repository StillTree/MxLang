#include "Diagnostics.h"
#include "Interpreter.h"
#include "Parser.h"
#include "SourceManager.h"
#include "Tokenizer.h"
#include "TypeChecker.h"
#include <stdio.h>

int main(int argc, char* argv[])
{
	printf("MxLang v" MX_VERSION "\n\n");

	if (argc < 2) {
		// TODO: Report errors meaningfully
		return 1;
	}

	if (SourceInit(argv[1])) {
		return 1;
	}

	printf("File contents:\n%.*s\n", (u32)g_source.SourceLength, g_source.Source);

	if (DiagInit()) {
		return 1;
	}

	if (TokenizerInit()) {
		return 1;
	}

	if (ParserInit()) {
		return 1;
	}

	if (TypeCheckerInit()) {
		return 1;
	}

	if (ParserParse()) {
		return 1;
	}

	usz errCount = DiagReport();
	if (errCount == 0) { }

	printf("Parsed!\n\n");

	ParserPrintAST((ASTNode*)g_parser.ASTArena.Blocks->Data, 0);
	printf("\n\n");

	if (TypeCheckerSymbolBind()) {
		return 1;
	}

	errCount += DiagReport();
	if (errCount == 0) { }

	printf("Bound symbols!\n\n");

	if (errCount == 0) {
		TypeCheckerTypeCheck();

		errCount += DiagReport();
		if (errCount == 0) { }

		printf("Type checked!\n\n");
	}

	if (TypeCheckerDeinit()) {
		return 1;
	}

	if (InterpreterInit()) {
		return 1;
	}

	if (errCount == 0) {
		InterpreterInterpret();

		errCount += DiagReport();
		if (errCount == 0) { }

		printf("Interpreted!\n\n");
	}

	if (InterpreterDeinit()) {
		return 1;
	}

	if (ParserDeinit()) {
		return 1;
	}

	if (TokenizerDeinit()) {
		return 1;
	}

	if (DiagDeinit()) {
		return 1;
	}

	SourceDeinit();

	return 0;
}
