/*
MxLang - Parser.c
Autor: Alexander Dębowski (293472)
Data: 09.01.2026
*/

#include "Parser.h"
#include "Diagnostics.h"
#include <stdio.h>

Parser g_parser = { 0 };

static void ParserAdvance() { TokenizerNextToken(); }

static Token* ParserPeek() { return TokenizerPeekToken(); }

static Token* ParserConsume()
{
	Token* token = ParserPeek();
	ParserAdvance();

	return token;
}

static bool ParserMatch(TokenType type)
{
	Token* token = ParserPeek();

	if (token->Type != type) {
		return false;
	}

	ParserAdvance();
	return true;
}

static void ParserSynchronize()
{
	ParserAdvance();

	while (true) {
		Token* token = ParserPeek();

		switch (token->Type) {
		case TokenEof:
		case TokenLet:
		case TokenConst:
		case TokenIf:
		case TokenWhile:
		case TokenLeftCurlyBracket:
		case TokenRightCurlyBracket:
			return;
		default:
			ParserAdvance();
			break;
		}
	}
}

static ASTNode* ParseStatement();
static ASTNode* ParseBlock();
static ASTNode* ParseVarDecl();
static ASTNode* ParseWhileStmt();
static ASTNode* ParseIfStmt();
static ASTNode* ParseExprOrAssignment();
static ASTNode* ParseExpression();
static ASTNode* ParseLogicAnd();
static ASTNode* ParseEquality();
static ASTNode* ParseComparison();
static ASTNode* ParseTerm();
static ASTNode* ParseFactor();
static ASTNode* ParseExponent();
static ASTNode* ParseUnary();
static ASTNode* ParsePostfix();
static ASTNode* ParsePrimary();
static ASTNode* ParseIdentifierPrimary();

static ASTNode* ParseIdentifierPrimary()
{
	SourceLoc loc = ParserPeek()->Loc;
	SymbolView identifier = ParserConsume()->Lexeme;

	// A function call
	if (ParserMatch(TokenLeftRoundBracket)) {
		ASTNode* functionCall;
		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&functionCall));

		functionCall->Type = ASTNodeFunctionCall;
		functionCall->Loc = loc;
		functionCall->FnCall.Identifier = identifier;
		DIAG_PANIC_ON_ERR(DynArenaAlloc(&g_parser.ArraysArena, (void**)&functionCall->FnCall.CallArgs, 3 * sizeof(ASTNode*)));

		usz i = 0;
		while (ParserPeek()->Type != TokenRightRoundBracket) {
			if (i > 0) {
				if (!ParserMatch(TokenComma)) {
					DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc, DIAG_ARG_TOKEN_TYPE(TokenComma));
					ParserSynchronize();
					return nullptr;
				}
			}

			if (i > 2) {
				DIAG_EMIT(DiagTooManyFunctionCallArgs, functionCall->Loc, DIAG_ARG_SYMBOL_VIEW(functionCall->FnCall.Identifier));
				ParserSynchronize();
				return nullptr;
			}

			functionCall->FnCall.CallArgs[i] = ParseExpression();
			++i;
		}

		if (!ParserMatch(TokenRightRoundBracket)) {
			DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc, DIAG_ARG_TOKEN_TYPE(TokenRightRoundBracket));
			ParserSynchronize();
			return nullptr;
		}
		functionCall->FnCall.ArgCount = i;

		return functionCall;
	}

	// Not a function call
	ASTNode* indexSuffix = nullptr;
	if (ParserMatch(TokenLeftSquareBracket)) {
		ASTNode* i;
		i = ParseExpression();
		ASTNode* j = nullptr;

		if (ParserPeek()->Type != TokenRightSquareBracket) {
			j = ParseExpression();
		}

		if (!ParserMatch(TokenRightSquareBracket)) {
			DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc, DIAG_ARG_TOKEN_TYPE(TokenRightSquareBracket));
			ParserSynchronize();
			return nullptr;
		}

		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&indexSuffix));

		indexSuffix->Type = ASTNodeIndexSuffix;
		indexSuffix->IndexSuffix.I = i;
		indexSuffix->IndexSuffix.J = j;
	}

	ASTNode* astIdentifier;
	DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&astIdentifier));

	astIdentifier->Type = ASTNodeIdentifier;
	astIdentifier->Loc = loc;
	astIdentifier->Identifier.Identifier = identifier;
	astIdentifier->Identifier.Index = indexSuffix;

	return astIdentifier;
}

static ASTNode* ParsePrimary()
{
	Token token = *ParserPeek();

	if (token.Type == TokenNumber) {
		ASTNode* number;
		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&number));

		number->Type = ASTNodeMxLiteral;
		number->Loc = token.Loc;
		DIAG_PANIC_ON_ERR(DynArenaAlloc(&g_parser.ArraysArena, (void**)&number->MxLiteral.Matrix, sizeof(ASTNode*)));

		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&number->MxLiteral.Matrix[0]));

		ParserAdvance();

		number->MxLiteral.Matrix[0]->Type = ASTNodeNumber;
		number->MxLiteral.Matrix[0]->Loc = token.Loc;
		number->MxLiteral.Matrix[0]->Number = token.Number;
		number->MxLiteral.Shape.Height = 1;
		number->MxLiteral.Shape.Width = 1;

		return number;
	}

	if (ParserMatch(TokenLeftRoundBracket)) {
		ASTNode* expression;
		expression = ParseExpression();

		ASTNode* grouping;
		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&grouping));

		grouping->Type = ASTNodeGrouping;
		grouping->Loc = token.Loc;
		grouping->Grouping.Expression = expression;

		if (!ParserMatch(TokenRightRoundBracket)) {
			DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc, DIAG_ARG_TOKEN_TYPE(TokenRightRoundBracket));
			ParserSynchronize();
			return nullptr;
		}

		return grouping;
	}

	if (token.Type == TokenIdentifier) {
		return ParseIdentifierPrimary();
	}

	if (ParserMatch(TokenLeftSquareBracket)) {
		ASTNode* matrixLit;
		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&matrixLit));

		matrixLit->Type = ASTNodeMxLiteral;
		matrixLit->Loc = token.Loc;

		if (ParserMatch(TokenRightSquareBracket)) {
			DIAG_EMIT0(DiagEmptyMxLiteralsNotAllowed, ParserPeek()->Loc);
			ParserSynchronize();
			return nullptr;
		}

		Tokenizer backup = g_tokenizer;
		usz height = 0;
		usz width = 0;
		usz maxWidth = 0;
		do {
			while (ParserPeek()->Type != TokenRightSquareBracket && ParserPeek()->Type != TokenEof) {
				ParseExpression();
				++width;
			}

			if (!ParserMatch(TokenRightSquareBracket)) {
				DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc, DIAG_ARG_TOKEN_TYPE(TokenRightSquareBracket));
				ParserSynchronize();
				return nullptr;
			}

			if (width > maxWidth) {
				maxWidth = width;
			}

			++height;
			width = 0;
		} while (ParserMatch(TokenLeftSquareBracket));

		DIAG_PANIC_ON_ERR(DynArenaAlloc(&g_parser.ArraysArena, (void**)&matrixLit->MxLiteral.Matrix, height * maxWidth * sizeof(ASTNode*)));

		g_tokenizer = backup;
		usz i = 0;
		usz j = 0;
		do {
			while (ParserPeek()->Type != TokenRightSquareBracket) {
				matrixLit->MxLiteral.Matrix[(i * maxWidth) + j] = ParseExpression();
				++j;
			}

			while (j < maxWidth) {
				ASTNode* zero;
				DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&zero));

				zero->Type = ASTNodeMxLiteral;
				DIAG_PANIC_ON_ERR(DynArenaAlloc(&g_parser.ArraysArena, (void**)&zero->MxLiteral.Matrix, sizeof(ASTNode*)));

				DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&zero->MxLiteral.Matrix[0]));

				zero->MxLiteral.Matrix[0]->Type = ASTNodeNumber;
				zero->MxLiteral.Matrix[0]->Number = 0;
				zero->MxLiteral.Shape.Height = 1;
				zero->MxLiteral.Shape.Width = 1;

				matrixLit->MxLiteral.Matrix[(i * maxWidth) + j] = zero;

				++j;
			}

			if (!ParserMatch(TokenRightSquareBracket)) {
				DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc, DIAG_ARG_TOKEN_TYPE(TokenRightSquareBracket));
				ParserSynchronize();
				return nullptr;
			}

			++i;
			j = 0;
		} while (ParserMatch(TokenLeftSquareBracket));

		matrixLit->MxLiteral.Shape.Height = height;
		matrixLit->MxLiteral.Shape.Width = maxWidth;

		return matrixLit;
	}

	if (token.Type == TokenLeftVectorBracket) {
		ParserAdvance();

		ASTNode* vectorLit;
		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&vectorLit));

		vectorLit->Type = ASTNodeMxLiteral;
		vectorLit->Loc = token.Loc;
		vectorLit->MxLiteral.Shape.Width = 1;

		if (ParserMatch(TokenRightVectorBracket)) {
			DIAG_EMIT0(DiagEmptyVecLiteralsNotAllowed, ParserPeek()->Loc);
			ParserSynchronize();
			return nullptr;
		}

		Tokenizer backup = g_tokenizer;
		usz count = 0;
		while (ParserPeek()->Type != TokenRightVectorBracket && ParserPeek()->Type != TokenEof) {
			ParseExpression();
			++count;
		}

		DIAG_PANIC_ON_ERR(DynArenaAlloc(&g_parser.ArraysArena, (void**)&vectorLit->MxLiteral.Matrix, count * sizeof(ASTNode*)));

		g_tokenizer = backup;

		usz i = 0;
		while (ParserPeek()->Type != TokenRightVectorBracket && ParserPeek()->Type != TokenEof) {
			vectorLit->MxLiteral.Matrix[i] = ParseExpression();
			++i;
		}

		if (!ParserMatch(TokenRightVectorBracket)) {
			DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc, DIAG_ARG_TOKEN_TYPE(TokenRightVectorBracket));
			ParserSynchronize();
			return nullptr;
		}

		vectorLit->MxLiteral.Shape.Height = i;

		return vectorLit;
	}

	DIAG_EMIT(DiagUnexpectedToken, token.Loc, DIAG_ARG_TOKEN(token));
	ParserSynchronize();
	return nullptr;
}

static ASTNode* ParsePostfix()
{
	ASTNode* primary = ParsePrimary();

	if (ParserPeek()->Type == TokenTranspose) {
		TokenType operator = ParserPeek()->Type;
		ParserAdvance();

		ASTNode* postfix;
		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&postfix));

		postfix->Type = ASTNodeUnary;
		postfix->Loc = primary->Loc;
		postfix->Unary.Operator = operator;
		postfix->Unary.Operand = primary;
		return postfix;
	}

	return primary;
}

static ASTNode* ParseExponent()
{
	ASTNode* left = ParsePostfix();

	while (ParserPeek()->Type == TokenToPower) {
		TokenType operator = ParserPeek()->Type;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right = ParsePostfix();

		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&left));

		left->Type = ASTNodeBinary;
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
	}

	return left;
}

static ASTNode* ParseUnary()
{
	if (ParserPeek()->Type == TokenSubtract) {
		TokenType operator = ParserPeek()->Type;
		SourceLoc loc = ParserPeek()->Loc;
		ParserAdvance();

		ASTNode* operand = ParseUnary();

		ASTNode* unary;
		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&unary));

		unary->Type = ASTNodeUnary;
		unary->Loc = loc;
		unary->Unary.Operator = operator;
		unary->Unary.Operand = operand;
		return unary;
	}

	return ParseExponent();
}

static ASTNode* ParseFactor()
{
	ASTNode* left = ParseUnary();

	while (ParserPeek()->Type == TokenMultiply || ParserPeek()->Type == TokenDivide) {
		TokenType operator = ParserPeek()->Type;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right = ParseUnary();

		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&left));

		left->Type = ASTNodeBinary;
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
	}

	return left;
}

static ASTNode* ParseTerm()
{
	ASTNode* left = ParseFactor();

	while (ParserPeek()->Type == TokenAdd || ParserPeek()->Type == TokenSubtract) {
		TokenType operator = ParserPeek()->Type;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right = ParseFactor();

		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&left));

		left->Type = ASTNodeBinary;
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
	}

	return left;
}

static ASTNode* ParseComparison()
{
	ASTNode* left = ParseTerm();

	while (ParserPeek()->Type == TokenLess || ParserPeek()->Type == TokenLessEqual || ParserPeek()->Type == TokenGreater
		|| ParserPeek()->Type == TokenGreaterEqual) {
		TokenType operator = ParserPeek()->Type;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right = ParseTerm();

		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&left));

		left->Type = ASTNodeBinary;
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
	}

	return left;
}

static ASTNode* ParseEquality()
{
	ASTNode* left = ParseComparison();

	while (ParserPeek()->Type == TokenEqualEqual || ParserPeek()->Type == TokenNotEqual) {
		TokenType operator = ParserPeek()->Type;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right = ParseComparison();

		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&left));

		left->Type = ASTNodeBinary;
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
	}

	return left;
}

static ASTNode* ParseLogicAnd()
{
	ASTNode* left = ParseEquality();

	while (ParserPeek()->Type == TokenAnd) {
		TokenType operator = ParserPeek()->Type;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right = ParseEquality();

		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&left));

		left->Type = ASTNodeBinary;
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
	}

	return left;
}

static ASTNode* ParseExpression()
{
	ASTNode* left = ParseLogicAnd();

	while (ParserPeek()->Type == TokenOr) {
		TokenType operator = ParserPeek()->Type;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right = ParseLogicAnd();

		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&left));

		left->Type = ASTNodeBinary;
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
	}

	return left;
}

static ASTNode* ParseExprOrAssignment()
{
	Token* token = ParserPeek();
	SourceLoc loc = token->Loc;
	if (token->Type == TokenIdentifier) {
		SymbolView identifier = token->Lexeme;

		Tokenizer backup = g_tokenizer;

		ParserAdvance();

		ASTNode* indexSuffix = nullptr;
		if (ParserMatch(TokenLeftSquareBracket)) {
			ASTNode* i = ParseExpression();
			ASTNode* j = nullptr;

			token = ParserPeek();
			if (token->Type != TokenRightSquareBracket) {
				j = ParseExpression();
			}

			if (!ParserMatch(TokenRightSquareBracket)) {
				DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc, DIAG_ARG_TOKEN_TYPE(TokenRightSquareBracket));
				ParserSynchronize();
				return nullptr;
			}

			DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&indexSuffix));

			indexSuffix->Type = ASTNodeIndexSuffix;
			indexSuffix->IndexSuffix.I = i;
			indexSuffix->IndexSuffix.J = j;
		}

		// This is an assignment
		if (ParserMatch(TokenEqual)) {
			ASTNode* assignment;
			DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&assignment));

			assignment->Type = ASTNodeAssignment;
			assignment->Loc = loc;
			assignment->Assignment.Identifier = identifier;
			assignment->Assignment.Index = indexSuffix;
			assignment->Assignment.Expression = ParseExpression();

			return assignment;
		}

		// Not an assignment
		g_tokenizer = backup;
	}

	return ParseExpression();
}

static ASTNode* ParseIfStmt()
{
	SourceLoc loc = ParserPeek()->Loc;
	ParserAdvance();

	ASTNode* condition = ParseExpression();

	ASTNode* body = ParseBlock();

	ASTNode* elseBody = nullptr;
	if (ParserMatch(TokenElse)) {
		Token* token = ParserPeek();
		if (token->Type == TokenIf) {
			elseBody = ParseIfStmt();
		} else {
			elseBody = ParseBlock();
		}
	}

	ASTNode* ifStmt;
	DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&ifStmt));

	ifStmt->Type = ASTNodeIfStmt;
	ifStmt->Loc = loc;
	ifStmt->IfStmt.Condition = condition;
	ifStmt->IfStmt.ThenBlock = body;
	ifStmt->IfStmt.ElseBlock = elseBody;

	return ifStmt;
}

static ASTNode* ParseWhileStmt()
{
	SourceLoc loc = ParserPeek()->Loc;
	ParserAdvance();

	ASTNode* condition = ParseExpression();

	ASTNode* body = ParseBlock();

	ASTNode* whileStmt;
	DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&whileStmt));

	whileStmt->Type = ASTNodeWhileStmt;
	whileStmt->Loc = loc;
	whileStmt->WhileStmt.Body = body;
	whileStmt->WhileStmt.Condition = condition;
	return whileStmt;
}

static ASTNode* ParseVarDecl()
{
	Token* token = ParserConsume();
	SourceLoc loc = token->Loc;
	bool isConst = token->Type == TokenConst;

	token = ParserPeek();
	if (token->Type != TokenIdentifier) {
		DIAG_EMIT(DiagExpectedToken, token->Loc, DIAG_ARG_TOKEN_TYPE(TokenIdentifier));
		ParserSynchronize();
		return nullptr;
	}

	ASTNode* varDecl;
	DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&varDecl));

	varDecl->Type = ASTNodeVarDecl;
	varDecl->Loc = loc;
	varDecl->VarDecl.IsConst = isConst;
	varDecl->VarDecl.HasDeclaredShape = false;
	varDecl->VarDecl.Identifier = ParserPeek()->Lexeme;
	ParserAdvance();

	if (ParserMatch(TokenColon)) {
		token = ParserConsume();
		if (token->Type != TokenMatrixShape) {
			DIAG_EMIT(DiagExpectedToken, token->Loc, DIAG_ARG_TOKEN_TYPE(TokenMatrixShape));
			ParserSynchronize();
			return nullptr;
		}

		if (token->MatrixShape.Height < 1 || token->MatrixShape.Width < 1) {
			DIAG_EMIT0(DiagInvalidMxShape, token->Loc);
			ParserSynchronize();
			return nullptr;
		}

		varDecl->VarDecl.Shape = token->MatrixShape;
		varDecl->VarDecl.HasDeclaredShape = true;
	}

	if (!ParserMatch(TokenEqual)) {
		if (!varDecl->VarDecl.HasDeclaredShape) {
			DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc, DIAG_ARG_TOKEN_TYPE(TokenColon));
			ParserSynchronize();
			return nullptr;
		}

		varDecl->VarDecl.Expression = nullptr;
		return varDecl;
	}

	varDecl->VarDecl.Expression = ParseExpression();

	return varDecl;
}

static ASTNode* ParseBlock()
{
	SourceLoc loc = ParserPeek()->Loc;
	if (!ParserMatch(TokenLeftCurlyBracket)) {
		DIAG_EMIT(DiagExpectedToken, loc, DIAG_ARG_TOKEN_TYPE(TokenLeftCurlyBracket));
		ParserSynchronize();
		return nullptr;
	}

	ASTNode* block;
	DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&block));

	block->Type = ASTNodeBlock;
	block->Loc = loc;

	Tokenizer backup = g_tokenizer;

	usz count = 0;
	while (!ParserMatch(TokenRightCurlyBracket)) {
		Token* token = ParserPeek();
		if (token->Type == TokenEof) {
			DIAG_EMIT(DiagExpectedToken, token->Loc, DIAG_ARG_TOKEN_TYPE(TokenRightCurlyBracket));
			ParserSynchronize();
			return nullptr;
		}

		ParseStatement();
		++count;
	}

	DIAG_PANIC_ON_ERR(DynArenaAlloc(&g_parser.ArraysArena, (void**)&block->Block.Nodes, count * sizeof(ASTNode*)));

	g_tokenizer = backup;

	usz i = 0;
	while (!ParserMatch(TokenRightCurlyBracket)) {
		Token* token = ParserPeek();
		if (token->Type == TokenEof) {
			DIAG_EMIT(DiagExpectedToken, token->Loc, DIAG_ARG_TOKEN_TYPE(TokenRightCurlyBracket));
			ParserSynchronize();
			return nullptr;
		}

		ASTNode* statement = ParseStatement();

		block->Block.Nodes[i] = statement;

		++i;
	}

	block->Block.NodeCount = i;

	return block;
}

static ASTNode* ParseStatement()
{
	Token* token = ParserPeek();
	switch (token->Type) {
	case TokenConst:
	case TokenLet:
		return ParseVarDecl();
	case TokenIf:
		return ParseIfStmt();
	case TokenWhile:
		return ParseWhileStmt();
	case TokenLeftCurlyBracket:
		return ParseBlock();
	default:
		return ParseExprOrAssignment();
	}
}

void ParserParse()
{
	ASTNode* topLevelBlock;
	DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_parser.ASTArena, (void**)&topLevelBlock));

	topLevelBlock->Type = ASTNodeBlock;

	Tokenizer backup = g_tokenizer;

	usz count = 0;
	while (true) {
		Token* token = ParserPeek();
		if (token->Type == TokenEof) {
			break;
		}

		ASTNode* node = ParseStatement();
		if (!node) {
			continue;
		}

		++count;
	}

	if (count < 1) {
		return;
	}

	DIAG_PANIC_ON_ERR(DynArenaAllocZeroed(&g_parser.ArraysArena, (void**)&topLevelBlock->Block.Nodes, count * sizeof(ASTNode*)));

	g_tokenizer = backup;

	usz i = 0;
	while (true) {
		Token* token = ParserPeek();
		if (token->Type == TokenEof) {
			break;
		}

		ASTNode* node = ParseStatement();
		if (!node) {
			continue;
		}

		topLevelBlock->Block.Nodes[i] = node;

		++i;
	}

	topLevelBlock->Block.NodeCount = i;

	if (i < 1) {
		DIAG_EMIT0(DiagEmptyFileParsed, ParserPeek()->Loc);
	}
}

void ParserPrintAST(const ASTNode* node, usz indents)
{
	for (usz i = 0; i < indents; ++i) {
		putchar(' ');
	}

	if (!node) {
		printf("(error)");
		return;
	}

	switch (node->Type) {
	case ASTNodeNumber:
		printf("%lf", node->Number);
		break;
	case ASTNodeMxLiteral:
		printf("(lit %zux%zu ", node->MxLiteral.Shape.Height, node->MxLiteral.Shape.Width);
		for (size_t i = 0; i < node->MxLiteral.Shape.Height; ++i) {
			printf("(row ");
			for (size_t j = 0; j < node->MxLiteral.Shape.Width; ++j) {
				ParserPrintAST(node->MxLiteral.Matrix[(i * node->MxLiteral.Shape.Width) + j], 0);

				if (j < node->MxLiteral.Shape.Width - 1) {
					printf(" ");
				}
			}
			printf(")");
		}
		printf(")");
		break;
	case ASTNodeBlock:
		printf("(block\n");
		for (size_t i = 0; i < node->Block.NodeCount; ++i) {
			ParserPrintAST(node->Block.Nodes[i], indents + 2);
			printf("\n");
		}
		for (usz i = 0; i < indents; ++i) {
			putchar(' ');
		}
		printf(")");
		break;
	case ASTNodeUnary:
		printf("(un %u ", node->Unary.Operator);
		ParserPrintAST(node->Unary.Operand, 0);
		printf(")");
		break;
	case ASTNodeGrouping:
		printf("(grouping ");
		ParserPrintAST(node->Grouping.Expression, 0);
		printf(")");
		break;
	case ASTNodeBinary:
		printf("(bin %u ", node->Binary.Operator);
		ParserPrintAST(node->Binary.Left, 0);
		printf(" ");
		ParserPrintAST(node->Binary.Right, 0);
		printf(")");
		break;
	case ASTNodeVarDecl:
		if (node->VarDecl.IsConst) {
			printf("(const ");
		} else {
			printf("(let ");
		}
		printf("%.*s", (i32)node->VarDecl.Identifier.SymbolLength, node->VarDecl.Identifier.Symbol);
		if (node->VarDecl.HasDeclaredShape) {
			printf(": %zux%zu", node->VarDecl.Shape.Height, node->VarDecl.Shape.Width);
		}
		printf(" = ");
		ParserPrintAST(node->VarDecl.Expression, 0);
		printf(")");
		break;
	case ASTNodeWhileStmt:
		printf("(while ");
		ParserPrintAST(node->WhileStmt.Condition, 0);
		printf("\n");
		ParserPrintAST(node->WhileStmt.Body, indents + 2);
		printf("\n");
		for (usz i = 0; i < indents; ++i) {
			putchar(' ');
		}
		printf(")");
		break;
	case ASTNodeIfStmt:
		printf("(if ");
		ParserPrintAST(node->IfStmt.Condition, 0);
		printf(" then\n");
		ParserPrintAST(node->IfStmt.ThenBlock, indents + 2);
		if (node->IfStmt.ElseBlock) {
			printf(" else\n");
			ParserPrintAST(node->IfStmt.ElseBlock, indents + 2);
		}
		printf("\n");
		for (usz i = 0; i < indents; ++i) {
			putchar(' ');
		}
		printf(")");
		break;
	case ASTNodeIndexSuffix:
		printf("(index ");
		ParserPrintAST(node->IndexSuffix.I, 0);
		if (node->IndexSuffix.J) {
			printf(" ");
			ParserPrintAST(node->IndexSuffix.J, 0);
		}
		printf(")");
		break;
	case ASTNodeAssignment:
		printf("(assignment %.*s", (i32)node->Assignment.Identifier.SymbolLength, node->Assignment.Identifier.Symbol);
		if (node->Assignment.Index) {
			ParserPrintAST(node->Assignment.Index, 0);
		}
		printf(" ");
		ParserPrintAST(node->Assignment.Expression, 0);
		printf(")");
		break;
	case ASTNodeIdentifier:
		printf("(ident %.*s", (i32)node->Identifier.Identifier.SymbolLength, node->Identifier.Identifier.Symbol);
		if (node->Identifier.Index) {
			ParserPrintAST(node->Identifier.Index, 0);
		}
		printf(")");
		break;
	case ASTNodeFunctionCall:
		printf("(call %.*s", (i32)node->FnCall.Identifier.SymbolLength, node->FnCall.Identifier.Symbol);
		for (size_t i = 0; i < node->FnCall.ArgCount; ++i) {
			printf(" (arg ");
			ParserPrintAST(node->FnCall.CallArgs[i], 0);
			printf(")");
		}
		printf(")");
		break;
	}
}

void ParserInit()
{
	DIAG_PANIC_ON_ERR(StatArenaInit(&g_parser.ASTArena, sizeof(ASTNode)));

	DIAG_PANIC_ON_ERR(DynArenaInit(&g_parser.ArraysArena));
}

void ParserDeinit()
{
	DIAG_PANIC_ON_ERR(DynArenaDeinit(&g_parser.ArraysArena));

	DIAG_PANIC_ON_ERR(StatArenaDeinit(&g_parser.ASTArena));
}
