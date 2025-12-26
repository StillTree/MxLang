#include "Mx.h"

#include <stdio.h>

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

Result MxAdd(const Mx* left, const Mx* right, Mx* out)
{
	out->Shape.Height = left->Shape.Height;
	out->Shape.Width = left->Shape.Width;

	for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
		out->Data[i] = left->Data[i] + right->Data[i];
	}

	return ResOk;
}

Result MxSubtract(const Mx* left, const Mx* right, Mx* out)
{
	out->Shape.Height = left->Shape.Height;
	out->Shape.Width = left->Shape.Width;

	for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
		out->Data[i] = left->Data[i] - right->Data[i];
	}

	return ResOk;
}

Result MxMultiply(const Mx* left, const Mx* right, Mx* out)
{
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

	return ResOk;
}

Result MxDivide(const Mx* left, const Mx* right, Mx* out)
{
	out->Shape.Height = left->Shape.Height;
	out->Shape.Width = right->Shape.Width;

	for (usz i = 0; i < left->Shape.Height; ++i) {
		for (usz j = 0; j < right->Shape.Height; ++j) {
			for (usz k = 0; k < right->Shape.Width; ++k) {
				out->Data[(i * out->Shape.Width) + k]
					+= left->Data[(i * left->Shape.Width) + j] / right->Data[(j * right->Shape.Width) + k];
			}
		}
	}

	return ResOk;
}

Result MxToPower(const Mx* left, const Mx* right, Mx* out)
{
	out->Shape = left->Shape;

	for (usz i = 0; i < out->Shape.Height * out->Shape.Width; ++i) {
		out->Data[i] = left->Data[i];
	}

	for (usz i = 0; i < (usz)right->Data[0] - 1; ++i) {
		Result result = MxMultiply(out, left, out);
		if (result) {
			return result;
		}
	}

	return ResOk;
}

Result MxTranspose(const Mx* mx, Mx* out)
{
	out->Shape.Height = mx->Shape.Width;
	out->Shape.Width = mx->Shape.Height;

	for (usz i = 0; i < mx->Shape.Height; ++i) {
		for (usz j = 0; j < mx->Shape.Width; ++j) {
			out->Data[(j * out->Shape.Width) + i] = mx->Data[(i * mx->Shape.Width) + j];
		}
	}

	return ResOk;
}

Result MxNegate(const Mx* mx, Mx* out)
{
	out->Shape = mx->Shape;

	for (usz i = 0; i < mx->Shape.Height * mx->Shape.Width; ++i) {
		out->Data[i] = -mx->Data[i];
	}

	return ResOk;
}

Result MxGreater(const Mx* left, const Mx* right, Mx* out)
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

	return ResOk;
}

Result MxGreaterEqual(const Mx* left, const Mx* right, Mx* out)
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

	return ResOk;
}

Result MxLess(const Mx* left, const Mx* right, Mx* out)
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

	return ResOk;
}

Result MxLessEqual(const Mx* left, const Mx* right, Mx* out)
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

	return ResOk;
}

Result MxEqualEqual(const Mx* left, const Mx* right, Mx* out)
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

	return ResOk;
}

Result MxNotEqual(const Mx* left, const Mx* right, Mx* out)
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

	return ResOk;
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
