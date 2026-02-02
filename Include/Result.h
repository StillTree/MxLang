/*
MxLang - Result.h
Autor: Alexander Dębowski (293472)
Data: 09.01.2026
*/

#pragma once

#include "Types.h"

typedef enum Result : u8 {
	ResOk,
	ResUnimplemented,
	ResCouldNotOpenFile,
	ResOutOfMemory,
	ResNotFound,
	ResInvalidParams,
	ResEndOfIteration,
	ResInvalidToken,
	ResInvalidOperand,
	ResMatrixPowerToNonInt
} Result;
