#include "Parser.h"
#include "Diagnostics.h"

Parser g_parser = { 0 };

static Token* ParserConsume()
{
	Token* token = g_parser.CurToken;
	Result result = StatArenaIterNext(&g_tokenizer.ArenaTokens, &g_parser.Iter);
	g_parser.CurToken = result == ResOk ? g_parser.Iter.Item : nullptr;

	return token;
}

static void ParserAdvance() { StatArenaIterNext(&g_tokenizer.ArenaTokens, &g_parser.Iter); }

static bool ParserMatch(TokenType type)
{
	Token* token = ParserConsume();

	if (token->Type != type) {
		StatArenaIterBack(&g_tokenizer.ArenaTokens, &g_parser.Iter);
		g_parser.CurToken = g_parser.Iter.Item;
		return false;
	}

	return true;
}

static void ParserSynchronize()
{
	while (true) {
		Token* token = ParserConsume();

		if (!token) {
			return;
		}

		switch (token->Type) {
		case TokenLet:
		case TokenConst:
		case TokenIf:
		case TokenWhile:
		case TokenLeftCurlyBracket:
			return;
		default:
		}
	}
}

static Result ParseStatement(ASTNode** node);
static Result ParseBlock(ASTNode** node);
static Result ParseVarDecl(ASTNode** node);
static Result ParseWhileStmt(ASTNode** node);
static Result ParseIsStmt(ASTNode** node);
static Result ParseExprOrAssignment(ASTNode** node);
static Result ParseExpression(ASTNode** node);
static Result ParseLogicAnd(ASTNode** node);
static Result ParseEquality(ASTNode** node);
static Result ParseComparison(ASTNode** node);
static Result ParseTerm(ASTNode** node);
static Result ParseFactor(ASTNode** node);
static Result ParseExponent(ASTNode** node);
static Result ParseUnary(ASTNode** node);
static Result ParsePostfix(ASTNode** node);
static Result ParsePrimary(ASTNode** node);
static Result ParseIdentifierPrimary(ASTNode** node);

static Result ParsePrimary(ASTNode** node)
{
	if (g_parser.CurToken->Type == TokenNumber) {
		ASTNode* number;
		Result result = StatArenaAlloc(&g_parser.ASTArena, (void**)&number);
		if (result) {
			return result;
		}

		number->Type = ASTNodeLiteral;
		result = DynArenaAlloc(&g_parser.ArraysArena, (void**)&number->Data.Literal.Matrix, sizeof(ASTNode*));
		if (result) {
			return result;
		}

		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&number->Data.Literal.Matrix[0]);
		if (result) {
			return result;
		}

		ParserAdvance();

		number->Data.Literal.Matrix[0]->Type = ASTNodeNumber;
		number->Data.Literal.Matrix[0]->Data.Number = g_parser.CurToken->Number;
		number->Data.Literal.Width = 1;
		number->Data.Literal.Height = 1;

		*node = number;
		return ResOk;
	}

	if (ParserMatch(TokenLeftRoundBracket)) {
		ASTNode* expression;
		Result result = ParseExpression(&expression);
		if (result) {
			return result;
		}

		ASTNode* grouping;
		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&grouping);
		if (result) {
			return result;
		}

		grouping->Type = ASTNodeGrouping;
		grouping->Data.Grouping.Expression = expression;

		if (!ParserMatch(TokenRightRoundBracket)) {
			// TODO: Source location
			DIAG_EMIT(DiagExpectedToken, 1, 1, DIAG_ARG_CHAR('a'));
			ParserSynchronize();
			*node = nullptr;
			return ResOk;
		}

		*node = grouping;
		return ResOk;
	}

	if (parser->Tokens[parser->CurrentToken].Type == TokenIdentifier) {
		return ParseIdentifierPrimary(parser);
	}

	if (g_parser.CurToken->Type == TokenLeftSquareBracket) {
		ParserAdvance();

		ASTNode* matrixLit;
		Result result = StatArenaAlloc(&g_parser.ASTArena, (void**)&matrixLit);
		if (result) {
			return result;
		}

		matrixLit->Type = ASTNodeLiteral;
		matrixLit->Data.Literal.Matrix = (ASTNode**)malloc(64 * sizeof(ASTNode*));

		if (ParserMatch(parser, TokenRightSquareBracket)) {
			printf("Empty matrix row literals not allowed\n");
		}

		size_t currentTokenBackup = parser->CurrentToken;
		size_t height = 0;
		size_t width = 0;
		size_t maxWidth = 0;
		do {
			while (parser->Tokens[parser->CurrentToken].Type != TokenRightSquareBracket) {
				ParseExpression(parser);
				++width;
			}

			ParserExpect(parser, TokenRightSquareBracket);

			if (width > maxWidth) {
				maxWidth = width;
			}

			++height;
			width = 0;
		} while (ParserMatch(parser, TokenLeftSquareBracket));

		parser->CurrentToken = currentTokenBackup;
		size_t i = 0;
		size_t j = 0;
		do {
			while (parser->Tokens[parser->CurrentToken].Type != TokenRightSquareBracket) {
				matrixLit->Data.Literal.Matrix[(i * maxWidth) + j] = ParseExpression(parser);
				++j;
			}

			while (j < maxWidth) {
				ASTNode* zero = malloc(sizeof(ASTNode));
				zero->Type = Literal;
				zero->Data.Literal.Matrix = (ASTNode**)malloc(sizeof(ASTNode*));
				zero->Data.Literal.Matrix[0] = malloc(sizeof(ASTNode));
				zero->Data.Literal.Matrix[0]->Type = Number;
				zero->Data.Literal.Matrix[0]->Data.Number = 0;
				zero->Data.Literal.Width = 1;
				zero->Data.Literal.Height = 1;

				matrixLit->Data.Literal.Matrix[(i * maxWidth) + j] = zero;

				++j;
			}

			ParserExpect(parser, TokenRightSquareBracket);
			++i;
			j = 0;
		} while (ParserMatch(parser, TokenLeftSquareBracket));

		matrixLit->Data.Literal.Height = height;
		matrixLit->Data.Literal.Width = maxWidth;

		return matrixLit;
	}

	if (parser->Tokens[parser->CurrentToken].Type == TokenLeftVectorBracket) {
		ParserAdvance(parser);

		ASTNode* vectorLit = malloc(sizeof(ASTNode));
		vectorLit->Type = Literal;
		vectorLit->Data.Literal.Width = 1;
		vectorLit->Data.Literal.Matrix = (ASTNode**)malloc(8 * sizeof(ASTNode*));

		if (ParserMatch(parser, TokenRightVectorBracket)) {
			printf("Empty vector literals not allowed\n");
		}

		size_t i = 0;
		while (parser->Tokens[parser->CurrentToken].Type != TokenRightVectorBracket) {
			vectorLit->Data.Literal.Matrix[i] = ParseExpression(parser);
			++i;
		}

		ParserExpect(parser, TokenRightVectorBracket);

		vectorLit->Data.Literal.Height = i;

		return vectorLit;
	}

	printf("Unexpected token %s in primary\n", parser->Tokens[parser->CurrentToken].Lexeme);
	exit(1);
	return nullptr;
}

static Result ParsePostfix(ASTNode** node)
{
	ASTNode* primary;
	Result result = ParsePrimary(&primary);
	if (result) {
		return result;
	}

	if (g_parser.CurToken->Type == TokenTranspose) {
		Token* operator= g_parser.CurToken;
		 ParserAdvance();

		ASTNode* postfix;
		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&postfix);
		if (result) {
			return result;
		}

		postfix->Type = ASTNodeUnary;
		postfix->Data.Unary.Operator = operator;
		postfix->Data.Unary.Operand = primary;
		*node = postfix;
		return ResOk;
	}

	*node = primary;
	return ResOk;
}

static Result ParseUnary(ASTNode** node)
{
	if (g_parser.CurToken->Type == TokenSubtract) {
		Token* operator= g_parser.CurToken;
		ParserAdvance();

		ASTNode* operand;
		Result result = ParseUnary(&operand);
		if (result) {
			return result;
		}

		ASTNode* unary;
		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&unary);
		if (result) {
			return result;
		}

		unary->Type = ASTNodeUnary;
		unary->Data.Unary.Operator = operator;
		unary->Data.Unary.Operand = operand;
		*node = unary;
		return ResOk;
	}

	return ParsePostfix(&node);
}

static Result ParseExponent(ASTNode** node)
{
	ASTNode* left;
	Result result = ParseUnary(&left);
	if (result) {
		return result;
	}

	while (g_parser.CurToken->Type == TokenToPower) {
		Token* operator= g_parser.CurToken;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right;
		result = ParseUnary(&right);
		if (result) {
			return result;
		}

		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&left);
		if (result) {
			return result;
		}

		left->Type = ASTNodeBinary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	*node = left;
	return ResOk;
}

static Result ParseFactor(ASTNode** node)
{
	ASTNode* left;
	Result result = ParseExponent(&left);
	if (result) {
		return result;
	}

	while (g_parser.CurToken->Type == TokenMultiply || g_parser.CurToken->Type == TokenDivide) {
		Token* operator= g_parser.CurToken;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right;
		result = ParseExponent(&right);
		if (result) {
			return result;
		}

		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&left);
		if (result) {
			return result;
		}

		left->Type = ASTNodeBinary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	*node = left;
	return ResOk;
}

static Result ParseTerm(ASTNode** node)
{
	ASTNode* left;
	Result result = ParseFactor(&left);
	if (result) {
		return result;
	}

	while (g_parser.CurToken->Type == TokenAdd || g_parser.CurToken->Type == TokenSubtract) {
		Token* operator= g_parser.CurToken;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right;
		result = ParseFactor(&right);
		if (result) {
			return result;
		}

		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&left);
		if (result) {
			return result;
		}

		left->Type = ASTNodeBinary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	*node = left;
	return ResOk;
}

static Result ParseComparison(ASTNode** node)
{
	ASTNode* left;
	Result result = ParseTerm(&left);
	if (result) {
		return result;
	}

	while (g_parser.CurToken->Type == TokenLess || g_parser.CurToken->Type == TokenLessEqual || g_parser.CurToken->Type == TokenGreater
		|| g_parser.CurToken->Type == TokenGreaterEqual) {
		Token* operator= g_parser.CurToken;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right;
		result = ParseTerm(&right);
		if (result) {
			return result;
		}

		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&left);
		if (result) {
			return result;
		}

		left->Type = ASTNodeBinary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	*node = left;
	return ResOk;
}

static Result ParseEquality(ASTNode** node)
{
	ASTNode* left;
	Result result = ParseComparison(&left);
	if (result) {
		return result;
	}

	while (g_parser.CurToken->Type == TokenEqualEqual || g_parser.CurToken->Type == TokenNotEqual) {
		Token* operator= g_parser.CurToken;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right;
		result = ParseComparison(&right);
		if (result) {
			return result;
		}

		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&left);
		if (result) {
			return result;
		}

		left->Type = ASTNodeBinary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	*node = left;
	return ResOk;
}

static Result ParseLogicAnd(ASTNode** node)
{
	ASTNode* left;
	Result result = ParseEquality(&left);
	if (result) {
		return result;
	}

	while (g_parser.CurToken->Type == TokenAnd) {
		Token* operator= g_parser.CurToken;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right;
		result = ParseEquality(&right);
		if (result) {
			return result;
		}

		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&left);
		if (result) {
			return result;
		}

		left->Type = ASTNodeBinary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	*node = left;
	return ResOk;
}

static Result ParseExpression(ASTNode** node)
{
	ASTNode* left;
	Result result = ParseLogicAnd(&left);
	if (result) {
		return result;
	}

	while (g_parser.CurToken->Type == TokenOr) {
		Token* operator= g_parser.CurToken;
		ParserAdvance();

		ASTNode* temp = left;

		ASTNode* right;
		result = ParseLogicAnd(&right);
		if (result) {
			return result;
		}

		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&left);
		if (result) {
			return result;
		}

		left->Type = ASTNodeBinary;
		left->Data.Binary.Left = temp;
		left->Data.Binary.Operator = operator;
		left->Data.Binary.Right = right;
	}

	*node = left;
	return ResOk;
}

static Result ParseExprOrAssignment(ASTNode** node)
{
	if (g_parser.CurToken->Type == TokenIdentifier) {
		Token* identifier = g_parser.CurToken;

		Parser backup = g_parser;

		ParserAdvance();

		ASTNode* indexSuffix = nullptr;
		if (ParserMatch(TokenLeftSquareBracket)) {
			ASTNode* i;
			Result result = ParseExpression(&i);
			if (result) {
				return result;
			}
			ASTNode* j = nullptr;

			if (g_parser.CurToken->Type != TokenRightSquareBracket) {
				result = ParseExpression(&j);
				if (result) {
					return result;
				}
			}

			if (!ParserMatch(TokenRightSquareBracket)) {
				// TODO: Source location
				DIAG_EMIT(DiagExpectedToken, 1, 1, DIAG_ARG_CHAR(']'));
				ParserSynchronize();
				*node = nullptr;
				return ResOk;
			}

			result = StatArenaAlloc(&g_parser.ASTArena, (void**)&indexSuffix);
			if (result) {
				return result;
			}

			indexSuffix->Type = ASTNodeIndexSuffix;
			indexSuffix->Data.IndexSuffix.I = i;
			indexSuffix->Data.IndexSuffix.J = j;
		}

		// This is an assignment
		if (ParserMatch(TokenEqual)) {
			ASTNode* assignment;
			Result result = StatArenaAlloc(&g_parser.ASTArena, (void**)&assignment);
			if (result) {
				return result;
			}

			assignment->Type = ASTNodeAssignment;
			assignment->Data.Assignment.Identifier = identifier;
			assignment->Data.Assignment.Index = indexSuffix;
			result = ParseExpression(&assignment->Data.Assignment.Expression);
			if (result) {
				return result;
			}

			*node = assignment;
			return ResOk;
		}

		// Not an assignment
		g_parser = backup;
	}

	return ParseExpression(node);
}

static Result ParseIfStmt(ASTNode** node)
{
	ParserAdvance();

	ASTNode* condition;
	Result result = ParseExpression(&condition);
	if (result) {
		return result;
	}

	ASTNode* body;
	result = ParseBlock(&body);
	if (result) {
		return result;
	}

	ASTNode* elseBody = nullptr;
	if (ParserMatch(TokenElse)) {
		if (g_parser.CurToken->Type == TokenIf) {
			result = ParseIfStmt(&elseBody);
			if (result) {
				return result;
			}
		} else {
			result = ParseBlock(&elseBody);
			if (result) {
				return result;
			}
		}
	}

	ASTNode* ifStmt;
	result = StatArenaAlloc(&g_parser.ASTArena, (void**)&ifStmt);
	if (result) {
		return result;
	}

	ifStmt->Type = ASTNodeIfStmt;
	ifStmt->Data.IfStmt.Condition = condition;
	ifStmt->Data.IfStmt.ThenBlock = body;
	ifStmt->Data.IfStmt.ElseBlock = elseBody;

	*node = ifStmt;
	return ResOk;
}

static Result ParseWhileStmt(ASTNode** node)
{
	ParserAdvance();

	ASTNode* condition;
	Result result = ParseExpression(&condition);
	if (result) {
		return result;
	}

	ASTNode* body;
	result = ParseBlock(&body);
	if (result) {
		return result;
	}

	ASTNode* whileStmt;
	result = StatArenaAlloc(&g_parser.ASTArena, (void**)&whileStmt);
	if (result) {
		return result;
	}

	whileStmt->Type = ASTNodeWhileStmt;
	whileStmt->Data.WhileStmt.Body = body;
	whileStmt->Data.WhileStmt.Condition = condition;
	*node = whileStmt;
	return ResOk;
}

static Result ParseVarDecl(ASTNode** node)
{
	bool isConst = g_parser.CurToken->Type == TokenConst;

	if (!ParserMatch(TokenIdentifier)) {
		// TODO: Source location
		DIAG_EMIT(DiagExpectedToken, 1, 1, DIAG_ARG_STRING("identifier"));
		ParserSynchronize();
		*node = nullptr;
		return ResOk;
	}

	ASTNode* varDecl;
	Result result = StatArenaAlloc(&g_parser.ASTArena, (void**)&varDecl);
	if (result) {
		return result;
	}

	varDecl->Type = ASTNodeVarDecl;
	varDecl->Data.VarDecl.IsConst = isConst;
	varDecl->Data.VarDecl.Type = nullptr;
	varDecl->Data.VarDecl.Identifier = g_parser.CurToken;

	if (ParserMatch(TokenColon)) {
		if (!ParserMatch(TokenMatrixShape)) {
			// TODO: Source location
			DIAG_EMIT(DiagExpectedToken, 1, 1, DIAG_ARG_STRING("matrix shape declaration"));
			ParserSynchronize();
			*node = nullptr;
			return ResOk;
		}

		varDecl->Data.VarDecl.Type = g_parser.CurToken;
	}

	if (!ParserMatch(TokenEqual)) {
		if (!varDecl->Data.VarDecl.Type) {
			// TODO: Source location
			DIAG_EMIT(DiagExpectedToken, 1, 1, DIAG_ARG_STRING("assignment without explicit matrix shape"));
			ParserSynchronize();
			*node = nullptr;
			return ResOk;
		}

		varDecl->Data.VarDecl.Expression = nullptr;
		*node = varDecl;
		return ResOk;
	}

	result = ParseExpression(&varDecl->Data.VarDecl.Expression);
	if (result) {
		return result;
	}

	*node = varDecl;
	return ResOk;
}

static Result ParseBlock(ASTNode** node)
{
	if (!ParserMatch(TokenLeftCurlyBracket)) {
		// TODO: Source location
		DIAG_EMIT(DiagExpectedToken, 1, 1, DIAG_ARG_STRING("at block start"));
		ParserSynchronize();
		*node = nullptr;
		return ResOk;
	}

	ASTNode* block;
	Result result = StatArenaAlloc(&g_parser.ASTArena, (void**)&block);
	if (result) {
		return result;
	}

	block->Type = ASTNodeBlock;
	result = DynArenaAlloc(&g_parser.ArraysArena, (void**)&block->Data.Block.Nodes, 8 * sizeof(ASTNode*));
	if (result) {
		return result;
	}

	usz i = 0;
	while (!ParserMatch(TokenRightCurlyBracket)) {
		ASTNode* statement;
		result = ParseStatement(&statement);
		if (result) {
			return result;
		}

		if (!statement) {
			continue;
		}

		block->Data.Block.Nodes[i] = statement;

		++i;
	}

	if (!ParserMatch(TokenRightCurlyBracket)) {
		DIAG_EMIT(DiagExpectedToken, g_parser.CurToken->SourceLine, g_parser.CurToken->SourceLinePos - 1, DIAG_ARG_STRING("at block end"));
		ParserSynchronize();
		*node = nullptr;
		return ResOk;
	}

	*node = block;
	return ResOk;
}

static Result ParseStatement(ASTNode** node)
{
	switch (g_parser.CurToken->Type) {
	case TokenConst:
	case TokenLet:
		return ParseVarDecl(node);
	case TokenIf:
		return ParseIfStmt(node);
	case TokenWhile:
		return ParseWhileStmt(node);
	case TokenLeftCurlyBracket:
		return ParseBlock(node);
	default:
		return ParseExprOrAssignment(node);
	}
}

Result ParserParse()
{
	ASTNode* topLevelBlock;
	Result result = StatArenaAlloc(&g_parser.ASTArena, (void**)&topLevelBlock);
	if (result) {
		return result;
	}

	topLevelBlock->Type = ASTNodeBlock;
	result = DynArenaAllocZeroed(&g_parser.ArraysArena, (void**)&topLevelBlock->Data.Block.Nodes, 8 * sizeof(ASTNode*));
	if (result) {
		return result;
	}

	usz i = 0;
	while (true) {
		Token* curToken = ParserConsume();
		if (!curToken) {
			break;
		}

		ASTNode* node;
		result = ParseStatement(&node);
		if (result) {
			return result;
		}

		if (!node) {
			continue;
		}

		topLevelBlock->Data.Block.Nodes[i] = node;

		++i;
	}

	topLevelBlock->Data.Block.NodeCount = i;

	return ResOk;
}

Result ParserInit()
{
	Result result = StatArenaInit(&g_parser.ASTArena, sizeof(ASTNode));
	if (result) {
		return result;
	}

	return DynArenaInit(&g_parser.ArraysArena);
}

Result ParserDeinit()
{
	Result result = DynArenaDeinit(&g_parser.ArraysArena);
	if (result) {
		return result;
	}

	return StatArenaDeinit(&g_parser.ASTArena);
}
