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

// A gemini's data structure but modified:
// Uses multiple tagged unions in AST nodes
// For now possible AST node types:
// - literal: a double because that's what I'll operate on now
// - blocks: just so I can have multiple lines of code and then later ifs and so (just an array)
// - unary: negation (no booleans for now) and transposing
// - grouping: parenthesis to group expressions (just a pointer to another node)
//
// Each of them having appropriate pointers with operators, literals, identifiers (tokens) or other AST nodes
// This can then be displayed and parsed

typedef enum ASTNodeType : uint8_t {
	Literal,
	Block,
	Unary,
	Grouping,
	Binary
} ASTNodeType;

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

	ASTNode* left = malloc(sizeof(ASTNode));
	left->Type = Unary;
	left->Data.Unary.Operator = malloc(sizeof(Token));
	left->Data.Unary.Operator->Number = 0;
	left->Data.Unary.Operator->SourceLine = 69;
	left->Data.Unary.Operator->Lexeme = "'";
	left->Data.Unary.Operator->Type = Apostrophe;
	left->Data.Unary.Operand = malloc(sizeof(ASTNode));
	left->Data.Unary.Operand->Type = Grouping;
	left->Data.Unary.Operand->Data.Grouping.Node = malloc(sizeof(ASTNode));
	left->Data.Unary.Operand->Data.Grouping.Node->Type = Binary;
	left->Data.Unary.Operand->Data.Grouping.Node->Data.Binary.Left = malloc(sizeof(ASTNode));
	left->Data.Unary.Operand->Data.Grouping.Node->Data.Binary.Left->Type = Literal;
	left->Data.Unary.Operand->Data.Grouping.Node->Data.Binary.Left->Data.Literal = 98;
	left->Data.Unary.Operand->Data.Grouping.Node->Data.Binary.Right = malloc(sizeof(ASTNode));
	left->Data.Unary.Operand->Data.Grouping.Node->Data.Binary.Right->Type = Literal;
	left->Data.Unary.Operand->Data.Grouping.Node->Data.Binary.Right->Data.Literal = 65;
	left->Data.Unary.Operand->Data.Grouping.Node->Data.Binary.Operator = malloc(sizeof(Token));
	left->Data.Unary.Operand->Data.Grouping.Node->Data.Binary.Operator->Type = Minus;
	left->Data.Unary.Operand->Data.Grouping.Node->Data.Binary.Operator->Lexeme = "-";
	left->Data.Unary.Operand->Data.Grouping.Node->Data.Binary.Operator->SourceLine = 69;
	left->Data.Unary.Operand->Data.Grouping.Node->Data.Binary.Operator->Number = 0;

	ASTNode* right = malloc(sizeof(ASTNode));
	right->Type = Literal;
	right->Data.Literal = 456;

	ASTNode* node = malloc(sizeof(ASTNode));
	node->Type = Binary;
	node->Data.Binary.Left = left;
	node->Data.Binary.Right = right;
	node->Data.Binary.Operator = malloc(sizeof(Token));
	node->Data.Binary.Operator->Type = Plus;
	node->Data.Binary.Operator->Lexeme = "+";
	node->Data.Binary.Operator->SourceLine = 69;
	node->Data.Binary.Operator->Number = 0;

	ASTNode* nodes[3];
	nodes[0] = malloc(sizeof(ASTNode));
	nodes[0]->Type = Literal;
	nodes[0]->Data.Literal = 123;
	nodes[1] = node;
	nodes[2] = malloc(sizeof(ASTNode));
	nodes[2]->Type = Literal;
	nodes[2]->Data.Literal = 789;
	ASTNode* block = malloc(sizeof(ASTNode));
	block->Type = Block;
	block->Data.Block.NodeCount = 3;
	block->Data.Block.Nodes = nodes;

	ASTNodePrint(block);

	return 0;
}
