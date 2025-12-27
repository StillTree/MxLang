#include "Mx.h"
#include "Interpreter.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

bool IsF64Int(f64 num)
{
	if (!isfinite(num))
		return false;

	if (num < (double)INT64_MIN || num > (double)INT64_MAX)
		return false;

	return trunc(num) == num;
}

void MxPrint(const Mx* mx)
{
	for (usz i = 0; i < mx->Shape.Height; ++i) {
		printf("[");

		for (usz j = 0; j < mx->Shape.Width; ++j) {
			printf("%lf", mx->Data[(i * mx->Shape.Width) + j]);

			if (j < mx->Shape.Width - 1) {
				printf(" ");
			}
		}

		printf("]");
	}
}

void MxAdd(const Mx* left, const Mx* right, Mx* out)
{
	if (left->Shape.Height == 1 && left->Shape.Width == 1) {
		out->Shape = right->Shape;

		for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
			out->Data[i] = left->Data[0] + right->Data[i];
		}

		return;
	}

	if (right->Shape.Height == 1 && right->Shape.Width == 1) {
		out->Shape = left->Shape;

		for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
			out->Data[i] = left->Data[i] + right->Data[0];
		}

		return;
	}

	out->Shape.Height = left->Shape.Height;
	out->Shape.Width = left->Shape.Width;

	for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
		out->Data[i] = left->Data[i] + right->Data[i];
	}
}

void MxSubtract(const Mx* left, const Mx* right, Mx* out)
{
	if (left->Shape.Height == 1 && left->Shape.Width == 1) {
		out->Shape = right->Shape;

		for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
			out->Data[i] = left->Data[0] - right->Data[i];
		}

		return;
	}

	if (right->Shape.Height == 1 && right->Shape.Width == 1) {
		out->Shape = left->Shape;

		for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
			out->Data[i] = left->Data[i] - right->Data[0];
		}

		return;
	}

	out->Shape.Height = left->Shape.Height;
	out->Shape.Width = left->Shape.Width;

	for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
		out->Data[i] = left->Data[i] - right->Data[i];
	}
}

void MxMultiply(const Mx* left, const Mx* right, Mx* out)
{
	if (left->Shape.Height == 1 && left->Shape.Width == 1) {
		out->Shape = right->Shape;

		for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
			out->Data[i] = left->Data[0] * right->Data[i];
		}

		return;
	}

	if (right->Shape.Height == 1 && right->Shape.Width == 1) {
		out->Shape = left->Shape;

		for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
			out->Data[i] = left->Data[i] * right->Data[0];
		}

		return;
	}

	out->Shape.Height = left->Shape.Height;
	out->Shape.Width = right->Shape.Width;

	for (usz i = 0; i < left->Shape.Height; ++i) {
		for (usz j = 0; j < right->Shape.Height; ++j) {
			for (usz k = 0; k < right->Shape.Width; ++k) {
				out->Data[(i * out->Shape.Width) + k]
					+= left->Data[(i * left->Shape.Width) + j] * right->Data[(j * right->Shape.Width) + k];
			}
		}
	}
}

Result MxDivide(const Mx* left, const Mx* right, Mx* out)
{
	if (left->Shape.Height == 1 && left->Shape.Width == 1) {
		out->Shape = right->Shape;

		for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
			if (right->Data[i] == 0) {
				return ResErr;
			}

			out->Data[i] = left->Data[0] / right->Data[i];
		}

		return ResOk;
	}

	if (right->Data[0] == 0) {
		return ResErr;
	}

	out->Shape = left->Shape;

	for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
		out->Data[i] = left->Data[i] / right->Data[0];
	}

	return ResOk;
}

Result MxToPower(const Mx* left, const Mx* right, Mx* out)
{
	if (left->Shape.Height == 1 && left->Shape.Width == 1 && right->Shape.Height == 1 && right->Shape.Width == 1) {
		out->Shape = left->Shape;

		out->Data[0] = pow(left->Data[0], right->Data[0]);

		return ResOk;
	}

	if (right->Data[0] < 0 || !IsF64Int(right->Data[0])) {
		return ResErr;
	}

	u64 power = (u64)right->Data[0];

	out->Shape = left->Shape;

	if (power == 0) {
		memset(out->Data, 0, out->Shape.Height * out->Shape.Width * sizeof(f64));

		for (usz i = 0; i < out->Shape.Width; ++i) {
			out->Data[(i * out->Shape.Width) + i] = 1;
		}

		return ResOk;
	}

	memcpy(out->Data, left->Data, out->Shape.Height * out->Shape.Width * sizeof(f64));

	Mx* temp = InterpreterAllocMx(out->Shape.Height, out->Shape.Width);

	for (u64 i = 1; i < power; ++i) {
		MxMultiply(out, left, temp);
		memcpy(out->Data, temp->Data, out->Shape.Height * out->Shape.Width * sizeof(f64));
	}

	return ResOk;
}

void MxTranspose(const Mx* mx, Mx* out)
{
	out->Shape.Height = mx->Shape.Width;
	out->Shape.Width = mx->Shape.Height;

	for (usz i = 0; i < mx->Shape.Height; ++i) {
		for (usz j = 0; j < mx->Shape.Width; ++j) {
			out->Data[(j * out->Shape.Width) + i] = mx->Data[(i * mx->Shape.Width) + j];
		}
	}
}

void MxNegate(const Mx* mx, Mx* out)
{
	out->Shape = mx->Shape;

	for (usz i = 0; i < mx->Shape.Height * mx->Shape.Width; ++i) {
		out->Data[i] = -mx->Data[i];
	}
}

void MxGreater(const Mx* left, const Mx* right, Mx* out)
{
	out->Shape.Height = 1;
	out->Shape.Width = 1;
	out->Data[0] = 1;

	for (usz i = 0; i < left->Shape.Height * left->Shape.Width; ++i) {
		if (left->Data[i] <= right->Data[i]) {
			out->Data[0] = 0;
			break;
		}
	}
}

void MxGreaterEqual(const Mx* left, const Mx* right, Mx* out)
{
	out->Shape.Height = 1;
	out->Shape.Width = 1;
	out->Data[0] = 1;

	for (usz i = 0; i < left->Shape.Height * left->Shape.Width; ++i) {
		if (left->Data[i] < right->Data[i]) {
			out->Data[0] = 0;
			break;
		}
	}
}

void MxLess(const Mx* left, const Mx* right, Mx* out)
{
	out->Shape.Height = 1;
	out->Shape.Width = 1;
	out->Data[0] = 1;

	for (usz i = 0; i < left->Shape.Height * left->Shape.Width; ++i) {
		if (left->Data[i] >= right->Data[i]) {
			out->Data[0] = 0;
			break;
		}
	}
}

void MxLessEqual(const Mx* left, const Mx* right, Mx* out)
{
	out->Shape.Height = 1;
	out->Shape.Width = 1;
	out->Data[0] = 1;

	for (usz i = 0; i < left->Shape.Height * left->Shape.Width; ++i) {
		if (left->Data[i] > right->Data[i]) {
			out->Data[0] = 0;
			break;
		}
	}
}

void MxEqualEqual(const Mx* left, const Mx* right, Mx* out)
{
	out->Shape.Height = 1;
	out->Shape.Width = 1;
	out->Data[0] = 1;

	for (usz i = 0; i < left->Shape.Height * left->Shape.Width; ++i) {
		if (left->Data[i] != right->Data[i]) {
			out->Data[0] = 0;
			break;
		}
	}
}

void MxNotEqual(const Mx* left, const Mx* right, Mx* out)
{
	out->Shape.Height = 1;
	out->Shape.Width = 1;
	out->Data[0] = 1;

	for (usz i = 0; i < left->Shape.Height * left->Shape.Width; ++i) {
		if (left->Data[i] == right->Data[i]) {
			out->Data[0] = 0;
			break;
		}
	}
}

bool MxTruthy(const Mx* mx)
{
	for (usz i = 0; i < mx->Shape.Height * mx->Shape.Width; ++i) {
		if (mx->Data[i] == 0) {
			return false;
		}
	}

	return true;
}

void MxLogicalOr(const Mx* left, const Mx* right, Mx* out)
{
	out->Data[0] = 1;
	out->Shape.Height = 1;
	out->Shape.Width = 1;

	usz leftSize = left->Shape.Height * left->Shape.Width;
	usz rightSize = right->Shape.Height * right->Shape.Width;
	usz smallerSize = leftSize < rightSize ? leftSize : rightSize;

	for (usz i = 0; i < smallerSize; ++i) {
		if (!((bool)left->Data[i] || (bool)right->Data[i])) {
			out->Data[0] = 0;
			break;
		}
	}
}

void MxLogicalAnd(const Mx* left, const Mx* right, Mx* out)
{
	out->Data[0] = 1;
	out->Shape.Height = 1;
	out->Shape.Width = 1;

	usz leftSize = left->Shape.Height * left->Shape.Width;
	usz rightSize = right->Shape.Height * right->Shape.Width;
	usz smallerSize = leftSize < rightSize ? leftSize : rightSize;

	for (usz i = 0; i < smallerSize; ++i) {
		if (!((bool)left->Data[i] && (bool)right->Data[i])) {
			out->Data[0] = 0;
			break;
		}
	}
}
