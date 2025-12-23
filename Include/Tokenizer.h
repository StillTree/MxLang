#pragma once

#include "Memory/SymbolTable.h"
#include "Types.h"

typedef enum TokenType : u16 {
	TokenLeftRoundBracket = 256,
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
	TokenEof,
	TokenError
} TokenType;

typedef struct MatrixShape {
	u64 Height;
	u64 Width;
} MatrixShape;

typedef struct Token {
	TokenType Type;
	union {
		SymbolView Lexeme;
		double Number;
		MatrixShape MatrixShape;
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
	SymbolTable TableStrings;
	Token LastReturnedToken;
	bool HasLookahead;
} Tokenizer;

Result TokenizerInit();
Result TokenizerScan();
Result TokenizerNextToken(Token** token);
Result TokenizerPeekToken(Token** token);
Result TokenizerDeinit();

extern Tokenizer g_tokenizer;
