#include "Diagnostics.h"

#include "SourceManager.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

DiagState g_diagState = { 0 };

typedef enum DiagLevel { DiagLevelNote, DiagLevelWarning, DiagLevelError } DiagLevel;

static const char* const DIAG_LEVEL_STR[] = { "Note", "Warning", "Error" };

typedef struct DiagInfo {
	DiagLevel Level;
	const char* Format;
} DiagInfo;

static const DiagInfo DIAG_TYPE_INFO[] = { [DiagExpectedToken] = { DiagLevelError, "Expected token '%0'" },
	[DiagUnexpectedToken] = { DiagLevelError, "Unexpected token '%0'" },
	[DiagExpectedTokenAfter] = { DiagLevelError, "Expected '%0' after '%1'" } };

static usz NumberWidth(usz num)
{
	usz width = 0;

	while (num > 0) {
		num /= 10;
		++width;
	}

	return width;
}

void PrintDiagFormat(FILE* out, const char* format, const Diag* diag)
{
	const char* iter = format;
	while (*iter) {
		if (*iter == '%' && iter[1] >= '0' && iter[1] <= '9') {
			usz i = (usz)(iter[1] - '0');

			switch (diag->Args[i].Type) {
			case DiagArgChar:
				fputc(diag->Args[i].Char, out);
				break;
			case DiagArgString:
				fputs(diag->Args[i].String, out);
				break;
			}

			++iter;
		} else {
			fputc(*iter, out);
		}

		++iter;
	}
}

void DiagPrint(const Diag* diag)
{
	const DiagInfo* info = DIAG_TYPE_INFO + diag->Type;

	FILE* out = info->Level == DiagLevelError ? stderr : stdout;

	fprintf(out, "%s:%zu:%zu %s: ", g_source.FileName, diag->SourceLine, diag->SourceLinePos, DIAG_LEVEL_STR[info->Level]);
	PrintDiagFormat(out, info->Format, diag);

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
	fprintf(out, "^\n\n");
}

Result DiagInit()
{
	Result result = StatArenaInit(&g_diagState.Arena, sizeof(Diag));
	if (result) {
		return result;
	}

	result = StatArenaMarkSet(&g_diagState.Arena, &g_diagState.Mark);
	if (result) {
		StatArenaDeinit(&g_diagState.Arena);
		return result;
	}

	return ResOk;
}

void DiagEmit(DiagType type, usz sourceLine, usz sourceLinePos, const DiagArg* args, usz argCount)
{
	Diag* diag = nullptr;
	assert(StatArenaAlloc(&g_diagState.Arena, (void**)&diag) == ResOk);

	diag->Type = type;
	diag->SourceLine = sourceLine;
	diag->SourceLinePos = sourceLinePos;
	for (usz i = 0; i < argCount; ++i) {
		diag->Args[i] = args[i];
	}
}

void DiagReport()
{
	StatArenaIter iter = { 0 };
	while (StatArenaIterNext(&g_diagState.Arena, &iter) == ResOk) {
		DiagPrint(iter.Item);
	}

	assert(StatArenaMarkUndo(&g_diagState.Arena, &g_diagState.Mark) == ResOk);

	assert(StatArenaMarkSet(&g_diagState.Arena, &g_diagState.Mark) == ResOk);
}

Result DiagDeinit() { return StatArenaDeinit(&g_diagState.Arena); }
