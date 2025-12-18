#include "Diagnostics.h"
#include "SourceManager.h"
#include "Tokenizer.h"
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

	TokenizerInit();
	TokenizerScan();

	printf("Scanned!\n\n");

	// StatArenaIter i;
	// while (!StatArenaIterate(&g_tokenizer.ArenaTokens, &i)) {
	// 	Token* t = i.Item;

	// 	printf("Token %u at %lu:%lu", t->Type, t->SourceLine, t->SourceLinePos);

	// 	if (t->Type == TokenNumber) {
	// 		printf(" %lf", t->Number);
	// 	} else if (t->Type == TokenIdentifier) {
	// 		printf(" '%.*s'", (i32)t->Lexeme.SymbolLength, t->Lexeme.Symbol);
	// 	}

	// 	printf("\n");
	// }

	DiagEmit(DiagUnexpectedToken, 11, 5, 1, DIAG_ARG_CHAR('i'));

	DiagReport();

	if (DiagDeinit()) {
		return 1;
	}

	SourceDeinit();

	return 0;
}
