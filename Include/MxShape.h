#pragma once

#include "Types.h"

typedef enum MxType {
	MxStat,
	MxDyn
} MxType;

typedef struct MxShape {
	MxType Type;

	usz Height;
	usz Width;
} MxShape;
