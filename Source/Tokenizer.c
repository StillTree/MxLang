#include "Tokenizer.h"

#include "SourceManager.h"
#include <ctype.h>
#include <math.h>
#include <string.h>

static MatrixShape ParseMatrixShape(const char* str, usz strLength)
{
	const char* strEnd = str + strLength;
	MatrixShape shape = { 0 };

	while (*str != 'x') {
		shape.Height *= 10;
		shape.Height += (u64)*str - '0';

		++str;
	}

	++str;

	while (str < strEnd) {
		shape.Width *= 10;
		shape.Width += (u64)*str - '0';

		++str;
	}

	return shape;
}

static f64 ParseDouble(const char* str, usz strLength)
{
	const char* strEnd = str + strLength;
	f64 value = 0.0;

	while (str < strEnd && *str >= '0' && *str <= '9') {
		value *= 10;
		value += *str++ - '0';
	}

	if (str < strEnd && *str == '.') {
		f64 fraction = 0.1;
		str++;

		while (str < strEnd && *str >= '0' && *str <= '9') {
			value += (*str++ - '0') * fraction;
			fraction /= 10;
		}
	}

	if (str < strEnd && (*str == 'e' || *str == 'E')) {
		i64 exponent = 0;
		i64 exponentSign = 1;
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

		value *= pow(10.0, (f64)exponentSign * (f64)exponent);
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
	++g_tokenizer.LexemeCurrent;
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

// TODO: Make this function not trash
static void TokenizerAddToken(TokenType type, const SymbolView* lexeme, f64 number, const MatrixShape* matrixShape)
{
	if (type == TokenEof) {
		g_tokenizer.LastReturnedToken.Loc.LinePos = (usz)(g_tokenizer.SourceEnd - g_source.Lines[g_tokenizer.SourceLine - 1] + 1);
	} else {
		g_tokenizer.LastReturnedToken.Loc.LinePos
			= g_tokenizer.SourceLinePos - (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);
	}

	g_tokenizer.LastReturnedToken.Loc.Line = g_tokenizer.SourceLine;
	g_tokenizer.LastReturnedToken.Type = type;

	if (type == TokenNumber) {
		g_tokenizer.LastReturnedToken.Number = number;
	} else if (type == TokenIdentifier) {
		g_tokenizer.LastReturnedToken.Lexeme = *lexeme;
	} else if (type == TokenMatrixShape) {
		g_tokenizer.LastReturnedToken.MatrixShape = *matrixShape;
	}
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
	Result result = SymbolTableInit(&g_tokenizer.TableIdentifiers);
	if (result) {
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
			g_source.Lines[g_tokenizer.SourceLine] = g_tokenizer.LexemeCurrent + 1;

			g_tokenizer.SourceLinePos = 0;
			++g_tokenizer.SourceLine;

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
		Result result = TokenizerNextToken(token);
		g_tokenizer.HasLookahead = true;
		return result;
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
		TokenizerAddToken(TokenEof, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	}

	g_tokenizer.LexemeStart = g_tokenizer.LexemeCurrent;

	char c = TokenizerConsume();

	switch (c) {
	case '(':
		TokenizerAddToken(TokenLeftRoundBracket, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case ')':
		TokenizerAddToken(TokenRightRoundBracket, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '[':
		TokenizerAddToken(TokenLeftSquareBracket, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case ']':
		TokenizerAddToken(TokenRightSquareBracket, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '<':
		if (TokenizerMatch('=')) {
			TokenizerAddToken(TokenLessEqual, nullptr, 0, nullptr);
		} else if (TokenizerMatch('<')) {
			TokenizerAddToken(TokenLeftVectorBracket, nullptr, 0, nullptr);
		} else {
			TokenizerAddToken(TokenLess, nullptr, 0, nullptr);
		}
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '>':
		if (TokenizerMatch('=')) {
			TokenizerAddToken(TokenGreaterEqual, nullptr, 0, nullptr);
		} else if (TokenizerMatch('>')) {
			TokenizerAddToken(TokenRightVectorBracket, nullptr, 0, nullptr);
		} else {
			TokenizerAddToken(TokenGreater, nullptr, 0, nullptr);
		}
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '{':
		TokenizerAddToken(TokenLeftCurlyBracket, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '}':
		TokenizerAddToken(TokenRightCurlyBracket, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case ',':
		TokenizerAddToken(TokenComma, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '+':
		TokenizerAddToken(TokenAdd, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '-':
		TokenizerAddToken(TokenSubtract, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '*':
		TokenizerAddToken(TokenMultiply, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '/':
		TokenizerAddToken(TokenDivide, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '^':
		TokenizerAddToken(TokenToPower, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '\'':
		TokenizerAddToken(TokenTranspose, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case ':':
		TokenizerAddToken(TokenColon, nullptr, 0, nullptr);
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '=':
		if (TokenizerMatch('=')) {
			TokenizerAddToken(TokenEqualEqual, nullptr, 0, nullptr);
		} else {
			TokenizerAddToken(TokenEqual, nullptr, 0, nullptr);
		}
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	case '!':
		if (TokenizerMatch('=')) {
			TokenizerAddToken(TokenNotEqual, nullptr, 0, nullptr);
		} else {
			TokenizerAddToken((TokenType)TokenizerPeek(0), nullptr, 0, nullptr);
		}
		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	default:
		if (isdigit(c)) {
			while (isdigit(TokenizerPeek(0))) {
				TokenizerAdvance();
			}

			if (TokenizerMatch('x')) {
				if (!isdigit(TokenizerPeek(0))) {
					TokenizerAddToken((TokenType)TokenizerPeek(0), nullptr, 0, nullptr);
					*token = &g_tokenizer.LastReturnedToken;
					return ResOk;
				}

				TokenizerSkipDigit();

				if (isalpha(TokenizerPeek(0))) {
					TokenizerAddToken((TokenType)TokenizerPeek(0), nullptr, 0, nullptr);
					*token = &g_tokenizer.LastReturnedToken;
					return ResOk;
				}

				usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);
				MatrixShape shape = ParseMatrixShape(g_tokenizer.LexemeStart, lexemeLength);
				TokenizerAddToken(TokenMatrixShape, nullptr, 0, &shape);
			} else if (TokenizerMatch('.')) {
				if (!isdigit(TokenizerPeek(0))) {
					TokenizerAddToken((TokenType)TokenizerPeek(0), nullptr, 0, nullptr);
					*token = &g_tokenizer.LastReturnedToken;
					return ResOk;
				}

				TokenizerSkipDigit();

				if (isalpha(TokenizerPeek(0))) {
					TokenizerAddToken((TokenType)TokenizerPeek(0), nullptr, 0, nullptr);
					*token = &g_tokenizer.LastReturnedToken;
					return ResOk;
				}

				usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);
				TokenizerAddToken(TokenNumber, nullptr, ParseDouble(g_tokenizer.LexemeStart, lexemeLength), nullptr);
			} else if (isalpha(TokenizerPeek(0))) {
				TokenizerAddToken((TokenType)TokenizerPeek(0), nullptr, 0, nullptr);
				*token = &g_tokenizer.LastReturnedToken;
				return ResOk;
			} else {
				usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);
				TokenizerAddToken(TokenNumber, nullptr, ParseDouble(g_tokenizer.LexemeStart, lexemeLength), nullptr);
			}
			*token = &g_tokenizer.LastReturnedToken;
			return ResOk;
		} else if (isalpha(c)) {
			TokenizerSkipAlphanumeric();

			usz lexemeLength = (usz)(g_tokenizer.LexemeCurrent - g_tokenizer.LexemeStart);

			if (lexemeLength == 3 && memcmp(g_tokenizer.LexemeStart, "let", lexemeLength) == 0) {
				TokenizerAddToken(TokenLet, nullptr, 0, nullptr);
			} else if (lexemeLength == 5 && memcmp(g_tokenizer.LexemeStart, "const", lexemeLength) == 0) {
				TokenizerAddToken(TokenConst, nullptr, 0, nullptr);
			} else if (lexemeLength == 2 && memcmp(g_tokenizer.LexemeStart, "if", lexemeLength) == 0) {
				TokenizerAddToken(TokenIf, nullptr, 0, nullptr);
			} else if (lexemeLength == 4 && memcmp(g_tokenizer.LexemeStart, "else", lexemeLength) == 0) {
				TokenizerAddToken(TokenElse, nullptr, 0, nullptr);
			} else if (lexemeLength == 5 && memcmp(g_tokenizer.LexemeStart, "while", lexemeLength) == 0) {
				TokenizerAddToken(TokenWhile, nullptr, 0, nullptr);
			} else if (lexemeLength == 3 && memcmp(g_tokenizer.LexemeStart, "and", lexemeLength) == 0) {
				TokenizerAddToken(TokenAnd, nullptr, 0, nullptr);
			} else if (lexemeLength == 2 && memcmp(g_tokenizer.LexemeStart, "or", lexemeLength) == 0) {
				TokenizerAddToken(TokenOr, nullptr, 0, nullptr);
			} else {
				SymbolView lexeme;
				Result result = SymbolTableAdd(&g_tokenizer.TableIdentifiers, g_tokenizer.LexemeStart, lexemeLength, &lexeme);
				if (result) {
					return result;
				}
				TokenizerAddToken(TokenIdentifier, &lexeme, 0, nullptr);
			}
		} else {
			TokenizerAddToken((TokenType)c, nullptr, 0, nullptr);
		}

		*token = &g_tokenizer.LastReturnedToken;
		return ResOk;
	}
}

Result TokenizerDeinit() { return SymbolTableDeinit(&g_tokenizer.TableIdentifiers); }
