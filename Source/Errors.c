#include "Errors.h"

#include "SourceManager.h"
#include <stdio.h>

static const char* const DIAG_LEVEL_STR[] = { "Note", "Warning", "Error" };

static const char* const DIAG_TYPE_STR[] = { "Unexpected token '%c'" };

static usz NumberWidth(usz num)
{
	usz width = 0;

	while (num > 0) {
		num /= 10;
		++width;
	}

	return width;
}

void PrintDiagnostic(const Diagnostic* diag)
{
	FILE* out = diag->Level == Error ? stderr : stdout;

	fprintf(out, "%s:%zu:%zu %s: ", g_source.FileName, diag->SourceLine, diag->SourceLinePos, DIAG_LEVEL_STR[diag->Level]);

	fprintf(out, DIAG_TYPE_STR[diag->Type], 'c');
	fputc('\n', out);

	usz numWidth = NumberWidth(diag->SourceLine);

	for (usz i = 0; i < numWidth; ++i) {
		fputc(' ', out);
	}
	fprintf(out, " |\n");

	fprintf(out, "%zu | ", diag->SourceLine);

	const char* iter = g_source.Lines[diag->SourceLine - 1];
	while (*iter != '\n') {
		fputc(*iter, out);
		++iter;
	}
	fputc('\n', out);

	for (usz i = 0; i < numWidth; ++i) {
		fputc(' ', out);
	}
	fprintf(out, " | ");

	for (usz i = 0; i < diag->SourceLinePos - 1; ++i) {
		fputc(' ', out);
	}
	fprintf(out, "^\n");
}
