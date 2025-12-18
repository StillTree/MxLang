#pragma once

#include "Types.h"

typedef enum DiagLevel {
	Note,
	Warning,
	Error
} DiagLevel;

typedef enum DiagType {
	UnexpectedToken
} DiagType;

typedef struct Diagnostic {
	DiagLevel Level;
	DiagType Type;
	usz SourceLine;
	usz SourceLinePos;
} Diagnostic;

void EmitDiagnostic();
void PrintDiagnostic(const Diagnostic* diag);
