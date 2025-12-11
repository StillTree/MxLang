#pragma once

#include "Memory/Arena.h"
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
		const char* Lexeme;
		double Number;
	};
	usz SourceLine;
	usz LinePos;
} Token;

typedef struct Tokenizer {
	const char* Source;
	const char* LexemeStart;
	const char* LexemeCurrent;
	usz SourceLine;
	Arena ArenaTokens;
} Tokenizer;
