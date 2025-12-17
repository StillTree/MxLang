#include "Tokenizer.h"

#include "Memory/Arena.h"
#include <ctype.h>
#include <math.h>
#include <string.h>

static double ParseDouble(const char* str, size_t strLength)
{
	const char* strEnd = str + strLength;
	double value = 0.0;

	while (str < strEnd && *str >= '0' && *str <= '9') {
		value *= 10;
		value += *str++ - '0';
	}

	if (str < strEnd && *str == '.') {
		double fraction = 0.1;
		str++;

		while (str < strEnd && *str >= '0' && *str <= '9') {
			value += (*str++ - '0') * fraction;
			fraction /= 10;
		}
	}

	if (str < strEnd && (*str == 'e' || *str == 'E')) {
		int exponent = 0;
		int exponentSign = 1;
		str++;

		if (*str == '-') {
			exponentSign = -1;
			str++;
		} else if (*str == '+') {
			str++;
		}

		while (str < strEnd && *str >= '0' && *str <= '9') {
			exponent *= 10;
			exponent += *str++ - '0';
		}

		value *= pow(10.0, exponentSign * exponent);
	}

	return value;
}

Tokenizer g_tokenizer = { 0 };

static char TokenizerConsume()
{
	++g_tokenizer.SourceLinePos;
	return *g_tokenizer.LexemeCurrent++;
}

static void TokenizerAdvance()
{
	++g_tokenizer.SourceLinePos;
	g_tokenizer.LexemeCurrent++;
}

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

static Result TokenizerAddToken(TokenType type, const char* lexeme, double number)
{
	Token* token;
	Result result = ArenaAlloc(&g_tokenizer.ArenaTokens, (void**)&token, sizeof(Token));
	if (!result) {
		return result;
	}

	token->SourceLinePos = g_tokenizer.SourceLinePos;
	token->SourceLine = g_tokenizer.SourceLine;
	token->Type = type;

	if (type == TokenNumber) {
		token->Number = number;
	} else {
		token->Lexeme = lexeme;
	}

	return ResOk;
}

inline static void TokenizerSkipDigit()
{
	while (isdigit(TokenizerPeek(0))) {
		TokenizerAdvance();
	}
}

inline static void TokenizerSkipAlphanumeric()
{
	while (isalnum(TokenizerPeek(0))) {
		TokenizerAdvance();
	}
}

Result TokenizerInit(const char* source)
{
	Result result = ArenaInit(&g_tokenizer.ArenaTokens);
	if (result) {
		return result;
	}

	result = SymbolTableInit(&g_tokenizer.TableStrings);
	if (result) {
		ArenaDeinit(&g_tokenizer.ArenaTokens);
		return result;
	}

	g_tokenizer.Source = source;
	g_tokenizer.SourceEnd = source + strlen(source);
	g_tokenizer.SourceLine = 1;
	g_tokenizer.SourceLinePos = 1;
	g_tokenizer.LexemeStart = source;
	g_tokenizer.LexemeCurrent = source;

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
			TokenizerAddToken(TokenLeftRoundBracket, nullptr, 0);
			break;
		case ')':
			TokenizerAddToken(TokenRightRoundBracket, nullptr, 0);
			break;
		case '[':
			TokenizerAddToken(TokenLeftSquareBracket, nullptr, 0);
			break;
		case ']':
			TokenizerAddToken(TokenRightSquareBracket, nullptr, 0);
			break;
		case '<':
			if (TokenizerMatch('=')) {
				TokenizerAddToken(TokenLessEqual, nullptr, 0);
			} else if (TokenizerMatch('<')) {
				TokenizerAddToken(TokenLeftVectorBracket, nullptr, 0);
			} else {
				TokenizerAddToken(TokenLess, nullptr, 0);
			}
			break;
		case '>':
			if (TokenizerMatch('=')) {
				TokenizerAddToken(TokenGreaterEqual, nullptr, 0);
			} else if (TokenizerMatch('>')) {
				TokenizerAddToken(TokenRightVectorBracket, nullptr, 0);
			} else {
				TokenizerAddToken(TokenGreater, nullptr, 0);
			}
			break;
		case '{':
			TokenizerAddToken(TokenLeftCurlyBracket, nullptr, 0);
			break;
		case '}':
			TokenizerAddToken(TokenRightCurlyBracket, nullptr, 0);
			break;
		case ',':
			TokenizerAddToken(TokenComma, nullptr, 0);
			break;
		case '+':
			TokenizerAddToken(TokenAdd, nullptr, 0);
			break;
		case '-':
			TokenizerAddToken(TokenSubtract, nullptr, 0);
			break;
		case '*':
			TokenizerAddToken(TokenMultiply, nullptr, 0);
			break;
		case '/':
			TokenizerAddToken(TokenDivide, nullptr, 0);
			break;
		case '^':
			TokenizerAddToken(TokenToPower, nullptr, 0);
			break;
		case '\'':
			TokenizerAddToken(TokenTranspose, nullptr, 0);
			break;
		case ':': {
			TokenizerAddToken(TokenColon, nullptr, 0);

			while (isspace(TokenizerPeek(0))) {
				TokenizerAdvance();
			}

			g_tokenizer.LexemeStart = g_tokenizer.LexemeCurrent;

			if (!isdigit(TokenizerConsume())) {
				// TODO: Error out

				TokenizerSkipAlphanumeric();

				break;
			}

			TokenizerSkipDigit();

			if (!TokenizerMatch('x')) {
				// TODO: Error out

				TokenizerSkipAlphanumeric();

				break;
			}

			if (!isdigit(TokenizerConsume())) {
				// TODO: Error out

				TokenizerSkipAlphanumeric();

				break;
			}

			TokenizerSkipDigit();

			usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);
			const char* lexeme;
			Result result = SymbolTableAdd(&g_tokenizer.TableStrings, g_tokenizer.LexemeStart, lexemeLength, &lexeme);
			if (result) {
				// TODO: Error out
			}
			TokenizerAddToken(TokenMatrixShape, lexeme, 0);
		} break;
		case '=':
			if (TokenizerMatch('=')) {
				TokenizerAddToken(TokenEqualEqual, nullptr, 0);
			} else {
				TokenizerAddToken(TokenEqual, nullptr, 0);
			}
			break;
		case '!':
			if (TokenizerMatch('=')) {
				TokenizerAddToken(TokenNotEqual, nullptr, 0);
			} else {
				// TODO: Error out
			}
			break;
		case ' ':
		case '\r':
		case '\t':
			break;
		case '\n':
			g_tokenizer.SourceLinePos = 1;
			++g_tokenizer.SourceLine;
			break;
		default:
			if (isdigit(c)) {
				while (isdigit(TokenizerPeek(0))) {
					TokenizerAdvance();
				}

				if (TokenizerMatch('.') && isdigit(TokenizerPeek(0))) {
					TokenizerSkipDigit();
				}

				usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);
				TokenizerAddToken(TokenNumber, nullptr, ParseDouble(g_tokenizer.LexemeStart, lexemeLength));
			} else if (isalpha(c)) {
				TokenizerSkipAlphanumeric();

				usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);

				if (memcmp(g_tokenizer.LexemeStart, "let", lexemeLength) == 0) {
					TokenizerAddToken(TokenLet, nullptr, 0);
				} else if (memcmp(g_tokenizer.LexemeStart, "const", lexemeLength) == 0) {
					TokenizerAddToken(TokenConst, nullptr, 0);
				} else if (memcmp(g_tokenizer.LexemeStart, "if", lexemeLength) == 0) {
					TokenizerAddToken(TokenIf, nullptr, 0);
				} else if (memcmp(g_tokenizer.LexemeStart, "else", lexemeLength) == 0) {
					TokenizerAddToken(TokenElse, nullptr, 0);
				} else if (memcmp(g_tokenizer.LexemeStart, "while", lexemeLength) == 0) {
					TokenizerAddToken(TokenWhile, nullptr, 0);
				} else if (memcmp(g_tokenizer.LexemeStart, "and", lexemeLength) == 0) {
					TokenizerAddToken(TokenAnd, nullptr, 0);
				} else if (memcmp(g_tokenizer.LexemeStart, "or", lexemeLength) == 0) {
					TokenizerAddToken(TokenOr, nullptr, 0);
				} else {
					const char* lexeme;
					Result result = SymbolTableAdd(&g_tokenizer.TableStrings, g_tokenizer.LexemeStart, lexemeLength, &lexeme);
					if (result) {
						// TODO: Error out
					}
					TokenizerAddToken(TokenIdentifier, lexeme, 0);
				}
			} else {
				// printf("Unexpected character '%c' at line %lu.\n", c, g_scanner.Line);
				// TODO: Error out
			}
			break;
		}
	}

	TokenizerAddToken(TokenEof, nullptr, 0);

	return ResOk;
}
