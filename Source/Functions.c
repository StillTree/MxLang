#include "Functions.h"

#include "Diagnostics.h"
#include "Interpreter.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Mx* FuncInterpretDisplay(ASTNode* functionCall)
{
	for (size_t i = 0; i < functionCall->FnCall.ArgCount; ++i) {
		Mx* mx = InterpreterEval(functionCall->FnCall.CallArgs[i]);
		if (!mx) {
			continue;
		}

		if (functionCall->FnCall.CallArgs[i]->Type == ASTNodeIdentifier) {
			printf("%.*s: ", (i32)functionCall->FnCall.CallArgs[i]->Identifier.Identifier.SymbolLength,
				functionCall->FnCall.CallArgs[i]->Identifier.Identifier.Symbol);
		} else {
			printf("imm: ");
		}

		MxPrint(mx);

		if (i < functionCall->FnCall.ArgCount - 1) {
			printf(", ");
		}
	}

	printf("\n");

	return nullptr;
}

Mx* FuncInterpretFill(ASTNode* functionCall)
{
	usz height = (usz)functionCall->FnCall.CallArgs[0]->MxLiteral.Matrix[0]->Number;
	usz width = (usz)functionCall->FnCall.CallArgs[1]->MxLiteral.Matrix[0]->Number;

	Mx* fillValue = InterpreterEval(functionCall->FnCall.CallArgs[2]);

	Mx* mx = InterpreterAllocMx(height, width);

	for (usz i = 0; i < height * width; ++i) {
		mx->Data[i] = fillValue->Data[0];
	}

	return mx;
}

Mx* FuncInterpretIdent(ASTNode* functionCall)
{
	usz size = (usz)functionCall->FnCall.CallArgs[0]->MxLiteral.Matrix[0]->Number;

	Mx* mx = InterpreterAllocMx(size, size);
	memset(mx->Data, 0, size * size * sizeof(f64));

	for (usz i = 0; i < size; ++i) {
		mx->Data[(i * size) + i] = 1;
	}

	return mx;
}

Mx* FuncInterpretLog(ASTNode* functionCall)
{
	Mx* base = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	if (base->Data[0] <= 0 || base->Data[0] == 1) {
		DIAG_EMIT(DiagLogInvalidBase, functionCall->FnCall.CallArgs[0]->Loc, DIAG_ARG_NUMBER(base->Data[0]));
		InterpreterPanic();
	}

	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[1]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	f64 baseLnInverse = 1 / log(base->Data[0]);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		if (arg->Data[i] <= 0) {
			DIAG_EMIT(DiagLogInvalidArg, functionCall->FnCall.CallArgs[1]->Loc, DIAG_ARG_NUMBER(arg->Data[i]));
			InterpreterPanic();
		}

		mx->Data[i] = log(arg->Data[i]) * baseLnInverse;
	}

	return mx;
}

Mx* FuncInterpretLn(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		if (arg->Data[i] <= 0) {
			DIAG_EMIT(DiagLogInvalidArg, functionCall->FnCall.CallArgs[0]->Loc, DIAG_ARG_NUMBER(arg->Data[i]));
			InterpreterPanic();
		}

		mx->Data[i] = log(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretSqrt(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		if (arg->Data[i] < 0) {
			DIAG_EMIT(DiagSqrtInvalidArg, functionCall->FnCall.CallArgs[0]->Loc, DIAG_ARG_NUMBER(arg->Data[i]));
			InterpreterPanic();
		}

		mx->Data[i] = sqrt(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretAbs(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = fabs(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretCeil(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = ceil(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretFloor(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = floor(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretSin(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = sin(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretCos(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = cos(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretTan(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = tan(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretCot(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);

	for (usz i = 0; i < arg->Shape.Height * arg->Shape.Width; ++i) {
		mx->Data[i] = 1 / tan(arg->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretRand(ASTNode* functionCall)
{
	usz height = (usz)functionCall->FnCall.CallArgs[0]->MxLiteral.Matrix[0]->Number;
	usz width = (usz)functionCall->FnCall.CallArgs[1]->MxLiteral.Matrix[0]->Number;

	Mx* mx = InterpreterAllocMx(height, width);

	for (usz i = 0; i < height * width; ++i) {
		mx->Data[i] = (f64)rand() / ((f64)RAND_MAX + 1);
	}

	return mx;
}

Mx* FuncInterpretInput(ASTNode* functionCall)
{
	usz height = (usz)functionCall->FnCall.CallArgs[0]->MxLiteral.Matrix[0]->Number;
	usz width = (usz)functionCall->FnCall.CallArgs[1]->MxLiteral.Matrix[0]->Number;

	Mx* mx = InterpreterAllocMx(height, width);
	for (usz i = 0; i < height; ++i) {
		for (usz j = 0; j < width; ++j) {
			printf(">>> elem[%zu %zu] = ", i + 1, j + 1);

			fflush(stdout);
			f64 num;
			scanf("%lf", &num);

			mx->Data[(i * width) + j] = num;
		}
	}

	return mx;
}

Mx* FuncInterpretReshape(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	usz height = (usz)functionCall->FnCall.CallArgs[1]->MxLiteral.Matrix[0]->Number;
	usz width = (usz)functionCall->FnCall.CallArgs[2]->MxLiteral.Matrix[0]->Number;

	Mx* mx = InterpreterAllocMx(height, width);
	for (usz i = 0; i < height; ++i) {
		for (usz j = 0; j < width; ++j) {
			if (i < arg->Shape.Height && j < arg->Shape.Width) {
				mx->Data[(i * width) + j] = arg->Data[(i * width) + j];
			} else {
				mx->Data[(i * width) + j] = 0;
			}
		}
	}

	return mx;
}

Mx* FuncInterpretDiag(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* mx = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Height);
	memset(mx->Data, 0, arg->Shape.Height * arg->Shape.Height * sizeof(f64));
	for (usz i = 0; i < arg->Shape.Height; ++i) {
		mx->Data[(i * arg->Shape.Height) + i] = arg->Data[i];
	}

	return mx;
}

Mx* FuncInterpretPow(ASTNode* functionCall)
{
	Mx* arg1 = InterpreterEval(functionCall->FnCall.CallArgs[0]);
	Mx* arg2 = InterpreterEval(functionCall->FnCall.CallArgs[1]);

	Mx* mx = InterpreterAllocMx(arg1->Shape.Height, arg1->Shape.Width);

	for (usz i = 0; i < arg1->Shape.Height * arg1->Shape.Width; ++i) {
		mx->Data[i] = pow(arg1->Data[i], arg2->Data[i]);
	}

	return mx;
}

Mx* FuncInterpretDet(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* temp = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);
	memcpy(temp->Data, arg->Data, arg->Shape.Height * arg->Shape.Height * sizeof(f64));

	f64 det = 1.0;
	int sign = 1;

	for (usz i = 0; i < arg->Shape.Height; ++i) {
		usz pivot = i;
		for (usz r = i; r < arg->Shape.Height; ++r) {
			if (fabs(temp->Data[(r * temp->Shape.Width) + i]) > fabs(temp->Data[(pivot * temp->Shape.Width) + i]))
				pivot = r;
		}

		if (fabs(temp->Data[(pivot * temp->Shape.Width) + i]) < 1e-12) {
			det = 0.0;
			break;
		}

		if (pivot != i) {
			for (usz c = 0; c < arg->Shape.Height; ++c) {
				f64 tmp = temp->Data[(i * temp->Shape.Width) + c];
				temp->Data[(i * temp->Shape.Width) + c] = temp->Data[(pivot * temp->Shape.Width) + c];
				temp->Data[(pivot * temp->Shape.Width) + c] = tmp;
			}

			sign = -sign;
		}

		f64 piv = temp->Data[(i * temp->Shape.Width) + i];
		det *= piv;

		for (usz r = i + 1; r < arg->Shape.Height; ++r) {
			f64 f = temp->Data[(r * temp->Shape.Width) + i] / piv;

			for (usz c = i; c < arg->Shape.Height; ++c) {
				temp->Data[(r * temp->Shape.Width) + c] -= f * temp->Data[(i * temp->Shape.Width) + c];
			}
		}
	}

	Mx* out = InterpreterAllocMx(1, 1);
	out->Data[0] = det * sign;
	return out;
}

Mx* FuncInterpretInv(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* temp = InterpreterAllocMx(arg->Shape.Height, 2 * arg->Shape.Height);
	memset(temp->Data, 0, arg->Shape.Height * 2 * arg->Shape.Height * sizeof(f64));

	for (usz r = 0; r < arg->Shape.Height; ++r) {
		for (usz c = 0; c < arg->Shape.Height; ++c)
			temp->Data[(r * temp->Shape.Width) + c] = arg->Data[(r * arg->Shape.Width) + c];
		temp->Data[(r * temp->Shape.Width) + r + arg->Shape.Height] = 1.0;
	}

	for (usz i = 0; i < arg->Shape.Height; ++i) {
		usz pivot = i;
		for (usz r = i; r < arg->Shape.Height; ++r) {
			if (fabs(temp->Data[(r * temp->Shape.Width) + i]) > fabs(temp->Data[(pivot * temp->Shape.Width) + i]))
				pivot = r;
		}

		if (fabs(temp->Data[(pivot * temp->Shape.Width) + i]) < 1e-12) {
			DIAG_EMIT0(DiagMatrixIsSingular, functionCall->FnCall.CallArgs[0]->Loc);
			InterpreterPanic();
		}

		if (pivot != i) {
			for (usz c = 0; c < 2 * arg->Shape.Height; ++c) {
				f64 tmp = temp->Data[(i * temp->Shape.Width) + c];
				temp->Data[(i * temp->Shape.Width) + c] = temp->Data[(pivot * temp->Shape.Width) + c];
				temp->Data[(pivot * temp->Shape.Width) + c] = tmp;
			}
		}

		f64 piv = temp->Data[(i * temp->Shape.Width) + i];
		for (usz c = 0; c < 2 * arg->Shape.Height; ++c) {
			temp->Data[(i * temp->Shape.Width) + c] /= piv;
		}

		for (usz r = 0; r < arg->Shape.Height; ++r) {
			if (r == i) {
				continue;
			}

			f64 f = temp->Data[(r * temp->Shape.Width) + i];
			for (usz c = 0; c < 2 * arg->Shape.Height; ++c) {
				temp->Data[(r * temp->Shape.Width) + c] -= f * temp->Data[(i * temp->Shape.Width) + c];
			}
		}
	}

	Mx* inv = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Height);
	for (usz r = 0; r < arg->Shape.Height; ++r) {
		for (usz c = 0; c < arg->Shape.Height; ++c) {
			inv->Data[(r * inv->Shape.Width) + c] = temp->Data[(r * temp->Shape.Width) + c + arg->Shape.Height];
		}
	}

	return inv;
}

Mx* FuncInterpretRank(ASTNode* functionCall)
{
	Mx* arg = InterpreterEval(functionCall->FnCall.CallArgs[0]);

	Mx* temp = InterpreterAllocMx(arg->Shape.Height, arg->Shape.Width);
	memcpy(temp->Data, arg->Data, arg->Shape.Height * arg->Shape.Width * sizeof(f64));

	usz rank = 0;
	usz row = 0;

	for (usz col = 0; col < arg->Shape.Width && row < arg->Shape.Height; ++col) {
		usz pivot = row;
		for (usz r = row; r < arg->Shape.Height; ++r) {
			if (fabs(temp->Data[(r * arg->Shape.Width) * col]) > fabs(temp->Data[(pivot * arg->Shape.Width) * col])) {
				pivot = r;
			}
		}

		if (fabs(temp->Data[(pivot * arg->Shape.Width) * col]) < 1e-12) {
			continue;
		}

		if (pivot != row) {
			for (usz c = 0; c < arg->Shape.Width; ++c) {
				f64 tmp = temp->Data[(row * arg->Shape.Width) * c];
				temp->Data[(row * arg->Shape.Width) * c] = temp->Data[(pivot * arg->Shape.Width) * c];
				temp->Data[(pivot * arg->Shape.Width) * c] = tmp;
			}
		}

		f64 piv = temp->Data[(row * arg->Shape.Width) * col];
		for (usz c = col; c < arg->Shape.Width; ++c) {
			temp->Data[(row * arg->Shape.Width) * c] /= piv;
		}

		for (usz r = 0; r < arg->Shape.Height; ++r) {
			if (r == row) {
				continue;
			}

			f64 f = temp->Data[(r * arg->Shape.Width) * col];
			for (usz c = col; c < arg->Shape.Width; ++c) {
				temp->Data[(r * arg->Shape.Width) * c] -= f * temp->Data[(row * arg->Shape.Width) * c];
			}
		}

		row++;
		rank++;
	}

	Mx* out = InterpreterAllocMx(1, 1);
	out->Data[0] = (f64)rank;
	return out;
}
