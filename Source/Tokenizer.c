#include "Tokenizer.h"

#include "Diagnostics.h"
#include "SourceManager.h"
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
	if (g_tokenizer.LexemeCurrent + lookahead >= g_tokenizer.SourceEnd) {
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

static Result TokenizerAddToken(TokenType type, const SymbolView* lexeme, double number)
{
	// Token* token;
	// Result result = StatArenaAlloc(&g_tokenizer.ArenaTokens, (void**)&token);
	// if (result) {
	// 	return result;
	// }

	g_tokenizer.LastReturnedToken.SourceLinePos = g_tokenizer.SourceLinePos - (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);
	g_tokenizer.LastReturnedToken.SourceLine = g_tokenizer.SourceLine;
	g_tokenizer.LastReturnedToken.Type = type;
	g_tokenizer.LastReturnedToken.Number = number;

	if (type == TokenIdentifier) {
		g_tokenizer.LastReturnedToken.Lexeme = *lexeme;
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

Result TokenizerInit()
{
	// Result result = StatArenaInit(&g_tokenizer.ArenaTokens, sizeof(Token));
	// if (result) {
	// 	return result;
	// }

	Result result = SymbolTableInit(&g_tokenizer.TableStrings);
	if (result) {
		// StatArenaDeinit(&g_tokenizer.ArenaTokens);
		return result;
	}

	g_tokenizer.SourceEnd = g_source.Source + g_source.SourceLength;
	g_tokenizer.SourceLine = 1;
	g_tokenizer.SourceLinePos = 1;
	g_tokenizer.LexemeStart = g_source.Source;
	g_tokenizer.LexemeCurrent = g_source.Source;

	g_source.Lines[0] = g_source.Source;

	return ResOk;
}

static void TokenizerSkipWhiteSpace()
{
	while (true) {
		char c = TokenizerPeek(0);

		switch (c) {
		case '#':
			while (TokenizerPeek(0) != '\0' && TokenizerPeek(0) != '\n') {
				TokenizerAdvance();
			}
			break;
		case ' ':
		case '\r':
		case '\t':
			break;
		case '\n':
			g_tokenizer.SourceLinePos = 1;
			++g_tokenizer.SourceLine;

			g_source.Lines[g_tokenizer.SourceLine - 1] = g_tokenizer.LexemeCurrent;
			break;
		default:
			return;
		}

		TokenizerAdvance();
	}
}

Result TokenizerPeekToken(Token** token)
{
	if (!g_tokenizer.HasLookahead) {
		TokenizerNextToken(token);
		g_tokenizer.HasLookahead = true;
		return ResOk;
	}

	*token = &g_tokenizer.LastReturnedToken;
	return ResOk;
}

Result TokenizerNextToken(Token** token)
{
	if (g_tokenizer.HasLookahead) {
		*token = &g_tokenizer.LastReturnedToken;
		g_tokenizer.HasLookahead = false;
		return ResOk;
	}

	TokenizerSkipWhiteSpace();

	if (g_tokenizer.LexemeCurrent >= g_tokenizer.SourceEnd) {
		TokenizerAddToken(TokenEof, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	}

	// TODO: Matrix shapes!!!

	g_tokenizer.LexemeStart = g_tokenizer.LexemeCurrent;

	char c = TokenizerConsume();

	switch (c) {
	case '(':
		TokenizerAddToken(TokenLeftRoundBracket, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case ')':
		TokenizerAddToken(TokenRightRoundBracket, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '[':
		TokenizerAddToken(TokenLeftSquareBracket, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case ']':
		TokenizerAddToken(TokenRightSquareBracket, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '<':
		if (TokenizerMatch('=')) {
			TokenizerAddToken(TokenLessEqual, nullptr, 0);
		} else if (TokenizerMatch('<')) {
			TokenizerAddToken(TokenLeftVectorBracket, nullptr, 0);
		} else {
			TokenizerAddToken(TokenLess, nullptr, 0);
		}
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '>':
		if (TokenizerMatch('=')) {
			TokenizerAddToken(TokenGreaterEqual, nullptr, 0);
		} else if (TokenizerMatch('>')) {
			TokenizerAddToken(TokenRightVectorBracket, nullptr, 0);
		} else {
			TokenizerAddToken(TokenGreater, nullptr, 0);
		}
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '{':
		TokenizerAddToken(TokenLeftCurlyBracket, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '}':
		TokenizerAddToken(TokenRightCurlyBracket, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case ',':
		TokenizerAddToken(TokenComma, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '+':
		TokenizerAddToken(TokenAdd, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '-':
		TokenizerAddToken(TokenSubtract, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '*':
		TokenizerAddToken(TokenMultiply, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '/':
		TokenizerAddToken(TokenDivide, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '^':
		TokenizerAddToken(TokenToPower, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '\'':
		TokenizerAddToken(TokenTranspose, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case ':': {
		TokenizerAddToken(TokenColon, nullptr, 0);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;

		while (isspace(TokenizerPeek(0))) {
			TokenizerAdvance();
		}

		g_tokenizer.LexemeStart = g_tokenizer.LexemeCurrent;

		if (!isdigit(TokenizerConsume())) {
			DIAG_EMIT(DiagExpectedTokenAfter, g_tokenizer.SourceLine, g_tokenizer.SourceLinePos - 1, DIAG_ARG_STRING("matrix shape"),
				DIAG_ARG_CHAR(':'));

			TokenizerSkipAlphanumeric();

			break;
		}

		TokenizerSkipDigit();

		if (!TokenizerMatch('x')) {
			DIAG_EMIT(DiagExpectedTokenAfter, g_tokenizer.SourceLine, g_tokenizer.SourceLinePos - 1, DIAG_ARG_STRING("matrix shape"),
				DIAG_ARG_CHAR(':'));

			TokenizerSkipAlphanumeric();

			break;
		}

		if (!isdigit(TokenizerConsume())) {
			DIAG_EMIT(DiagExpectedTokenAfter, g_tokenizer.SourceLine, g_tokenizer.SourceLinePos - 1, DIAG_ARG_STRING("matrix shape"),
				DIAG_ARG_CHAR(':'));

			TokenizerSkipAlphanumeric();

			break;
		}

		TokenizerSkipDigit();

		usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);
		SymbolView lexeme;
		Result result = SymbolTableAdd(&g_tokenizer.TableStrings, g_tokenizer.LexemeStart, lexemeLength, &lexeme);
		if (result) {
			return result;
		}
		TokenizerAddToken(TokenMatrixShape, &lexeme, 0);
	} break;
	case '=':
		if (TokenizerMatch('=')) {
			TokenizerAddToken(TokenEqualEqual, nullptr, 0);
		} else {
			TokenizerAddToken(TokenEqual, nullptr, 0);
		}
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '!':
		if (TokenizerMatch('=')) {
			TokenizerAddToken(TokenNotEqual, nullptr, 0);
		} else {
			DIAG_EMIT(
				DiagExpectedTokenAfter, g_tokenizer.SourceLine, g_tokenizer.SourceLinePos - 1, DIAG_ARG_CHAR('='), DIAG_ARG_CHAR('!'));
		}
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	default:
		if (isdigit(c)) {
			while (isdigit(TokenizerPeek(0))) {
				TokenizerAdvance();
			}

			// TODO: Fix this, edge cases not accounted for
			if (TokenizerMatch('.') && isdigit(TokenizerPeek(0))) {
				TokenizerSkipDigit();
			}

			usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);
			TokenizerAddToken(TokenNumber, nullptr, ParseDouble(g_tokenizer.LexemeStart, lexemeLength));
		} else if (isalpha(c)) {
			TokenizerSkipAlphanumeric();

			usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);

			if (lexemeLength == 3 && memcmp(g_tokenizer.LexemeStart, "let", lexemeLength) == 0) {
				TokenizerAddToken(TokenLet, nullptr, 0);
			} else if (lexemeLength == 5 && memcmp(g_tokenizer.LexemeStart, "const", lexemeLength) == 0) {
				TokenizerAddToken(TokenConst, nullptr, 0);
			} else if (lexemeLength == 2 && memcmp(g_tokenizer.LexemeStart, "if", lexemeLength) == 0) {
				TokenizerAddToken(TokenIf, nullptr, 0);
			} else if (lexemeLength == 4 && memcmp(g_tokenizer.LexemeStart, "else", lexemeLength) == 0) {
				TokenizerAddToken(TokenElse, nullptr, 0);
			} else if (lexemeLength == 5 && memcmp(g_tokenizer.LexemeStart, "while", lexemeLength) == 0) {
				TokenizerAddToken(TokenWhile, nullptr, 0);
			} else if (lexemeLength == 3 && memcmp(g_tokenizer.LexemeStart, "and", lexemeLength) == 0) {
				TokenizerAddToken(TokenAnd, nullptr, 0);
			} else if (lexemeLength == 2 && memcmp(g_tokenizer.LexemeStart, "or", lexemeLength) == 0) {
				TokenizerAddToken(TokenOr, nullptr, 0);
			} else {
				SymbolView lexeme;
				Result result = SymbolTableAdd(&g_tokenizer.TableStrings, g_tokenizer.LexemeStart, lexemeLength, &lexeme);
				if (result) {
					return result;
				}
				TokenizerAddToken(TokenIdentifier, &lexeme, 0);
			}
		}
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	}

	DIAG_EMIT(DiagUnexpectedToken, g_tokenizer.SourceLine, g_tokenizer.SourceLinePos - 1, DIAG_ARG_CHAR(c));
	*token = nullptr;
	return ResOk;
}

// Result TokenizerScan()
// {
// 	while (g_tokenizer.LexemeCurrent < g_tokenizer.SourceEnd) {
// 		g_tokenizer.LexemeStart = g_tokenizer.LexemeCurrent;
//
// 		char c = TokenizerConsume();
// 		switch (c) {
// 		case '#':
// 			while (TokenizerPeek(0) != '\0' && TokenizerPeek(0) != '\n') {
// 				TokenizerAdvance();
// 			}
// 			break;
// 		case '(':
// 			TokenizerAddToken(TokenLeftRoundBracket, nullptr, 0);
// 			break;
// 		case ')':
// 			TokenizerAddToken(TokenRightRoundBracket, nullptr, 0);
// 			break;
// 		case '[':
// 			TokenizerAddToken(TokenLeftSquareBracket, nullptr, 0);
// 			break;
// 		case ']':
// 			TokenizerAddToken(TokenRightSquareBracket, nullptr, 0);
// 			break;
// 		case '<':
// 			if (TokenizerMatch('=')) {
// 				TokenizerAddToken(TokenLessEqual, nullptr, 0);
// 			} else if (TokenizerMatch('<')) {
// 				TokenizerAddToken(TokenLeftVectorBracket, nullptr, 0);
// 			} else {
// 				TokenizerAddToken(TokenLess, nullptr, 0);
// 			}
// 			break;
// 		case '>':
// 			if (TokenizerMatch('=')) {
// 				TokenizerAddToken(TokenGreaterEqual, nullptr, 0);
// 			} else if (TokenizerMatch('>')) {
// 				TokenizerAddToken(TokenRightVectorBracket, nullptr, 0);
// 			} else {
// 				TokenizerAddToken(TokenGreater, nullptr, 0);
// 			}
// 			break;
// 		case '{':
// 			TokenizerAddToken(TokenLeftCurlyBracket, nullptr, 0);
// 			break;
// 		case '}':
// 			TokenizerAddToken(TokenRightCurlyBracket, nullptr, 0);
// 			break;
// 		case ',':
// 			TokenizerAddToken(TokenComma, nullptr, 0);
// 			break;
// 		case '+':
// 			TokenizerAddToken(TokenAdd, nullptr, 0);
// 			break;
// 		case '-':
// 			TokenizerAddToken(TokenSubtract, nullptr, 0);
// 			break;
// 		case '*':
// 			TokenizerAddToken(TokenMultiply, nullptr, 0);
// 			break;
// 		case '/':
// 			TokenizerAddToken(TokenDivide, nullptr, 0);
// 			break;
// 		case '^':
// 			TokenizerAddToken(TokenToPower, nullptr, 0);
// 			break;
// 		case '\'':
// 			TokenizerAddToken(TokenTranspose, nullptr, 0);
// 			break;
// 		case ':': {
// 			TokenizerAddToken(TokenColon, nullptr, 0);
//
// 			while (isspace(TokenizerPeek(0))) {
// 				TokenizerAdvance();
// 			}
//
// 			g_tokenizer.LexemeStart = g_tokenizer.LexemeCurrent;
//
// 			if (!isdigit(TokenizerConsume())) {
// 				DIAG_EMIT(DiagExpectedTokenAfter, g_tokenizer.SourceLine, g_tokenizer.SourceLinePos - 1, DIAG_ARG_STRING("matrix shape"),
// 					DIAG_ARG_CHAR(':'));
//
// 				TokenizerSkipAlphanumeric();
//
// 				break;
// 			}
//
// 			TokenizerSkipDigit();
//
// 			if (!TokenizerMatch('x')) {
// 				DIAG_EMIT(DiagExpectedTokenAfter, g_tokenizer.SourceLine, g_tokenizer.SourceLinePos - 1, DIAG_ARG_STRING("matrix shape"),
// 					DIAG_ARG_CHAR(':'));
//
// 				TokenizerSkipAlphanumeric();
//
// 				break;
// 			}
//
// 			if (!isdigit(TokenizerConsume())) {
// 				DIAG_EMIT(DiagExpectedTokenAfter, g_tokenizer.SourceLine, g_tokenizer.SourceLinePos - 1, DIAG_ARG_STRING("matrix shape"),
// 					DIAG_ARG_CHAR(':'));
//
// 				TokenizerSkipAlphanumeric();
//
// 				break;
// 			}
//
// 			TokenizerSkipDigit();
//
// 			usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);
// 			SymbolView lexeme;
// 			Result result = SymbolTableAdd(&g_tokenizer.TableStrings, g_tokenizer.LexemeStart, lexemeLength, &lexeme);
// 			if (result) {
// 				return result;
// 			}
// 			TokenizerAddToken(TokenMatrixShape, &lexeme, 0);
// 		} break;
// 		case '=':
// 			if (TokenizerMatch('=')) {
// 				TokenizerAddToken(TokenEqualEqual, nullptr, 0);
// 			} else {
// 				TokenizerAddToken(TokenEqual, nullptr, 0);
// 			}
// 			break;
// 		case '!':
// 			if (TokenizerMatch('=')) {
// 				TokenizerAddToken(TokenNotEqual, nullptr, 0);
// 			} else {
// 				DIAG_EMIT(
// 					DiagExpectedTokenAfter, g_tokenizer.SourceLine, g_tokenizer.SourceLinePos - 1, DIAG_ARG_CHAR('='), DIAG_ARG_CHAR('!'));
// 			}
// 			break;
// 		case ' ':
// 		case '\r':
// 		case '\t':
// 			break;
// 		case '\n':
// 			g_tokenizer.SourceLinePos = 1;
// 			++g_tokenizer.SourceLine;
//
// 			g_source.Lines[g_tokenizer.SourceLine - 1] = g_tokenizer.LexemeCurrent;
// 			break;
// 		default:
// 			if (isdigit(c)) {
// 				while (isdigit(TokenizerPeek(0))) {
// 					TokenizerAdvance();
// 				}
//
// 				if (TokenizerMatch('.') && isdigit(TokenizerPeek(0))) {
// 					TokenizerSkipDigit();
// 				}
//
// 				usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);
// 				TokenizerAddToken(TokenNumber, nullptr, ParseDouble(g_tokenizer.LexemeStart, lexemeLength));
// 			} else if (isalpha(c)) {
// 				TokenizerSkipAlphanumeric();
//
// 				usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);
//
// 				if (lexemeLength == 3 && memcmp(g_tokenizer.LexemeStart, "let", lexemeLength) == 0) {
// 					TokenizerAddToken(TokenLet, nullptr, 0);
// 				} else if (lexemeLength == 5 && memcmp(g_tokenizer.LexemeStart, "const", lexemeLength) == 0) {
// 					TokenizerAddToken(TokenConst, nullptr, 0);
// 				} else if (lexemeLength == 2 && memcmp(g_tokenizer.LexemeStart, "if", lexemeLength) == 0) {
// 					TokenizerAddToken(TokenIf, nullptr, 0);
// 				} else if (lexemeLength == 4 && memcmp(g_tokenizer.LexemeStart, "else", lexemeLength) == 0) {
// 					TokenizerAddToken(TokenElse, nullptr, 0);
// 				} else if (lexemeLength == 5 && memcmp(g_tokenizer.LexemeStart, "while", lexemeLength) == 0) {
// 					TokenizerAddToken(TokenWhile, nullptr, 0);
// 				} else if (lexemeLength == 3 && memcmp(g_tokenizer.LexemeStart, "and", lexemeLength) == 0) {
// 					TokenizerAddToken(TokenAnd, nullptr, 0);
// 				} else if (lexemeLength == 2 && memcmp(g_tokenizer.LexemeStart, "or", lexemeLength) == 0) {
// 					TokenizerAddToken(TokenOr, nullptr, 0);
// 				} else {
// 					SymbolView lexeme;
// 					Result result = SymbolTableAdd(&g_tokenizer.TableStrings, g_tokenizer.LexemeStart, lexemeLength, &lexeme);
// 					if (result) {
// 						return result;
// 					}
// 					TokenizerAddToken(TokenIdentifier, &lexeme, 0);
// 				}
// 			} else {
// 				DIAG_EMIT(DiagUnexpectedToken, g_tokenizer.SourceLine, g_tokenizer.SourceLinePos - 1, DIAG_ARG_CHAR(c));
// 			}
// 			break;
// 		}
// 	}
//
// 	TokenizerAddToken(TokenEof, nullptr, 0);
//
// 	return ResOk;
// }
