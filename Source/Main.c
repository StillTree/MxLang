#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum TokenType : uint8_t {
	LeftRoundBracket,
	RightRoundBracket,
	LeftSquareBracket,
	RightSquareBracket,
	LeftAngleBracket,
	RightAngleBracket,
	Comma,
	Plus,
	Minus,
	Star,
	Slash,
	UpArrow,
	Apostrophe,
	Colon,
	Equal,
	Identifier,
	Number,
	Let,
	Const,
	Det,
	Ident,
	Inv,
	Eof
} TokenType;

typedef struct Token {
	TokenType Type;
	const char* Lexeme;
	double Number;
	size_t SourceLine;
} Token;

typedef enum ASTNodeType : uint8_t { Literal, Block, Unary, Grouping, Binary } ASTNodeType;

struct ASTNode;

typedef struct ASTNode {
	ASTNodeType Type;

	union {
		double Literal;

		struct {
			struct ASTNode** Nodes;
			size_t NodeCount;
		} Block;

		struct {
			Token* Operator;
			struct ASTNode* Operand;
		} Unary;

		struct {
			struct ASTNode* Node;
		} Grouping;

		struct {
			struct ASTNode* Left;
			Token* Operator;
			struct ASTNode* Right;
		} Binary;
	} Data;
} ASTNode;

void ASTNodePrint(const ASTNode* node)
{
	switch (node->Type) {
	case Literal:
		printf("%lf", node->Data.Literal);
		break;
	case Block:
		printf("(block\n");
		for (size_t i = 0; i < node->Data.Block.NodeCount; ++i) {
			printf("  ");
			ASTNodePrint(node->Data.Block.Nodes[i]);
			printf("\n");
		}
		printf(")");
		break;
	case Unary:
		printf("(%s ", node->Data.Unary.Operator->Lexeme);
		ASTNodePrint(node->Data.Unary.Operand);
		printf(")");
		break;
	case Grouping:
		printf("(grouping ");
		ASTNodePrint(node->Data.Grouping.Node);
		printf(")");
		break;
	case Binary:
		printf("(%s ", node->Data.Binary.Operator->Lexeme);
		ASTNodePrint(node->Data.Binary.Left);
		printf(" ");
		ASTNodePrint(node->Data.Binary.Right);
		printf(")");
		break;
	}
}

typedef struct Parser {
	Token* Tokens;
	size_t CurrentToken;
} Parser;

Parser g_parser = { .Tokens = nullptr, .CurrentToken = 0 };

ASTNode* ParsePrimary(Parser* parser);
ASTNode* ParseUnary(Parser* parser);
ASTNode* ParseFactor(Parser* parser);
ASTNode* ParseTerm(Parser* parser);

ASTNode* ParsePrimary(Parser* parser)
{
	if (parser->Tokens[parser->CurrentToken].Type == Number) {
		++parser->CurrentToken;

		ASTNode* result = malloc(sizeof(ASTNode));
		result->Type = Literal;
		result->Data.Literal = parser->Tokens[parser->CurrentToken - 1].Number;
		return result;
	}

	if (parser->Tokens[parser->CurrentToken].Type == LeftRoundBracket) {
		++parser->CurrentToken;

		ASTNode* term = ParseTerm(parser);
		if (parser->Tokens[parser->CurrentToken].Type != RightRoundBracket) {
			return nullptr;
		}

		++parser->CurrentToken;

		ASTNode* result = malloc(sizeof(ASTNode));
		result->Type = Grouping;
		result->Data.Grouping.Node = term;
		return result;
	}

	return nullptr;
}

ASTNode* ParseUnary(Parser* parser)
{
	if (parser->Tokens[parser->CurrentToken].Type == Minus) {
		++parser->CurrentToken;

		Token* operator = parser->Tokens + parser->CurrentToken - 1;
		ASTNode* operand = ParseUnary(parser);

		ASTNode* result = malloc(sizeof(ASTNode));
		result->Type = Unary;
		result->Data.Unary.Operator = operator;
		result->Data.Unary.Operand = operand;
		return result;
	}

	return ParsePrimary(parser);
}

ASTNode* ParseFactor(Parser* parser)
{
	ASTNode* unary = ParseUnary(parser);

	while (parser->Tokens[parser->CurrentToken].Type == Slash || parser->Tokens[parser->CurrentToken].Type == Star) {
		++parser->CurrentToken;

		Token* token = parser->Tokens + parser->CurrentToken - 1;
		ASTNode* right = ParseUnary(parser);

		ASTNode* temp = unary;
		unary = malloc(sizeof(ASTNode));
		unary->Type = Binary;
		unary->Data.Binary.Left = temp;
		unary->Data.Binary.Operator = token;
		unary->Data.Binary.Right = right;
	}

	return unary;
}

ASTNode* ParseTerm(Parser* parser)
{
	ASTNode* factor = ParseFactor(parser);

	while (parser->Tokens[parser->CurrentToken].Type == Plus || parser->Tokens[parser->CurrentToken].Type == Minus) {
		++parser->CurrentToken;

		Token* token = parser->Tokens + parser->CurrentToken - 1;
		ASTNode* right = ParseFactor(parser);

		ASTNode* temp = factor;
		factor = malloc(sizeof(ASTNode));
		factor->Type = Binary;
		factor->Data.Binary.Left = temp;
		factor->Data.Binary.Operator = token;
		factor->Data.Binary.Right = right;
	}

	return factor;
}

typedef struct Scanner {
	const char* Source;
	Token Tokens[1000];
	size_t TokensFreeIndex;
	size_t LexemeStart;
	size_t LexemeCurrent;
	size_t Line;
} Scanner;

Scanner g_scanner = { .Source = nullptr, .TokensFreeIndex = 0, .LexemeStart = 0, .LexemeCurrent = 0, .Line = 1 };

void ScannerAddToken(TokenType type, double number)
{
	g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = type;
	g_scanner.Tokens[g_scanner.TokensFreeIndex].Number = number;
	g_scanner.Tokens[g_scanner.TokensFreeIndex].SourceLine = g_scanner.Line;

	char* lexeme = malloc(g_scanner.LexemeCurrent - g_scanner.LexemeStart + 1);
	lexeme[g_scanner.LexemeCurrent - g_scanner.LexemeStart] = '\0';
	memcpy(lexeme, g_scanner.Source + g_scanner.LexemeStart, g_scanner.LexemeCurrent - g_scanner.LexemeStart);
	g_scanner.Tokens[g_scanner.TokensFreeIndex].Lexeme = lexeme;

	++g_scanner.TokensFreeIndex;
}

void Scan()
{
	const size_t sourceLength = strlen(g_scanner.Source);
	while (g_scanner.LexemeCurrent < sourceLength) {
		g_scanner.LexemeStart = g_scanner.LexemeCurrent;

		char c = g_scanner.Source[g_scanner.LexemeCurrent++];
		switch (c) {
		case '#':
			while (g_scanner.Source[g_scanner.LexemeCurrent] != '\0' && g_scanner.Source[g_scanner.LexemeCurrent] != '\n') {
				++g_scanner.LexemeCurrent;
			}
			break;
		case '(':
			ScannerAddToken(LeftRoundBracket, 0);
			break;
		case ')':
			ScannerAddToken(RightRoundBracket, 0);
			break;
		case '[':
			ScannerAddToken(LeftSquareBracket, 0);
			break;
		case ']':
			ScannerAddToken(RightSquareBracket, 0);
			break;
		case '<':
			ScannerAddToken(LeftAngleBracket, 0);
			break;
		case '>':
			ScannerAddToken(RightAngleBracket, 0);
			break;
		case ',':
			ScannerAddToken(Comma, 0);
			break;
		case '+':
			ScannerAddToken(Plus, 0);
			break;
		case '-':
			ScannerAddToken(Minus, 0);
			break;
		case '*':
			ScannerAddToken(Star, 0);
			break;
		case '/':
			ScannerAddToken(Slash, 0);
			break;
		case '^':
			ScannerAddToken(UpArrow, 0);
			break;
		case '\'':
			ScannerAddToken(Apostrophe, 0);
			break;
		case ':':
			ScannerAddToken(Colon, 0);
			break;
		case '=':
			ScannerAddToken(Equal, 0);
			break;
		case ' ':
		case '\r':
		case '\t':
			break;
		case '\n':
			++g_scanner.Line;
			break;
		default:
			if (isdigit(c)) {
				while (isdigit(g_scanner.Source[g_scanner.LexemeCurrent])) {
					++g_scanner.LexemeCurrent;
				}

				if (g_scanner.Source[g_scanner.LexemeCurrent] == '.' && isdigit(g_scanner.Source[g_scanner.LexemeCurrent + 1])) {
					++g_scanner.LexemeCurrent;

					while (isdigit(g_scanner.Source[g_scanner.LexemeCurrent++]))
						;
				}

				char* lexeme = malloc(g_scanner.LexemeCurrent - g_scanner.LexemeStart + 1);
				lexeme[g_scanner.LexemeCurrent - g_scanner.LexemeStart] = '\0';
				memcpy(lexeme, g_scanner.Source + g_scanner.LexemeStart, g_scanner.LexemeCurrent - g_scanner.LexemeStart);
				g_scanner.Tokens[g_scanner.TokensFreeIndex].Lexeme = lexeme;

				g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = Number;
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
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = Let;
				} else if (strcmp(lexeme, "const") == 0) {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = Const;
				} else if (strcmp(lexeme, "det") == 0) {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = Det;
				} else if (strcmp(lexeme, "ident") == 0) {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = Ident;
				} else if (strcmp(lexeme, "inv") == 0) {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = Inv;
				} else {
					g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = Identifier;
				}

				++g_scanner.TokensFreeIndex;
			} else {
				printf("Unexpected character '%c' at line %lu.", c, g_scanner.Line);
			}
			break;
		}
	}

	g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = Eof;
	g_scanner.Tokens[g_scanner.TokensFreeIndex].Lexeme = nullptr;
	g_scanner.Tokens[g_scanner.TokensFreeIndex].SourceLine = g_scanner.Line;

	++g_scanner.TokensFreeIndex;
}

char* ReadFileToBuffer(const char* filePath)
{
	FILE* file = fopen(filePath, "rb");

	if (file == nullptr) {
		printf("Could not open file");
		return nullptr;
	}

	fseek(file, 0, SEEK_END);
	int64_t fileSize = ftell(file);

	if (fileSize < 0) {
		printf("Could not find file size");
		fclose(file);
		return nullptr;
	}

	fseek(file, 0, SEEK_SET);

	char* fileBuffer = malloc((size_t)fileSize + 1);
	if (!fileBuffer) {
		printf("Could not allocate memory for file");
		fclose(file);
		return nullptr;
	}

	size_t readSize = fread(fileBuffer, 1, (size_t)fileSize, file);
	fileBuffer[readSize] = '\0';

	fclose(file);

	return fileBuffer;
}

int main(int argc, char* argv[])
{
	printf("MxLang v" MX_VERSION "\n\n");

	if (argc < 2) {
		printf("No input file to run\n");
		return 1;
	}

	char* sourceCode = ReadFileToBuffer(argv[1]);

	if (!sourceCode) {
		return 1;
	}

	printf("File contents:\n%s\n", sourceCode);

	g_scanner.Source = sourceCode;
	Scan();

	for (size_t i = 0; i < g_scanner.TokensFreeIndex; ++i) {
		printf("Token %d lexeme \"%s\" number %lf line %lu\n", g_scanner.Tokens[i].Type, g_scanner.Tokens[i].Lexeme,
			g_scanner.Tokens[i].Number, g_scanner.Tokens[i].SourceLine);
	}

	free(sourceCode);

	g_parser.Tokens = g_scanner.Tokens;

	ASTNode* term = ParseTerm(&g_parser);

	ASTNodePrint(term);

	return 0;
}
