#pragma once

#include "MxShape.h"
#include "Types.h"
#include "Result.h"

typedef struct Mx {
	MxShape Shape;
	f64 Data[];
} Mx;

void MxPrint(const Mx* mx);
Result MxAdd(const Mx* left, const Mx* right, Mx* out);
Result MxSubtract(const Mx* left, const Mx* right, Mx* out);
Result MxMultiply(const Mx* left, const Mx* right, Mx* out);
Result MxDivide(const Mx* left, const Mx* right, Mx* out);
Result MxToPower(const Mx* left, const Mx* right, Mx* out);
Result MxTranspose(const Mx* mx, Mx* out);
Result MxNegate(const Mx* mx, Mx* out);
Result MxGreater(const Mx* left, const Mx* right, Mx* out);
Result MxGreaterEqual(const Mx* left, const Mx* right, Mx* out);
Result MxLess(const Mx* left, const Mx* right, Mx* out);
Result MxLessEqual(const Mx* left, const Mx* right, Mx* out);
Result MxEqualEqual(const Mx* left, const Mx* right, Mx* out);
Result MxNotEqual(const Mx* left, const Mx* right, Mx* out);
bool MxTruthy(const Mx* mx);
void MxLogicalOr(const Mx* left, const Mx* right, Mx* out);
void MxLogicalAnd(const Mx* left, const Mx* right, Mx* out);
