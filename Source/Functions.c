#include "Functions.h"

#include "Diagnostics.h"
#include "Interpreter.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Mx* FuncInterpretDisplay(ASTNode* functionCall)
{
	for (size_t i = 0; i < functionCall->FunctionCall.ArgCount; ++i) {
		if (functionCall->FunctionCall.CallArgs[i]->Type == ASTNodeIdentifier) {
			printf("%.*s: ", (i32)functionCall->FunctionCall.CallArgs[i]->Identifier.Identifier.SymbolLength,
				functionCall->FunctionCall.CallArgs[i]->Identifier.Identifier.Symbol);
		} else {
			printf("imm: ");
		}

		Mx* mx = InterpreterEval(functionCall->FunctionCall.CallArgs[i]);
		if (!mx) {
			// TODO: Panic!
			return nullptr;
		}

		MxPrint(mx);

		if (i < functionCall->FunctionCall.ArgCount - 1) {
			printf(", ");
		}
	}

	printf("\n");

	return nullptr;
}

Mx* FuncInterpretFill(ASTNode* functionCall)
{
	usz height = (usz)functionCall->FunctionCall.CallArgs[0]->MxLiteral.Matrix[0]->Number;
	usz width = (usz)functionCall->FunctionCall.CallArgs[1]->MxLiteral.Matrix[0]->Number;

	Mx* fillValue = InterpreterEval(functionCall->FunctionCall.CallArgs[2]);

	Mx* mx = InterpreterAllocMx(height, width);

	for (usz i = 0; i < height * width; ++i) {
		mx->Data[i] = fillValue->Data[0];
	}

	return mx;
}

Mx* FuncInterpretIdent(ASTNode* functionCall)
{
	usz size = (usz)functionCall->FunctionCall.CallArgs[0]->MxLiteral.Matrix[0]->Number;

	Mx* mx = InterpreterAllocMx(size, size);
	memset(mx->Data, 0, size * size * sizeof(f64));

	for (usz i = 0; i < size; ++i) {
		mx->Data[(i * size) + i] = 1;
	}

	return mx;
}

Mx* FuncInterpretLog(ASTNode* functionCall)
{
	Mx* base = InterpreterEval(functionCall->FunctionCall.CallArgs[0]);

	if (base->Data[0] <= 0 || base->Data[0] == 1) {
		DIAG_EMIT(DiagLogInvalidBase, functionCall->FunctionCall.CallArgs[0]->Loc, DIAG_ARG_NUMBER(base->Data[0]));
		return nullptr;
	}

	Mx* arg = InterpreterEval(functionCall->FunctionCall.CallArgs[1]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	f64 baseLnInverse = 1 / log(base->Data[0]);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		if (arg->Data[i] <= 0) {
			DIAG_EMIT(DiagLogInvalidArg, functionCall->FunctionCall.CallArgs[1]->Loc, DIAG_ARG_NUMBER(arg->Data[i]));
			return nullptr;
		}

		mx->Data[i] = log(arg->Data[i]) * baseLnInverse;
	}

	return mx;
}

Mx* FuncInterpretLn(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FunctionCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		if (arg->Data[i] <= 0) {
			DIAG_EMIT(DiagLogInvalidArg, functionCall->FunctionCall.CallArgs[0]->Loc, DIAG_ARG_NUMBER(arg->Data[i]));
			return nullptr;
		}

		mx->Data[i] = log(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretSqrt(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FunctionCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		if (arg->Data[i] < 0) {
			DIAG_EMIT(DiagSqrtInvalidArg, functionCall->FunctionCall.CallArgs[0]->Loc, DIAG_ARG_NUMBER(arg->Data[i]));
			return nullptr;
		}

		mx->Data[i] = sqrt(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretAbs(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FunctionCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = fabs(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretCeil(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FunctionCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = ceil(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretFloor(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FunctionCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = floor(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretSin(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FunctionCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = sin(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretCos(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FunctionCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = cos(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretTan(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FunctionCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = tan(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretCot(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FunctionCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = 1 / tan(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretRand(ASTNode* functionCall)
{
	usz height = (usz)functionCall->FunctionCall.CallArgs[0]->MxLiteral.Matrix[0]->Number;
	usz width = (usz)functionCall->FunctionCall.CallArgs[1]->MxLiteral.Matrix[0]->Number;

	Mx* mx = InterpreterAllocMx(height, width);

	for (usz i = 0; i < height * width; ++i) {
		mx->Data[i] = (f64)rand() / ((f64)RAND_MAX + 1);
	}

	return mx;
}

static Result ParseDouble(const char* str, usz strLength, double* num)
{
	const char* strEnd = str + strLength;
	f64 value = 0.0;

	while (str < strEnd && *str >= '0' && *str <= '9') {
		value *= 10;
		value += *str++ - '0';
	}

	if (str < strEnd && *str == '.') {
		f64 fraction = 0.1;
		str++;

		while (str < strEnd && *str >= '0' && *str <= '9') {
			value += (*str++ - '0') * fraction;
			fraction /= 10;
		}
	}

	if (str < strEnd && (*str == 'e' || *str == 'E')) {
		i64 exponent = 0;
		i64 exponentSign = 1;
		str++;

		if (*str == '-') {
			exponentSign = -1;
			str++;
		} else if (*str == '+') {
			str++;
		}

		while (str < strEnd && *str >= '0' && *str <= '9') {
			exponent *= 10;
			exponent += *str++ - '0';
		}

		value *= pow(10.0, (f64)exponentSign * (f64)exponent);
	} else if (str < strEnd) {
		return ResErr;
	}

	*num = value;
	return ResOk;
}

Mx* FuncInterpretInput(ASTNode* functionCall)
{
	printf(">>> ");

	char buf[256];

	if (!fgets(buf, sizeof(buf), stdin)) {
		DIAG_EMIT0(DiagInputTooBig, functionCall->Loc);
		InterpreterPanic();
	}

	usz len = strlen(buf);
	if (buf[len - 1] == '\n') {
		--len;
	}

	double num;
	Result result = ParseDouble(buf, len, &num);
	if (result) {
		DIAG_EMIT0(DiagInvalidInput, functionCall->Loc);
		InterpreterPanic();
	}

	Mx* mx = InterpreterAllocMx(1, 1);
	mx->Data[0] = num;

	return mx;
}
