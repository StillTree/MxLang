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

	if (TokenizerInit()) {
		return 1;
	}
	if (TokenizerScan()) {
		return 1;
	}

	printf("Scanned!\n\n");

	DiagReport();

	if (DiagDeinit()) {
		return 1;
	}

	SourceDeinit();

	return 0;
}
