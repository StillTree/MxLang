#pragma once

#include "Memory/StatArena.h"
#include "Result.h"
#include "Tokenizer.h"
#include "Types.h"

static constexpr usz MAX_DIAG_ARGS = 3;

typedef enum DiagType {
	DiagExpectedToken,
	DiagExpectedIdentifier,
	DiagUnexpectedToken,
	DiagExpectedTokenAfter,
	DiagEmptyMxLiteralsNotAllowed,
	DiagEmptyVecLiteralsNotAllowed,
	DiagRedeclarationInScope,
	DiagUndeclaredVarUsed,
	DiagExprDoesNotReturnValue,
	DiagMxLiteralOnly1x1,
	DiagMxLiteralShapesDifferAddSub,
	DiagMxLiteralShapesDifferMul,
	DiagMxLiteralShapesDifferDiv,
	DiagMxLiteralShapesDifferComp,
	DiagMxLiteralShapesDifferAssign,
	DiagMxLiteralInvalidPower,
	DiagMxLiteralInvalidPowerBase,
	DiagUninitializedUntypedVar,
	DiagUninitializedConstVar,
	DiagAssignToConstVar,
	DiagIndexOutOfRange,
	DiagIndexNotInteger,
	DiagTooManyFunctionCallArgs,
	DiagTooLittleFunctionCallArgs,
	DiagFunctionCallArgMustBeCompTime,
	DiagNotInteger,
	DiagUndeclaredFunction,
	DiagLogInvalidBase,
	DiagLogInvalidArg,
	DiagSqrtInvalidArg,
	DiagDivisionByZero,
	DiagPoweringToNonInt,
	DiagInputTooBig,
	DiagUnusedExpressionResult,
	DiagEmptyFileParsed
} DiagType;

typedef enum DiagArgType {
	DiagArgChar,
	DiagArgToken,
	DiagArgTokenType,
	DiagArgSymbolView,
	DiagArgMxShape,
	DiagArgNumber,
	DiagArgResult
} DiagArgType;

typedef struct DiagArg {
	DiagArgType Type;
	union {
		char Char;
		Token Token;
		TokenType TokenType;
		SymbolView SymbolView;
		MxShape MxShape;
		double Number;
		Result Result;
	};
} DiagArg;

typedef struct Diag {
	DiagType Type;
	SourceLoc Loc;
	DiagArg Args[MAX_DIAG_ARGS];
} Diag;

typedef struct DiagState {
	StatArena Arena;
	StatArenaMark Mark;
} DiagState;

Result DiagInit();
void DiagEmit(DiagType type, SourceLoc loc, const DiagArg* args, usz argCount);
usz DiagReport();
[[noreturn]] void DiagPanic(Result result, const char* fileName, i32 lineNumber);
Result DiagDeinit();

extern DiagState g_diagState;

#define DIAG_PANIC_ON_ERR(x)                                                                                                               \
	do {                                                                                                                                   \
		Result r = (x);                                                                                                               \
		if (r) {                                                                                                                      \
			DiagPanic(r, __FILE_NAME__, __LINE__);                                                                                    \
		}                                                                                                                                  \
	} while (false)

#define DIAG_ARG_CHAR(x) ((DiagArg) { DiagArgChar, { .Char = (x) } })
#define DIAG_ARG_TOKEN(x) ((DiagArg) { DiagArgToken, { .Token = (x) } })
#define DIAG_ARG_TOKEN_TYPE(x) ((DiagArg) { DiagArgTokenType, { .TokenType = (x) } })
#define DIAG_ARG_SYMBOL_VIEW(x) ((DiagArg) { DiagArgSymbolView, { .SymbolView = (x) } })
#define DIAG_ARG_MX_SHAPE(x) ((DiagArg) { DiagArgMxShape, { .MxShape = (x) } })
#define DIAG_ARG_NUMBER(x) ((DiagArg) { DiagArgNumber, { .Number = (x) } })
#define DIAG_ARG_RESULT(x) ((DiagArg) { DiagArgResult, { .Result = (x) } })

#define DIAG_EMIT0(type, loc) DiagEmit((type), (loc), nullptr, 0)

#define DIAG_EMIT(type, loc, ...)                                                                                                          \
	do {                                                                                                                                   \
		static_assert(sizeof((const DiagArg[]) { __VA_ARGS__ }) / sizeof(DiagArg) <= MAX_DIAG_ARGS, "Too many diagnostic arguments");      \
		DiagEmit((type), (loc), (const DiagArg[]) { __VA_ARGS__ }, sizeof((const DiagArg[]) { __VA_ARGS__ }) / sizeof(DiagArg));           \
	} while (false)
