/*
MxLang - Types.h
Autor: Alexander Dębowski (293472)
Data: 08.01.2026
*/

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef size_t usz;
typedef ptrdiff_t isz;

typedef float f32;
typedef double f64;

static inline usz AlignUp(usz value, usz alignment) { return (value + alignment - 1) & ~(alignment - 1); }
