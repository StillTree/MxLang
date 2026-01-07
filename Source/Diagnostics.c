#include "Diagnostics.h"

#include "SourceManager.h"
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

DiagState g_diagState = { 0 };

typedef enum DiagLevel { DiagLevelNote, DiagLevelWarning, DiagLevelError } DiagLevel;

static const char* const DIAG_LEVEL_STR[] = { "\033[1;94mNote", "\033[1;33mWarning", "\033[1;31mError" };

typedef struct DiagInfo {
	DiagLevel Level;
	const char* Format;
} DiagInfo;

static const DiagInfo DIAG_TYPE_INFO[] = { [DiagExpectedToken] = { DiagLevelError, "Expected token %0" },
	[DiagExpectedIdentifier] = { DiagLevelError, "Expected identifier" },
	[DiagUnexpectedToken] = { DiagLevelError, "Unexpected token %0" },
	[DiagExpectedTokenAfter] = { DiagLevelError, "Expected token %0 after %1" },
	[DiagEmptyMxLiteralsNotAllowed] = { DiagLevelError, "Empty matrix literals are not allowed" },
	[DiagEmptyVecLiteralsNotAllowed] = { DiagLevelError, "Empty vector literals are not allowed" },
	[DiagRedeclarationInScope] = { DiagLevelError, "Variable redeclaration in current scope" },
	[DiagUndeclaredVarUsed] = { DiagLevelError, "Undeclared variable %0 used" },
	[DiagExprDoesNotReturnValue] = { DiagLevelError, "Expression does not return a value" },
	[DiagMxLiteralOnly1x1] = { DiagLevelError, "A matrix literal here can only be 1x1" },
	[DiagMxLiteralShapesDifferAddSub] = { DiagLevelError, "Matrices of shapes %0 and %1 cannot be added/subtracted together" },
	[DiagMxLiteralShapesDifferMul] = { DiagLevelError, "Matrices of shapes %0 and %1 cannot be multiplied together" },
	[DiagMxLiteralShapesDifferDiv]
	= { DiagLevelError, "Matrices of shapes %0 and %1 cannot be divided together. Consider multiplication by the inverse matrix" },
	[DiagMxLiteralShapesDifferComp] = { DiagLevelError, "Matrices of shapes %0 and %1 cannot be compared together" },
	[DiagMxLiteralShapesDifferAssign] = { DiagLevelError, "Assigning a matrix of shape %0 to a variable of shape %1" },
	[DiagMxLiteralInvalidPower] = { DiagLevelError, "Matrix exponentiation requires a positive, natural, 1x1 exponent" },
	[DiagMxLiteralInvalidPowerBase] = { DiagLevelError, "Matrix exponentiation requires a square matrix" },
	[DiagUninitializedUntypedVar] = { DiagLevelError, "Variable %0 must have either a type declaration or an initialization expression" },
	[DiagUninitializedConstVar] = { DiagLevelError, "A const variable %0 must have an initialization expression" },
	[DiagAssignToConstVar] = { DiagLevelError, "Assignment to a constant variable" },
	[DiagIndexOutOfRange] = { DiagLevelError, "Index %0 out of range for matrix of shape %1" },
	[DiagIndexNotInteger] = { DiagLevelError, "Index %0 is not an integer" },
	[DiagTooManyFunctionCallArgs] = { DiagLevelError, "Too many arguments in function call %0" },
	[DiagTooLittleFunctionCallArgs] = { DiagLevelError, "Too little arguments in function call %0" },
	[DiagFnCallArgMustBeCompTime] = { DiagLevelError, "Function call argument here must be a compile-time 1x1 matrix literal" },
	[DiagFnCallArgMustBeVec] = { DiagLevelError, "Function call argument here must be a vector" },
	[DiagFnCallArgMustBeSquare] = { DiagLevelError, "Function call argument here must be square" },
	[DiagFnCallArgsMustBeEqualShape] = { DiagLevelError, "Function call arguments here must have identical shapes" },
	[DiagNotInteger] = { DiagLevelError, "Number must be an integer" },
	[DiagUndeclaredFunction] = { DiagLevelError, "Call to undeclared function %0" },
	[DiagLogInvalidBase] = { DiagLevelError, "Logarithm base %0 must be greater than 0 and not equal to 1" },
	[DiagLogInvalidArg] = { DiagLevelError, "Logarithm argument %0 must be greater than 0" },
	[DiagSqrtInvalidArg] = { DiagLevelError, "Square root argument %0 must be greater than or equal to 0" },
	[DiagDivisionByZero] = { DiagLevelError, "Division by 0" },
	[DiagPoweringToNonInt] = { DiagLevelError, "A matrix can only be raised to a power of a positive integer. Here: %0" },
	[DiagInputInternalErr] = { DiagLevelError, "An internal error occured while trying to read stdin" },
	[DiagInvalidInput] = { DiagLevelError, "Input return type must be a valid matrix" },
	[DiagInvalidMxShape] = { DiagLevelError, "Invalid matrix shape" },
	[DiagMatrixIsSingular] = { DiagLevelError, "Cannot compute inverse of singular matrix" },
	[DiagUnusedExpressionResult] = { DiagLevelWarning, "Unused expression result" },
	[DiagEmptyFileParsed] = { DiagLevelNote, "Empty file parsed" } };

static usz NumberWidth(usz num)
{
	usz width = 0;

	while (num > 0) {
		num /= 10;
		++width;
	}

	return width;
}

static void PrintResult(Result result, FILE* out)
{
	switch (result) {
	case ResUnimplemented:
		fprintf(out, "ResUnimplemented");
		break;
	case ResCouldNotOpenFile:
		fprintf(out, "ResCouldNotOpenFile");
		break;
	case ResOutOfMemory:
		fprintf(out, "ResOutOfMemory");
		break;
	case ResNotFound:
		fprintf(out, "ResNotFound");
		break;
	case ResInvalidParams:
		fprintf(out, "ResInvalidParams");
		break;
	case ResEndOfIteration:
		fprintf(out, "ResEndOfIteration");
		break;
	default:
		fprintf(out, "ResultUnknown");
		break;
	}
}

static void PrintTokenType(TokenType tokenType, FILE* out)
{
	if (tokenType < 256) {
		fputc('\'', out);
		fputc((char)tokenType, out);
		fputc('\'', out);
		return;
	}

	switch (tokenType) {
	case TokenLeftRoundBracket:
		fputs("'('", out);
		break;
	case TokenRightRoundBracket:
		fputs("')'", out);
		break;
	case TokenLeftSquareBracket:
		fputs("'['", out);
		break;
	case TokenRightSquareBracket:
		fputs("']'", out);
		break;
	case TokenLeftVectorBracket:
		fputs("'<<'", out);
		break;
	case TokenRightVectorBracket:
		fputs("'>>'", out);
		break;
	case TokenLeftCurlyBracket:
		fputs("'{'", out);
		break;
	case TokenRightCurlyBracket:
		fputs("'}'", out);
		break;
	case TokenComma:
		fputs("','", out);
		break;
	case TokenAdd:
		fputs("'+'", out);
		break;
	case TokenSubtract:
		fputs("'-'", out);
		break;
	case TokenMultiply:
		fputs("'*'", out);
		break;
	case TokenDivide:
		fputs("'/'", out);
		break;
	case TokenToPower:
		fputs("'^'", out);
		break;
	case TokenTranspose:
		fputs("\"'\"", out);
		break;
	case TokenColon:
		fputs("':'", out);
		break;
	case TokenMatrixShape:
		fputs("matrix shape", out);
		break;
	case TokenEqual:
		fputs("'='", out);
		break;
	case TokenIdentifier:
		fputs("identifier", out);
		break;
	case TokenNumber:
		fputs("number", out);
		break;
	case TokenLet:
		fputs("'let'", out);
		break;
	case TokenConst:
		fputs("'const'", out);
		break;
	case TokenIf:
		fputs("'if'", out);
		break;
	case TokenElse:
		fputs("'else'", out);
		break;
	case TokenWhile:
		fputs("'while'", out);
		break;
	case TokenOr:
		fputs("'or'", out);
		break;
	case TokenAnd:
		fputs("'and'", out);
		break;
	case TokenEqualEqual:
		fputs("'=='", out);
		break;
	case TokenNotEqual:
		fputs("'!='", out);
		break;
	case TokenLess:
		fputs("'<'", out);
		break;
	case TokenLessEqual:
		fputs("'<='", out);
		break;
	case TokenGreater:
		fputs("'>'", out);
		break;
	case TokenGreaterEqual:
		fputs("'>='", out);
		break;
	case TokenEof:
		fputs("EOF", out);
		break;
	}
}

static void PrintToken(const Token* token, FILE* out)
{
	if (token->Type == TokenIdentifier) {
		fprintf(out, "'%.*s'", (i32)token->Lexeme.SymbolLength, token->Lexeme.Symbol);
	} else if (token->Type == TokenNumber) {
		fprintf(out, "'%lf'", token->Number);
	} else {
		PrintTokenType(token->Type, out);
	}
}

static void PrintDiagFormat(FILE* out, const char* format, const Diag* diag)
{
	const char* iter = format;
	while (*iter) {
		if (*iter == '%' && iter[1] >= '0' && iter[1] <= '9') {
			usz i = (usz)(iter[1] - '0');

			switch (diag->Args[i].Type) {
			case DiagArgChar:
				fputc('\'', out);
				fputc(diag->Args[i].Char, out);
				fputc('\'', out);
				break;
			case DiagArgToken:
				PrintToken(&diag->Args[i].Token, out);
				break;
			case DiagArgTokenType:
				PrintTokenType(diag->Args[i].TokenType, out);
				break;
			case DiagArgSymbolView:
				fprintf(out, "'%.*s'", (i32)diag->Args[i].SymbolView.SymbolLength, diag->Args[i].SymbolView.Symbol);
				break;
			case DiagArgMxShape:
				fprintf(out, "%zux%zu", diag->Args[i].MxShape.Height, diag->Args[i].MxShape.Width);
				break;
			case DiagArgNumber:
				fprintf(out, "'%lf'", diag->Args[i].Number);
				break;
			case DiagArgResult:
				PrintResult(diag->Args[i].Result, out);
				break;
			}

			++iter;
		} else {
			fputc(*iter, out);
		}

		++iter;
	}
}

static void DiagPrint(const Diag* diag)
{
	const DiagInfo* info = DIAG_TYPE_INFO + diag->Type;

	FILE* out = info->Level == DiagLevelError ? stderr : stdout;

	fputs("\033[1m", out);
	fprintf(out, "%s:%zu:%zu %s: ", g_source.FileName, diag->Loc.Line, diag->Loc.LinePos, DIAG_LEVEL_STR[info->Level]);
	fputs("\033[0m\033[1m", out);
	PrintDiagFormat(out, info->Format, diag);
	fputs("\033[0m", out);

	fputc('\n', out);

	usz numWidth = NumberWidth(diag->Loc.Line);

	for (usz i = 0; i < numWidth; ++i) {
		fputc(' ', out);
	}
	fprintf(out, " |\n");

	fprintf(out, "%zu | ", diag->Loc.Line);

	const char* iter = g_source.Lines[diag->Loc.Line - 1];
	bool beginning = true;
	while (*iter != '\n' && *iter != '\0') {
		if (beginning && isspace(*iter)) {
			++iter;
			continue;
		}

		beginning = false;
		fputc(*iter, out);
		++iter;
	}
	fputc('\n', out);

	for (usz i = 0; i < numWidth; ++i) {
		fputc(' ', out);
	}
	fprintf(out, " | ");

	beginning = true;
	for (usz i = 0; i < diag->Loc.LinePos - 1; ++i) {
		if (beginning && isspace(g_source.Lines[diag->Loc.Line - 1][i])) {
			continue;
		}

		beginning = false;
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

void DiagEmit(DiagType type, SourceLoc loc, const DiagArg* args, usz argCount)
{
	Diag* diag = nullptr;
	assert(StatArenaAlloc(&g_diagState.Arena, (void**)&diag) == ResOk);

	diag->Type = type;
	diag->Loc = loc;
	for (usz i = 0; i < argCount; ++i) {
		diag->Args[i] = args[i];
	}
}

[[noreturn]] void DiagPanic(Result result, const char* fileName, i32 lineNumber)
{
	DiagReport();

	fprintf(stderr, "An unrecoverable internal error occurred at %s:%d: ", fileName, lineNumber);
	PrintResult(result, stderr);
	fputc('\n', stderr);

	exit(1);
}

usz DiagReport()
{
	usz errCount = 0;
	StatArenaIter iter = { 0 };
	while (StatArenaIterNext(&g_diagState.Arena, &iter) == ResOk) {
		Diag* diag = iter.Item;

		DiagPrint(diag);

		const DiagInfo* info = DIAG_TYPE_INFO + diag->Type;
		if (info->Level == DiagLevelError) {
			++errCount;
		}
	}

	DIAG_PANIC_ON_ERR(StatArenaMarkUndo(&g_diagState.Arena, &g_diagState.Mark));

	DIAG_PANIC_ON_ERR(StatArenaMarkSet(&g_diagState.Arena, &g_diagState.Mark));

	return errCount;
}

Result DiagDeinit() { return StatArenaDeinit(&g_diagState.Arena); }
