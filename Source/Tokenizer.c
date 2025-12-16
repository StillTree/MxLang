#include "Tokenizer.h"

#include "Memory/Arena.h"
#include <ctype.h>
#include <string.h>

Tokenizer g_tokenizer = { 0 };

static char TokenizerConsume() { return *g_tokenizer.LexemeCurrent++; }

static void TokenizerAdvance() { g_tokenizer.LexemeCurrent++; }

static char TokenizerPeek(usz lookahead)
{
	if (g_tokenizer.Source >= g_tokenizer.SourceEnd) {
		return '\0';
	}

	return g_tokenizer.LexemeCurrent[lookahead];
}

static bool TokenizerMatch(char expected)
{
	char c = TokenizerPeek(0);

	if (c != expected) {
		return false;
	}

	TokenizerAdvance();
	return true;
}

static void TokenizerAddToken(TokenType t, double d) { }

Result TokenizerInit(const char* source)
{
	Result result = ArenaInit(&g_tokenizer.ArenaTokens);
	if (result) {
		return result;
	}

	g_tokenizer.Source = source;
	g_tokenizer.SourceEnd = source + strlen(source);
	g_tokenizer.SourceLine = 1;
	g_tokenizer.LexemeCurrent = source;
	g_tokenizer.LexemeStart = source;

	return ResOk;
}

Result TokenizerScan()
{
	while (g_tokenizer.LexemeCurrent < g_tokenizer.SourceEnd) {
		g_tokenizer.LexemeStart = g_tokenizer.LexemeCurrent;

		char c = TokenizerConsume();
		switch (c) {
		case '#':
			while (TokenizerPeek(0) != '\0' && TokenizerPeek(0) != '\n') {
				TokenizerAdvance();
			}
			break;
		case '(':
			TokenizerAddToken(TokenLeftRoundBracket, 0);
			break;
		case ')':
			TokenizerAddToken(TokenRightRoundBracket, 0);
			break;
		case '[':
			TokenizerAddToken(TokenLeftSquareBracket, 0);
			break;
		case ']':
			TokenizerAddToken(TokenRightSquareBracket, 0);
			break;
		case '<':
			if (TokenizerMatch('=')) {
				TokenizerAddToken(TokenLessEqual, 0);
			} else if (TokenizerMatch('<')) {
				TokenizerAddToken(TokenLeftVectorBracket, 0);
			} else {
				TokenizerAddToken(TokenLess, 0);
			}
			break;
		case '>':
			if (TokenizerMatch('=')) {
				TokenizerAddToken(TokenGreaterEqual, 0);
			} else if (TokenizerMatch('>')) {
				TokenizerAddToken(TokenRightVectorBracket, 0);
			} else {
				TokenizerAddToken(TokenGreater, 0);
			}
			break;
		case '{':
			TokenizerAddToken(TokenLeftCurlyBracket, 0);
			break;
		case '}':
			TokenizerAddToken(TokenRightCurlyBracket, 0);
			break;
		case ',':
			TokenizerAddToken(TokenComma, 0);
			break;
		case '+':
			TokenizerAddToken(TokenAdd, 0);
			break;
		case '-':
			TokenizerAddToken(TokenSubtract, 0);
			break;
		case '*':
			TokenizerAddToken(TokenMultiply, 0);
			break;
		case '/':
			TokenizerAddToken(TokenDivide, 0);
			break;
		case '^':
			TokenizerAddToken(TokenToPower, 0);
			break;
		case '\'':
			TokenizerAddToken(TokenTranspose, 0);
			break;
		case ':': {
			TokenizerAddToken(TokenColon, 0);

			while (isspace(TokenizerPeek(0))) {
				TokenizerAdvance();
			}

			g_tokenizer.LexemeStart = g_tokenizer.LexemeCurrent;

			if (!isdigit(TokenizerConsume())) {
				// TODO: Error out

				while (isalnum(TokenizerPeek(0))) {
					TokenizerAdvance();
				}

				break;
			}

			while (isdigit(TokenizerPeek(0))) {
				TokenizerAdvance();
			}

			if (TokenizerConsume() != 'x') {
				// TODO: Error out

				while (isalnum(TokenizerPeek(0))) {
					TokenizerAdvance();
				}

				break;
			}

			if (!isdigit(TokenizerConsume())) {
				// TODO: Error out

				while (isalnum(TokenizerPeek(0))) {
					TokenizerAdvance();
				}

				break;
			}

			while (isdigit(TokenizerPeek(0))) {
				TokenizerAdvance();
			}

			// TODO: Finish this
			// char* lexeme = malloc(g_scanner.LexemeCurrent - g_scanner.LexemeStart + 1);
			// lexeme[g_scanner.LexemeCurrent - g_scanner.LexemeStart] = '\0';
			// memcpy(lexeme, g_scanner.Source + g_scanner.LexemeStart, g_scanner.LexemeCurrent - g_scanner.LexemeStart);
			// g_scanner.Tokens[g_scanner.TokensFreeIndex].Lexeme = lexeme;

			// g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = TokenMatrixShape;
			// g_scanner.Tokens[g_scanner.TokensFreeIndex].Number = 0;
			// g_scanner.Tokens[g_scanner.TokensFreeIndex].SourceLine = g_scanner.Line;
		} break;
		case '=':
			if (TokenizerMatch('=')) {
				TokenizerAddToken(TokenEqualEqual, 0);
			} else {
				TokenizerAddToken(TokenEqual, 0);
			}
			break;
		case '!':
			if (TokenizerMatch('=')) {
				TokenizerAddToken(TokenNotEqual, 0);
			} else {
				// TODO: Error out
			}
			break;
		case ' ':
		case '\r':
		case '\t':
			break;
		case '\n':
			++g_tokenizer.SourceLine;
			break;
		default:
			if (isdigit(c)) {
				while (isdigit(g_scanner.Source[g_scanner.LexemeCurrent])) {
					++g_scanner.LexemeCurrent;
				}

				if (g_scanner.Source[g_scanner.LexemeCurrent] == '.' && isdigit(g_scanner.Source[g_scanner.LexemeCurrent + 1])) {
					++g_scanner.LexemeCurrent;

					while (isdigit(g_scanner.Source[g_scanner.LexemeCurrent])) {
						++g_scanner.LexemeCurrent;
					}
				}

				char* lexeme = malloc(g_scanner.LexemeCurrent - g_scanner.LexemeStart + 1);
				lexeme[g_scanner.LexemeCurrent - g_scanner.LexemeStart] = '\0';
				memcpy(lexeme, g_scanner.Source + g_scanner.LexemeStart, g_scanner.LexemeCurrent - g_scanner.LexemeStart);
				g_scanner.Tokens[g_scanner.TokensFreeIndex].Lexeme = lexeme;

				g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = TokenNumber;
				g_scanner.Tokens[g_scanner.TokensFreeIndex].Number = atof(lexeme);
				g_scanner.Tokens[g_scanner.TokensFreeIndex].SourceLine = g_scanner.Line;

				++g_scanner.TokensFreeIndex;
			} else if (isalpha(c)) {
				while (isalnum(g_scanner.Source[g_scanner.LexemeCurrent])) {
					++g_scanner.LexemeCurrent;
				}

				char* lexeme = malloc(g_scanner.LexemeCurrent - g_scanner.LexemeStart + 1);
				lexeme[g_scanner.LexemeCurrent - g_scanner.LexemeStart] = '\0';
				memcpy(lexeme, g_scanner.Source + g_scanner.LexemeStart, g_scanner.LexemeCurrent - g_scanner.LexemeStart);
				g_scanner.Tokens[g_scanner.TokensFreeIndex].Lexeme = lexeme;

				g_scanner.Tokens[g_scanner.TokensFreeIndex].Number = 0;
				g_scanner.Tokens[g_scanner.TokensFreeIndex].SourceLine = g_scanner.Line;

				if (strcmp(lexeme, "let") == 0) {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = TokenLet;
				} else if (strcmp(lexeme, "const") == 0) {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = TokenConst;
				} else if (strcmp(lexeme, "if") == 0) {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = TokenIf;
				} else if (strcmp(lexeme, "while") == 0) {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = TokenWhile;
				} else if (strcmp(lexeme, "else") == 0) {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = TokenElse;
				} else if (strcmp(lexeme, "and") == 0) {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = TokenAnd;
				} else if (strcmp(lexeme, "or") == 0) {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = TokenOr;
				} else {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = TokenIdentifier;
				}

				++g_scanner.TokensFreeIndex;
			} else {
				printf("Unexpected character '%c' at line %lu.\n", c, g_scanner.Line);
			}
			break;
		}
	}

	g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = TokenEof;
	g_scanner.Tokens[g_scanner.TokensFreeIndex].Lexeme = nullptr;
	g_scanner.Tokens[g_scanner.TokensFreeIndex].SourceLine = g_scanner.Line;

	++g_scanner.TokensFreeIndex;
	return ResOk;
}
