#pragma once

#include "Memory/StatArena.h"
#include "Memory/SymbolTable.h"
#include "Types.h"

typedef enum TokenType : u8 {
	TokenLeftRoundBracket,
	TokenRightRoundBracket,
	TokenLeftSquareBracket,
	TokenRightSquareBracket,
	TokenLeftVectorBracket,
	TokenRightVectorBracket,
	TokenLeftCurlyBracket,
	TokenRightCurlyBracket,
	TokenComma,
	TokenAdd,
	TokenSubtract,
	TokenMultiply,
	TokenDivide,
	TokenToPower,
	TokenTranspose,
	TokenColon,
	TokenMatrixShape,
	TokenEqual,
	TokenIdentifier,
	TokenNumber,
	TokenLet,
	TokenConst,
	TokenIf,
	TokenElse,
	TokenWhile,
	TokenOr,
	TokenAnd,
	TokenEqualEqual,
	TokenNotEqual,
	TokenLess,
	TokenLessEqual,
	TokenGreater,
	TokenGreaterEqual,
	TokenEof
} TokenType;

typedef struct Token {
	TokenType Type;
	union {
		SymbolView Lexeme;
		double Number;
	};
	usz SourceLine;
	usz SourceLinePos;
} Token;

typedef struct Tokenizer {
	const char* SourceEnd;
	const char* LexemeStart;
	const char* LexemeCurrent;
	usz SourceLine;
	usz SourceLinePos;
	StatArena ArenaTokens;
	SymbolTable TableStrings;
} Tokenizer;

Result TokenizerInit();
Result TokenizerScan();

extern Tokenizer g_tokenizer;
