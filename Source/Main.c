#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum TokenType : uint8_t {
	TokenLeftRoundBracket,
	TokenRightRoundBracket,
	TokenLeftSquareBracket,
	TokenRightSquareBracket,
	TokenLeftVectorBracket,
	TokenRightVectorBracket,
	TokenLeftCurlyBracket,
	TokenRightCurlyBracket,
	TokenComma,
	TokenPlus,
	TokenMinus,
	TokenStar,
	TokenSlash,
	TokenUpArrow,
	TokenApostrophe,
	TokenColon,
	TokenMatrixShape,
	TokenSemicolon,
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
	const char* Lexeme;
	double Number;
	size_t SourceLine;
} Token;

typedef enum ASTNodeType : uint8_t {
	Literal,
	Block,
	Unary,
	Grouping,
	Binary,
	VarDecl,
	WhileStmt,
	IfStmt,
	IndexSuffix,
	Assignment,
	Identifier
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
			struct ASTNode* Expression;
		} Grouping;

		struct {
			struct ASTNode* Left;
			Token* Operator;
			struct ASTNode* Right;
		} Binary;

		struct {
			Token* Identifier;
			// Nullable
			Token* Type;
			// Nullable
			struct ASTNode* Expression;
			bool IsConst;
		} VarDecl;

		struct {
			struct ASTNode* Condition;
			struct ASTNode* Body;
		} WhileStmt;

		struct {
			struct ASTNode* Condition;
			struct ASTNode* ThenBlock;
			struct ASTNode* ElseBlock;
		} IfStmt;

		struct {
			struct ASTNode* I;
			struct ASTNode* J;
		} IndexSuffix;

		struct {
			Token* Identifier;
			struct ASTNode* Index;
			struct ASTNode* Expression;
		} Assignment;

		Token* Identifier;
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
		ASTNodePrint(node->Data.Grouping.Expression);
		printf(")");
		break;
	case Binary:
		printf("(%s ", node->Data.Binary.Operator->Lexeme);
		ASTNodePrint(node->Data.Binary.Left);
		printf(" ");
		ASTNodePrint(node->Data.Binary.Right);
		printf(")");
		break;
	case VarDecl:
		if (node->Data.VarDecl.IsConst) {
			printf("(const ");
		} else {
			printf("(let ");
		}
		printf("%s", node->Data.VarDecl.Identifier->Lexeme);
		if (node->Data.VarDecl.Type) {
			printf(": %s", node->Data.VarDecl.Type->Lexeme);
		}
		printf(" = ");
		ASTNodePrint(node->Data.VarDecl.Expression);
		printf(")");
		break;
	case WhileStmt:
		printf("(while ");
		ASTNodePrint(node->Data.WhileStmt.Condition);
		printf(" ");
		ASTNodePrint(node->Data.WhileStmt.Body);
		printf(" )");
		break;
	case IfStmt:
		printf("(if ");
		ASTNodePrint(node->Data.IfStmt.Condition);
		printf(" then ");
		ASTNodePrint(node->Data.IfStmt.ThenBlock);
		printf(" else ");
		ASTNodePrint(node->Data.IfStmt.ElseBlock);
		printf(" )");
		break;
	case IndexSuffix:
		printf("(index ");
		ASTNodePrint(node->Data.IndexSuffix.I);
		printf(" ");
		ASTNodePrint(node->Data.IndexSuffix.J);
		printf(")");
		break;
	case Assignment:
		printf("(assignment %s", node->Data.Assignment.Identifier->Lexeme);
		if (node->Data.Assignment.Index) {
			ASTNodePrint(node->Data.Assignment.Index);
		}
		printf(" ");
		ASTNodePrint(node->Data.Assignment.Expression);
		printf(")");
		break;
	case Identifier:
		printf("(ident %s", node->Data.Identifier->Lexeme);
		printf(")");
		break;
	}
}

typedef struct Parser {
	Token* Tokens;
	size_t CurrentToken;
} Parser;

Parser g_parser = { .Tokens = nullptr, .CurrentToken = 0 };

void ParserAdvance(Parser* parser) { parser->CurrentToken++; }

bool ParserMatch(Parser* parser, TokenType type)
{
	if (parser->Tokens[parser->CurrentToken].Type == type) {
		ParserAdvance(parser);
		return true;
	}

	return false;
}

void ParserExpect(Parser* parser, TokenType type)
{
	if (parser->Tokens[parser->CurrentToken].Type != type) {
		printf("Error: Expected token %u but got %u", type, parser->Tokens[parser->CurrentToken].Type);
	}

	ParserAdvance(parser);
}

ASTNode* ParseStatement(Parser* parser);
ASTNode* ParseBlock(Parser* parser);
ASTNode* ParseVarDecl(Parser* parser);
ASTNode* ParseWhileStmt(Parser* parser);
ASTNode* ParseIfStmt(Parser* parser);
ASTNode* ParseExpressionOrAssignment(Parser* parser);
ASTNode* ParseExpression(Parser* parser);
ASTNode* ParseLogicAnd(Parser* parser);
ASTNode* ParseEquality(Parser* parser);
ASTNode* ParseComparison(Parser* parser);
ASTNode* ParseTerm(Parser* parser);
ASTNode* ParseFactor(Parser* parser);
ASTNode* ParseExponent(Parser* parser);
ASTNode* ParseUnary(Parser* parser);
ASTNode* ParsePostfix(Parser* parser);
ASTNode* ParsePrimary(Parser* parser);
ASTNode* ParseIdentifierPrimary(Parser* parser);

ASTNode* ParseProgram(Parser* parser)
{
	ASTNode* topLevelBlock = malloc(sizeof(ASTNode));
	topLevelBlock->Type = Block;
	topLevelBlock->Data.Block.Nodes = (ASTNode**)malloc(64 * sizeof(ASTNode*));

	size_t i = 0;
	while (parser->Tokens[parser->CurrentToken].Type != TokenEof) {
		ASTNode* statement = ParseStatement(parser);
		topLevelBlock->Data.Block.Nodes[i] = statement;

		++i;
	}

	topLevelBlock->Data.Block.NodeCount = i;

	return topLevelBlock;
}

// TODO: Implement mandatoy semicolons
ASTNode* ParseStatement(Parser* parser)
{
	switch (parser->Tokens[parser->CurrentToken].Type) {
	case TokenLet:
	case TokenConst:
		return ParseVarDecl(parser);
	case TokenIf:
		return ParseIfStmt(parser);
	case TokenWhile:
		return ParseWhileStmt(parser);
	case TokenLeftCurlyBracket:
		return ParseBlock(parser);

	default:
		return ParseExpressionOrAssignment(parser);
	}
}

ASTNode* ParseBlock(Parser* parser)
{
	ParserExpect(parser, TokenLeftCurlyBracket);

	ASTNode* block = malloc(sizeof(ASTNode));
	block->Type = Block;
	block->Data.Block.Nodes = (ASTNode**)malloc(64 * sizeof(ASTNode*));

	size_t i = 0;
	while (parser->Tokens[parser->CurrentToken].Type != TokenRightCurlyBracket) {
		ASTNode* statement = ParseStatement(parser);
		block->Data.Block.Nodes[i] = statement;

		++i;
	}

	ParserExpect(parser, TokenRightCurlyBracket);
	block->Data.Block.NodeCount = i;

	return block;
}

ASTNode* ParseVarDecl(Parser* parser)
{
	bool isConst = parser->Tokens[parser->CurrentToken].Type == TokenConst;
	ParserAdvance(parser);

	Token* identifier = parser->Tokens + parser->CurrentToken;
	ParserExpect(parser, TokenIdentifier);

	ASTNode* varDecl = malloc(sizeof(ASTNode));
	varDecl->Type = VarDecl;
	varDecl->Data.VarDecl.IsConst = isConst;
	varDecl->Data.VarDecl.Type = nullptr;
	varDecl->Data.VarDecl.Identifier = identifier;

	if (ParserMatch(parser, TokenColon)) {
		Token* type = parser->Tokens + parser->CurrentToken;
		ParserExpect(parser, TokenMatrixShape);
		varDecl->Data.VarDecl.Type = type;
	}

	ParserExpect(parser, TokenEqual);

	ASTNode* expression = ParseExpression(parser);
	varDecl->Data.VarDecl.Expression = expression;

	return varDecl;
}

ASTNode* ParseWhileStmt(Parser* parser)
{
	ParserExpect(parser, TokenWhile);

	ASTNode* condition = ParseExpression(parser);
	ASTNode* body = ParseBlock(parser);

	ASTNode* whileStmt = malloc(sizeof(ASTNode));
	whileStmt->Type = WhileStmt;
	whileStmt->Data.WhileStmt.Body = body;
	whileStmt->Data.WhileStmt.Condition = condition;

	return whileStmt;
}

ASTNode* ParseIfStmt(Parser* parser)
{
	ParserExpect(parser, TokenIf);

	ASTNode* condition = ParseExpression(parser);
	ASTNode* thenBlock = ParseBlock(parser);

	ASTNode* elseBlock = nullptr;
	if (ParserMatch(parser, TokenElse)) {
		if (parser->Tokens[parser->CurrentToken].Type == TokenIf) {
			elseBlock = ParseIfStmt(parser);
		} else {
			elseBlock = ParseBlock(parser);
		}
	}

	ASTNode* ifStmt = malloc(sizeof(ASTNode));
	ifStmt->Type = IfStmt;
	ifStmt->Data.IfStmt.Condition = condition;
	ifStmt->Data.IfStmt.ThenBlock = thenBlock;
	ifStmt->Data.IfStmt.ElseBlock = elseBlock;

	return ifStmt;
}

ASTNode* ParseExpressionOrAssignment(Parser* parser)
{
	if (parser->Tokens[parser->CurrentToken].Type == TokenIdentifier) {
		Token* identifier = parser->Tokens + parser->CurrentToken;

		Parser backup = *parser;

		ParserAdvance(parser);

		ASTNode* indexSuffix = nullptr;
		if (ParserMatch(parser, TokenLeftSquareBracket)) {
			ASTNode* i = ParseExpression(parser);
			ASTNode* j = nullptr;

			if (parser->Tokens[parser->CurrentToken].Type != TokenRightSquareBracket) {
				j = ParseExpression(parser);
			}

			ParserExpect(parser, TokenRightSquareBracket);
			indexSuffix = malloc(sizeof(ASTNode));
			indexSuffix->Type = IndexSuffix;
			indexSuffix->Data.IndexSuffix.I = i;
			indexSuffix->Data.IndexSuffix.J = j;
		}

		// This is an assignment
		if (ParserMatch(parser, TokenEqual)) {
			ASTNode* assignment = malloc(sizeof(ASTNode));
			assignment->Type = Assignment;
			assignment->Data.Assignment.Identifier = identifier;
			assignment->Data.Assignment.Index = indexSuffix;
			assignment->Data.Assignment.Expression = ParseExpression(parser);

			return assignment;
		}

		// Not an assignment
		*parser = backup;
	}

	return ParseExpression(parser);
}

// TODO: Check all of these functions so they match up exactly with the EBNF
ASTNode* ParseExpression(Parser* parser)
{
	ASTNode* left = ParseLogicAnd(parser);

	while (parser->Tokens[parser->CurrentToken].Type == TokenOr) {
		Token* operator = parser->Tokens + parser->CurrentToken;
		ParserAdvance(parser);

		ASTNode* temp = left;
		ASTNode* right = ParseLogicAnd(parser);
		left = malloc(sizeof(ASTNode));
		left->Type = Binary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	return left;
}

ASTNode* ParseLogicAnd(Parser* parser)
{
	ASTNode* left = ParseEquality(parser);

	while (parser->Tokens[parser->CurrentToken].Type == TokenAnd) {
		Token* operator = parser->Tokens + parser->CurrentToken;
		ParserAdvance(parser);

		ASTNode* temp = left;
		ASTNode* right = ParseEquality(parser);
		left = malloc(sizeof(ASTNode));
		left->Type = Binary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	return left;
}

ASTNode* ParseEquality(Parser* parser)
{
	ASTNode* left = ParseComparison(parser);

	while (parser->Tokens[parser->CurrentToken].Type == TokenEqualEqual || parser->Tokens[parser->CurrentToken].Type == TokenNotEqual) {
		Token* operator = parser->Tokens + parser->CurrentToken;
		ParserAdvance(parser);

		ASTNode* temp = left;
		ASTNode* right = ParseComparison(parser);
		left = malloc(sizeof(ASTNode));
		left->Type = Binary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	return left;
}

ASTNode* ParseComparison(Parser* parser)
{
	ASTNode* left = ParseTerm(parser);

	while (parser->Tokens[parser->CurrentToken].Type == TokenLess || parser->Tokens[parser->CurrentToken].Type == TokenLessEqual
		|| parser->Tokens[parser->CurrentToken].Type == TokenGreater || parser->Tokens[parser->CurrentToken].Type == TokenGreaterEqual) {
		Token* operator = parser->Tokens + parser->CurrentToken;
		ParserAdvance(parser);

		ASTNode* temp = left;
		ASTNode* right = ParseTerm(parser);
		left = malloc(sizeof(ASTNode));
		left->Type = Binary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	return left;
}

ASTNode* ParseTerm(Parser* parser)
{
	ASTNode* left = ParseFactor(parser);

	while (parser->Tokens[parser->CurrentToken].Type == TokenPlus || parser->Tokens[parser->CurrentToken].Type == TokenMinus) {
		Token* operator = parser->Tokens + parser->CurrentToken;
		ParserAdvance(parser);

		ASTNode* temp = left;
		ASTNode* right = ParseFactor(parser);
		left = malloc(sizeof(ASTNode));
		left->Type = Binary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	return left;
}

ASTNode* ParseFactor(Parser* parser)
{
	ASTNode* left = ParseExponent(parser);

	while (parser->Tokens[parser->CurrentToken].Type == TokenStar || parser->Tokens[parser->CurrentToken].Type == TokenSlash) {
		Token* operator = parser->Tokens + parser->CurrentToken;
		ParserAdvance(parser);

		ASTNode* temp = left;
		ASTNode* right = ParseExponent(parser);
		left = malloc(sizeof(ASTNode));
		left->Type = Binary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	return left;
}

ASTNode* ParseExponent(Parser* parser)
{
	ASTNode* left = ParseUnary(parser);

	while (parser->Tokens[parser->CurrentToken].Type == TokenUpArrow) {
		Token* operator = parser->Tokens + parser->CurrentToken;
		ParserAdvance(parser);

		ASTNode* temp = left;
		ASTNode* right = ParseUnary(parser);
		left = malloc(sizeof(ASTNode));
		left->Type = Binary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	return left;
}

ASTNode* ParseUnary(Parser* parser)
{
	if (parser->Tokens[parser->CurrentToken].Type == TokenMinus) {
		Token* operator = parser->Tokens + parser->CurrentToken;
		ParserAdvance(parser);

		ASTNode* operand = ParseUnary(parser);

		ASTNode* result = malloc(sizeof(ASTNode));
		result->Type = Unary;
		result->Data.Unary.Operator = operator;
		result->Data.Unary.Operand = operand;
		return result;
	}

	return ParsePostfix(parser);
}

ASTNode* ParsePostfix(Parser* parser)
{
	ASTNode* primary = ParsePrimary(parser);

	if (parser->Tokens[parser->CurrentToken].Type == TokenApostrophe) {
		Token* operator = parser->Tokens + parser->CurrentToken;
		ParserAdvance(parser);

		ASTNode* postfix = malloc(sizeof(ASTNode));
		postfix->Type = Unary;
		postfix->Data.Unary.Operator = operator;
		postfix->Data.Unary.Operand = primary;
		return postfix;
	}

	return primary;
}

ASTNode* ParsePrimary(Parser* parser)
{
	if (ParserMatch(parser, TokenNumber)) {
		ASTNode* number = malloc(sizeof(ASTNode));
		number->Type = Literal;
		number->Data.Literal = parser->Tokens[parser->CurrentToken - 1].Number;

		return number;
	}

	if (ParserMatch(parser, TokenLeftRoundBracket)) {
		ASTNode* expression = ParseExpression(parser);

		ASTNode* grouping = malloc(sizeof(ASTNode));
		grouping->Type = Grouping;
		grouping->Data.Grouping.Expression = expression;

		ParserExpect(parser, TokenRightRoundBracket);

		return grouping;
	}

	if (parser->Tokens[parser->CurrentToken].Type == TokenIdentifier) {
		return ParseIdentifierPrimary(parser);
	}

	if (parser->Tokens[parser->CurrentToken].Type == TokenLeftSquareBracket) {
		// TODO: Implement matrix literals
		printf("Unimplemented!!\n");
		return nullptr;
	}

	if (parser->Tokens[parser->CurrentToken].Type == TokenLeftVectorBracket) {
		// TODO: Implement vector literals
		printf("Unimplemented!!\n");
		return nullptr;
	}

	printf("Unexpected token %s in primary\n", parser->Tokens[parser->CurrentToken].Lexeme);
	return nullptr;
}

ASTNode* ParseIdentifierPrimary(Parser* parser)
{
	Token* identifier = parser->Tokens + parser->CurrentToken;
	ParserAdvance(parser);

	if (ParserMatch(parser, TokenLeftRoundBracket)) {
		// TODO: Implement function calls
		printf("Unimplemented!!\n");
		return nullptr;
	}

	if (ParserMatch(parser, TokenLeftSquareBracket)) {
		// TODO: Implement indexing
		printf("Unimplemented!!\n");
		return nullptr;
	}

	ASTNode* astIdentifier = malloc(sizeof(ASTNode));
	astIdentifier->Type = Identifier;
	astIdentifier->Data.Identifier = identifier;

	return astIdentifier;
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
			ScannerAddToken(TokenLeftRoundBracket, 0);
			break;
		case ')':
			ScannerAddToken(TokenRightRoundBracket, 0);
			break;
		case '[':
			ScannerAddToken(TokenLeftSquareBracket, 0);
			break;
		case ']':
			ScannerAddToken(TokenRightSquareBracket, 0);
			break;
		case '<':
			if (g_scanner.Source[g_scanner.LexemeCurrent] == '=') {
				ScannerAddToken(TokenLessEqual, 0);
				++g_scanner.LexemeCurrent;
			} else if (g_scanner.Source[g_scanner.LexemeCurrent] == '<') {
				ScannerAddToken(TokenLeftVectorBracket, 0);
				++g_scanner.LexemeCurrent;
			} else {
				ScannerAddToken(TokenLess, 0);
			}
			break;
		case '>':
			if (g_scanner.Source[g_scanner.LexemeCurrent] == '=') {
				ScannerAddToken(TokenGreaterEqual, 0);
				++g_scanner.LexemeCurrent;
			} else if (g_scanner.Source[g_scanner.LexemeCurrent] == '>') {
				ScannerAddToken(TokenRightVectorBracket, 0);
				++g_scanner.LexemeCurrent;
			} else {
				ScannerAddToken(TokenGreater, 0);
			}
			break;
		case '{':
			ScannerAddToken(TokenLeftCurlyBracket, 0);
			break;
		case '}':
			ScannerAddToken(TokenRightCurlyBracket, 0);
			break;
		case ',':
			ScannerAddToken(TokenComma, 0);
			break;
		case '+':
			ScannerAddToken(TokenPlus, 0);
			break;
		case '-':
			ScannerAddToken(TokenMinus, 0);
			break;
		case '*':
			ScannerAddToken(TokenStar, 0);
			break;
		case '/':
			ScannerAddToken(TokenSlash, 0);
			break;
		case '^':
			ScannerAddToken(TokenUpArrow, 0);
			break;
		case '\'':
			ScannerAddToken(TokenApostrophe, 0);
			break;
		case ':':
			ScannerAddToken(TokenColon, 0);

			while (isspace(g_scanner.Source[g_scanner.LexemeCurrent])) {
				++g_scanner.LexemeCurrent;
			}

			g_scanner.LexemeStart = g_scanner.LexemeCurrent;

			if (!isdigit(g_scanner.Source[g_scanner.LexemeCurrent++])) {
				printf("Expected type declaration at line %lu.\n", g_scanner.Line);

				while (isalnum(g_scanner.Source[g_scanner.LexemeCurrent])) {
					++g_scanner.LexemeCurrent;
				}

				break;
			}

			while (isdigit(g_scanner.Source[g_scanner.LexemeCurrent])) {
				++g_scanner.LexemeCurrent;
			}

			if (g_scanner.Source[g_scanner.LexemeCurrent++] != 'x') {
				printf("Expected type declaration at line %lu.\n", g_scanner.Line);

				while (isalnum(g_scanner.Source[g_scanner.LexemeCurrent])) {
					++g_scanner.LexemeCurrent;
				}

				break;
			}

			if (!isdigit(g_scanner.Source[g_scanner.LexemeCurrent++])) {
				printf("Expected type declaration at line %lu.\n", g_scanner.Line);

				while (isalnum(g_scanner.Source[g_scanner.LexemeCurrent])) {
					++g_scanner.LexemeCurrent;
				}

				break;
			}

			while (isdigit(g_scanner.Source[g_scanner.LexemeCurrent])) {
				++g_scanner.LexemeCurrent;
			}

			{
				char* lexeme = malloc(g_scanner.LexemeCurrent - g_scanner.LexemeStart + 1);
				lexeme[g_scanner.LexemeCurrent - g_scanner.LexemeStart] = '\0';
				memcpy(lexeme, g_scanner.Source + g_scanner.LexemeStart, g_scanner.LexemeCurrent - g_scanner.LexemeStart);
				g_scanner.Tokens[g_scanner.TokensFreeIndex].Lexeme = lexeme;

				g_scanner.Tokens[g_scanner.TokensFreeIndex].Type = TokenMatrixShape;
				g_scanner.Tokens[g_scanner.TokensFreeIndex].Number = 0;
				g_scanner.Tokens[g_scanner.TokensFreeIndex].SourceLine = g_scanner.Line;

				++g_scanner.TokensFreeIndex;
			}
			break;
		case '=':
			if (g_scanner.Source[g_scanner.LexemeCurrent] == '=') {
				ScannerAddToken(TokenEqualEqual, 0);
				++g_scanner.LexemeCurrent;
			} else {
				ScannerAddToken(TokenEqual, 0);
			}
			break;
		case ';':
			ScannerAddToken(TokenSemicolon, 0);
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

	ASTNode* program = ParseProgram(&g_parser);

	ASTNodePrint(program);

	return 0;
}
