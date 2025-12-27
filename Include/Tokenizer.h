#pragma once

#include "Memory/SymbolTable.h"
#include "MxShape.h"
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
	TokenEof
} TokenType;

typedef struct SourceLoc {
	usz Line;
	usz LinePos;
} SourceLoc;

typedef struct Token {
	TokenType Type;
	union {
		SymbolView Lexeme;
		double Number;
		MxShape MatrixShape;
	};
	SourceLoc Loc;
} Token;

typedef struct Tokenizer {
	const char* SourceEnd;
	const char* LexemeStart;
	const char* LexemeCurrent;
	usz SourceLine;
	usz SourceLinePos;
	SymbolTable TableIdentifiers;
	Token LastReturnedToken;
	bool HasLookahead;
} Tokenizer;

void TokenizerInit();
Token* TokenizerNextToken();
Token* TokenizerPeekToken();
void TokenizerDeinit();

extern Tokenizer g_tokenizer;
