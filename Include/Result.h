#pragma once

#include "Types.h"

typedef enum Result : u8 {
	ResOk,
	ResUnimplemented,
	ResCouldNotOpenFile,
	ResOutOfMemory,
	ResNotFound,
	ResInvalidParams,
	ResErr
} Result;
