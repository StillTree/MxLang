#include "Diagnostics.h"
#include "SourceManager.h"
#include "Tokenizer.h"
#include "Parser.h"
#include <stdio.h>

void PrintToken(TokenType t)
{
	switch (t) {
	case TokenLeftRoundBracket:
		printf("TokenLeftRoundBracket");
		break;
	case TokenRightRoundBracket:
		printf("TokenRightRoundBracket");
		break;
	case TokenLeftSquareBracket:
		printf("TokenLeftSquareBracket");
		break;
	case TokenRightSquareBracket:
		printf("TokenRightSquareBracket");
		break;
	case TokenLeftVectorBracket:
		printf("TokenLeftVectorBracket");
		break;
	case TokenRightVectorBracket:
		printf("TokenRightVectorBracket");
		break;
	case TokenLeftCurlyBracket:
		printf("TokenLeftCurlyBracket");
		break;
	case TokenRightCurlyBracket:
		printf("TokenRightCurlyBracket");
		break;
	case TokenComma:
		printf("TokenComma");
		break;
	case TokenAdd:
		printf("TokenAdd");
		break;
	case TokenSubtract:
		printf("TokenSubtract");
		break;
	case TokenMultiply:
		printf("TokenMultiply");
		break;
	case TokenDivide:
		printf("TokenDivide");
		break;
	case TokenToPower:
		printf("TokenToPower");
		break;
	case TokenTranspose:
		printf("TokenTranspose");
		break;
	case TokenColon:
		printf("TokenColon");
		break;
	case TokenMatrixShape:
		printf("TokenMatrixShape");
		break;
	case TokenEqual:
		printf("TokenEqual");
		break;
	case TokenIdentifier:
		printf("TokenIdentifier");
		break;
	case TokenNumber:
		printf("TokenNumber");
		break;
	case TokenLet:
		printf("TokenLet");
		break;
	case TokenConst:
		printf("TokenConst");
		break;
	case TokenIf:
		printf("TokenIf");
		break;
	case TokenElse:
		printf("TokenElse");
		break;
	case TokenWhile:
		printf("TokenWhile");
		break;
	case TokenOr:
		printf("TokenOr");
		break;
	case TokenAnd:
		printf("TokenAnd");
		break;
	case TokenEqualEqual:
		printf("TokenEqualEqual");
		break;
	case TokenNotEqual:
		printf("TokenNotEqual");
		break;
	case TokenLess:
		printf("TokenLess");
		break;
	case TokenLessEqual:
		printf("TokenLessEqual");
		break;
	case TokenGreater:
		printf("TokenGreater");
		break;
	case TokenGreaterEqual:
		printf("TokenGreaterEqual");
		break;
	case TokenEof:
		printf("TokenEof");
		break;
	}
}

int main(int argc, char* argv[])
{
	printf("MxLang v" MX_VERSION "\n\n");

	if (argc < 2) {
		// TODO: Report errors meaningfully
		return 1;
	}

	if (SourceInit(argv[1])) {
		return 1;
	}

	printf("File contents:\n%.*s\n", (u32)g_source.SourceLength, g_source.Source);

	if (DiagInit()) {
		return 1;
	}

	if (TokenizerInit()) {
		return 1;
	}

	if (ParserInit()) {
		return 1;
	}

	if (ParserParse()) {
		return 1;
	}

	DiagReport();

	printf("Parsed!\n\n");

	ParserPrintAST((ASTNode*)g_parser.ASTArena.Blocks->Data, 0);

	if (ParserDeinit()) {
		return 1;
	}

	if (TokenizerDeinit()) {
		return 1;
	}

	if (DiagDeinit()) {
		return 1;
	}

	SourceDeinit();

	return 0;
}
