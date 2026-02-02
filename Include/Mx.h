/*
MxLang - Mx.h
Autor: Alexander Dębowski (293472)
Data: 27.12.2025
*/

#pragma once

#include "MxShape.h"
#include "Types.h"
#include "Result.h"

typedef struct Mx {
	MxShape Shape;
	f64 Data[];
} Mx;

bool IsF64Int(f64 num);

void MxPrint(const Mx* mx);
void MxAdd(const Mx* left, const Mx* right, Mx* out);
void MxSubtract(const Mx* left, const Mx* right, Mx* out);
void MxMultiply(const Mx* left, const Mx* right, Mx* out);
Result MxDivide(const Mx* left, const Mx* right, Mx* out);
Result MxToPower(const Mx* left, const Mx* right, Mx* out);
void MxTranspose(const Mx* mx, Mx* out);
void MxNegate(const Mx* mx, Mx* out);
void MxGreater(const Mx* left, const Mx* right, Mx* out);
void MxGreaterEqual(const Mx* left, const Mx* right, Mx* out);
void MxLess(const Mx* left, const Mx* right, Mx* out);
void MxLessEqual(const Mx* left, const Mx* right, Mx* out);
void MxEqualEqual(const Mx* left, const Mx* right, Mx* out);
void MxNotEqual(const Mx* left, const Mx* right, Mx* out);
bool MxTruthy(const Mx* mx);
void MxLogicalOr(const Mx* left, const Mx* right, Mx* out);
void MxLogicalAnd(const Mx* left, const Mx* right, Mx* out);
