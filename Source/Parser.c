#include "Parser.h"
#include "Diagnostics.h"
#include <stdio.h>

Parser g_parser = { 0 };

static void ParserAdvance()
{
	Token* token;
	Result result = TokenizerNextToken(&token);
}

static Token* ParserPeek()
{
	Token* token;
	Result result = TokenizerPeekToken(&token);

	return token;
}

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

		if (token->Type == TokenEof) {
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
			ParserAdvance();
			break;
		}
	}
}

static Result ParseStatement(ASTNode** node);
static Result ParseBlock(ASTNode** node);
static Result ParseVarDecl(ASTNode** node);
static Result ParseWhileStmt(ASTNode** node);
static Result ParseIfStmt(ASTNode** node);
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

static Result ParseIdentifierPrimary(ASTNode** node)
{
	SourceLoc loc = ParserPeek()->Loc;
	SymbolView identifier = ParserConsume()->Lexeme;

	// A function call
	if (ParserMatch(TokenLeftRoundBracket)) {
		ASTNode* functionCall;
		Result result = StatArenaAlloc(&g_parser.ASTArena, (void**)&functionCall);
		if (result) {
			return result;
		}

		functionCall->Type = ASTNodeFunctionCall;
		functionCall->Loc = loc;
		functionCall->FunctionCall.Identifier = identifier;
		result = DynArenaAlloc(&g_parser.ArraysArena, (void**)&functionCall->FunctionCall.CallArgs, 8 * sizeof(ASTNode*));
		if (result) {
			return result;
		}

		usz i = 0;
		while (ParserPeek()->Type != TokenRightRoundBracket) {
			if (i > 0) {
				if (!ParserMatch(TokenComma)) {
					// TODO: Source location
					DIAG_EMIT(DiagExpectedToken, 1, 1, DIAG_ARG_CHAR(','));
					ParserSynchronize();
					*node = nullptr;
					return ResOk;
				}
			}

			result = ParseExpression(&functionCall->FunctionCall.CallArgs[i]);
			if (result) {
				return result;
			}
			++i;
		}

		if (!ParserMatch(TokenRightRoundBracket)) {
			// TODO: Source location
			DIAG_EMIT(DiagExpectedToken, 1, 1, DIAG_ARG_CHAR(')'));
			ParserSynchronize();
			*node = nullptr;
			return ResOk;
		}
		functionCall->FunctionCall.ArgCount = i;

		*node = functionCall;
		return ResOk;
	}

	// Not a function call
	ASTNode* indexSuffix = nullptr;
	if (ParserMatch(TokenLeftSquareBracket)) {
		ASTNode* i;
		Result result = ParseExpression(&i);
		if (result) {
			return result;
		}
		ASTNode* j = nullptr;

		if (ParserPeek()->Type != TokenRightSquareBracket) {
			result = ParseExpression(&j);
			if (result) {
				return result;
			}
		}

		if (!ParserMatch(TokenRightSquareBracket)) {
			// TODO: Source location
			DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc.Line, ParserPeek()->Loc.LinePos, DIAG_ARG_CHAR(']'));
			ParserSynchronize();
			*node = nullptr;
			return ResOk;
		}

		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&indexSuffix);
		if (result) {
			return result;
		}

		indexSuffix->Type = ASTNodeIndexSuffix;
		indexSuffix->IndexSuffix.I = i;
		indexSuffix->IndexSuffix.J = j;
	}

	ASTNode* astIdentifier;
	Result result = StatArenaAlloc(&g_parser.ASTArena, (void**)&astIdentifier);
	if (result) {
		return result;
	}

	astIdentifier->Type = ASTNodeIdentifier;
	astIdentifier->Loc = loc;
	astIdentifier->Identifier.Identifier = identifier;
	astIdentifier->Identifier.Index = indexSuffix;

	*node = astIdentifier;
	return ResOk;
}

static Result ParsePrimary(ASTNode** node)
{
	Token* token = ParserPeek();
	SourceLoc loc = token->Loc;

	if (token->Type == TokenNumber) {
		ASTNode* number;
		Result result = StatArenaAlloc(&g_parser.ASTArena, (void**)&number);
		if (result) {
			return result;
		}

		number->Type = ASTNodeMxLiteral;
		number->Loc = loc;
		result = DynArenaAlloc(&g_parser.ArraysArena, (void**)&number->MxLiteral.Matrix, sizeof(ASTNode*));
		if (result) {
			return result;
		}

		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&number->MxLiteral.Matrix[0]);
		if (result) {
			return result;
		}

		ParserAdvance();

		number->MxLiteral.Matrix[0]->Type = ASTNodeNumber;
		number->MxLiteral.Matrix[0]->Loc = loc;
		number->MxLiteral.Matrix[0]->Number = token->Number;
		number->MxLiteral.Shape.Height = 1;
		number->MxLiteral.Shape.Width = 1;

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
		grouping->Loc = loc;
		grouping->Grouping.Expression = expression;

		if (!ParserMatch(TokenRightRoundBracket)) {
			// TODO: Source location
			DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc.Line, ParserPeek()->Loc.LinePos, DIAG_ARG_CHAR(')'));
			ParserSynchronize();
			*node = nullptr;
			return ResOk;
		}

		*node = grouping;
		return ResOk;
	}

	if (token->Type == TokenIdentifier) {
		return ParseIdentifierPrimary(node);
	}

	if (ParserMatch(TokenLeftSquareBracket)) {
		ASTNode* matrixLit;
		Result result = StatArenaAlloc(&g_parser.ASTArena, (void**)&matrixLit);
		if (result) {
			return result;
		}

		matrixLit->Type = ASTNodeMxLiteral;
		matrixLit->Loc = loc;
		result = DynArenaAlloc(&g_parser.ArraysArena, (void**)&matrixLit->MxLiteral.Matrix, 64 * sizeof(ASTNode*));
		if (result) {
			return result;
		}

		if (ParserMatch(TokenRightSquareBracket)) {
			// TODO: Source location
			DIAG_EMIT(
				DiagExpectedToken, ParserPeek()->Loc.Line, ParserPeek()->Loc.LinePos, DIAG_ARG_STRING("empty matrix literals not allowed"));
			ParserSynchronize();
			*node = nullptr;
			return ResOk;
		}

		Tokenizer backup = g_tokenizer;
		usz height = 0;
		usz width = 0;
		usz maxWidth = 0;
		do {
			while (ParserPeek()->Type != TokenRightSquareBracket) {
				ParseExpression(node);
				++width;
			}

			if (!ParserMatch(TokenRightSquareBracket)) {
				// TODO: Source location
				DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc.Line, ParserPeek()->Loc.LinePos, DIAG_ARG_CHAR(']'));
				ParserSynchronize();
				*node = nullptr;
				return ResOk;
			}

			if (width > maxWidth) {
				maxWidth = width;
			}

			++height;
			width = 0;
		} while (ParserMatch(TokenLeftSquareBracket));

		g_tokenizer = backup;
		usz i = 0;
		usz j = 0;
		do {
			while (ParserPeek()->Type != TokenRightSquareBracket) {
				result = ParseExpression(&matrixLit->MxLiteral.Matrix[(i * maxWidth) + j]);
				if (result) {
					return result;
				}
				++j;
			}

			while (j < maxWidth) {
				ASTNode* zero;
				result = StatArenaAlloc(&g_parser.ASTArena, (void**)&zero);
				if (result) {
					return result;
				}

				zero->Type = ASTNodeMxLiteral;
				result = DynArenaAlloc(&g_parser.ArraysArena, (void**)&zero->MxLiteral.Matrix, sizeof(ASTNode*));
				if (result) {
					return result;
				}

				result = StatArenaAlloc(&g_parser.ASTArena, (void**)&zero->MxLiteral.Matrix[0]);
				if (result) {
					return result;
				}

				zero->MxLiteral.Matrix[0]->Type = ASTNodeNumber;
				zero->MxLiteral.Matrix[0]->Number = 0;
				zero->MxLiteral.Shape.Height = 1;
				zero->MxLiteral.Shape.Width = 1;

				matrixLit->MxLiteral.Matrix[(i * maxWidth) + j] = zero;

				++j;
			}

			if (!ParserMatch(TokenRightSquareBracket)) {
				// TODO: Source location
				DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc.Line, ParserPeek()->Loc.LinePos, DIAG_ARG_CHAR(']'));
				ParserSynchronize();
				*node = nullptr;
				return ResOk;
			}

			++i;
			j = 0;
		} while (ParserMatch(TokenLeftSquareBracket));

		matrixLit->MxLiteral.Shape.Height = height;
		matrixLit->MxLiteral.Shape.Width = maxWidth;

		*node = matrixLit;
		return ResOk;
	}

	if (token->Type == TokenLeftVectorBracket) {
		ParserAdvance();

		ASTNode* vectorLit;
		Result result = StatArenaAlloc(&g_parser.ASTArena, (void**)&vectorLit);
		if (result) {
			return result;
		}

		vectorLit->Type = ASTNodeMxLiteral;
		vectorLit->Loc = loc;
		vectorLit->MxLiteral.Shape.Width = 1;
		result = DynArenaAlloc(&g_parser.ArraysArena, (void**)&vectorLit->MxLiteral.Matrix, 8 * sizeof(ASTNode*));
		if (result) {
			return result;
		}

		if (ParserMatch(TokenRightVectorBracket)) {
			// TODO: Source location
			DIAG_EMIT(
				DiagExpectedToken, ParserPeek()->Loc.Line, ParserPeek()->Loc.LinePos, DIAG_ARG_STRING("empty vector literals not allowed"));
			ParserSynchronize();
			*node = nullptr;
			return ResOk;
		}

		usz i = 0;
		while (ParserPeek()->Type != TokenRightVectorBracket) {
			result = ParseExpression(&vectorLit->MxLiteral.Matrix[i]);
			if (result) {
				return result;
			}
			++i;
		}

		if (!ParserMatch(TokenRightVectorBracket)) {
			// TODO: Source location
			DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc.Line, ParserPeek()->Loc.LinePos, DIAG_ARG_STRING(">>"));
			ParserSynchronize();
			*node = nullptr;
			return ResOk;
		}

		vectorLit->MxLiteral.Shape.Height = i;

		*node = vectorLit;
		return ResOk;
	}

	DIAG_EMIT(DiagUnexpectedToken, loc.Line, loc.LinePos, DIAG_ARG_STRING("a"));
	ParserSynchronize();
	*node = nullptr;
	return ResOk;
}

static Result ParsePostfix(ASTNode** node)
{
	ASTNode* primary;
	Result result = ParsePrimary(&primary);
	if (result) {
		return result;
	}

	if (ParserPeek()->Type == TokenTranspose) {
		TokenType operator = ParserPeek()->Type;
		ParserAdvance();

		ASTNode* postfix;
		result = StatArenaAlloc(&g_parser.ASTArena, (void**)&postfix);
		if (result) {
			return result;
		}

		postfix->Type = ASTNodeUnary;
		postfix->Loc = primary->Loc;
		postfix->Unary.Operator = operator;
		postfix->Unary.Operand = primary;
		*node = postfix;
		return ResOk;
	}

	*node = primary;
	return ResOk;
}

static Result ParseUnary(ASTNode** node)
{
	if (ParserPeek()->Type == TokenSubtract) {
		TokenType operator = ParserPeek()->Type;
		SourceLoc loc = ParserPeek()->Loc;
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
		unary->Loc = loc;
		unary->Unary.Operator = operator;
		unary->Unary.Operand = operand;
		*node = unary;
		return ResOk;
	}

	return ParsePostfix(node);
}

static Result ParseExponent(ASTNode** node)
{
	ASTNode* left;
	Result result = ParseUnary(&left);
	if (result) {
		return result;
	}

	while (ParserPeek()->Type == TokenToPower) {
		TokenType operator = ParserPeek()->Type;
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
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
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

	while (ParserPeek()->Type == TokenMultiply || ParserPeek()->Type == TokenDivide) {
		TokenType operator = ParserPeek()->Type;
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
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
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

	while (ParserPeek()->Type == TokenAdd || ParserPeek()->Type == TokenSubtract) {
		TokenType operator = ParserPeek()->Type;
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
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
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

	while (ParserPeek()->Type == TokenLess || ParserPeek()->Type == TokenLessEqual || ParserPeek()->Type == TokenGreater
		|| ParserPeek()->Type == TokenGreaterEqual) {
		TokenType operator = ParserPeek()->Type;
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
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
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

	while (ParserPeek()->Type == TokenEqualEqual || ParserPeek()->Type == TokenNotEqual) {
		TokenType operator = ParserPeek()->Type;
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
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
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

	while (ParserPeek()->Type == TokenAnd) {
		TokenType operator = ParserPeek()->Type;
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
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
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

	while (ParserPeek()->Type == TokenOr) {
		TokenType operator = ParserPeek()->Type;
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
		left->Loc = temp->Loc;
		left->Binary.Left = temp;
		left->Binary.Operator = operator;
		left->Binary.Right = right;
	}

	*node = left;
	return ResOk;
}

static Result ParseExprOrAssignment(ASTNode** node)
{
	Token* token = ParserPeek();
	SourceLoc loc = token->Loc;
	if (token->Type == TokenIdentifier) {
		SymbolView identifier = token->Lexeme;

		Tokenizer backup = g_tokenizer;

		ParserAdvance();

		ASTNode* indexSuffix = nullptr;
		if (ParserMatch(TokenLeftSquareBracket)) {
			ASTNode* i;
			Result result = ParseExpression(&i);
			if (result) {
				return result;
			}
			ASTNode* j = nullptr;

			token = ParserPeek();
			if (token->Type != TokenRightSquareBracket) {
				result = ParseExpression(&j);
				if (result) {
					return result;
				}
			}

			if (!ParserMatch(TokenRightSquareBracket)) {
				// TODO: Source location
				DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc.Line, ParserPeek()->Loc.LinePos, DIAG_ARG_CHAR(']'));
				ParserSynchronize();
				*node = nullptr;
				return ResOk;
			}

			result = StatArenaAlloc(&g_parser.ASTArena, (void**)&indexSuffix);
			if (result) {
				return result;
			}

			indexSuffix->Type = ASTNodeIndexSuffix;
			indexSuffix->IndexSuffix.I = i;
			indexSuffix->IndexSuffix.J = j;
		}

		// This is an assignment
		if (ParserMatch(TokenEqual)) {
			ASTNode* assignment;
			Result result = StatArenaAlloc(&g_parser.ASTArena, (void**)&assignment);
			if (result) {
				return result;
			}

			assignment->Type = ASTNodeAssignment;
			assignment->Loc = loc;
			assignment->Assignment.Identifier = identifier;
			assignment->Assignment.Index = indexSuffix;
			result = ParseExpression(&assignment->Assignment.Expression);
			if (result) {
				return result;
			}

			*node = assignment;
			return ResOk;
		}

		// Not an assignment
		g_tokenizer = backup;
	}

	return ParseExpression(node);
}

static Result ParseIfStmt(ASTNode** node)
{
	SourceLoc loc = ParserPeek()->Loc;
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
		Token* token = ParserPeek();
		if (token->Type == TokenIf) {
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
	ifStmt->Loc = loc;
	ifStmt->IfStmt.Condition = condition;
	ifStmt->IfStmt.ThenBlock = body;
	ifStmt->IfStmt.ElseBlock = elseBody;

	*node = ifStmt;
	return ResOk;
}

static Result ParseWhileStmt(ASTNode** node)
{
	SourceLoc loc = ParserPeek()->Loc;
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
	whileStmt->Loc = loc;
	whileStmt->WhileStmt.Body = body;
	whileStmt->WhileStmt.Condition = condition;
	*node = whileStmt;
	return ResOk;
}

static Result ParseVarDecl(ASTNode** node)
{
	Token* token = ParserConsume();
	SourceLoc loc = token->Loc;
	bool isConst = token->Type == TokenConst;

	token = ParserPeek();
	if (token->Type != TokenIdentifier) {
		// TODO: Source location
		DIAG_EMIT(DiagExpectedToken, token->Loc.Line, token->Loc.LinePos, DIAG_ARG_STRING("identifier"));
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
	varDecl->Loc = loc;
	varDecl->VarDecl.IsConst = isConst;
	varDecl->VarDecl.HasDeclaredShape = false;
	varDecl->VarDecl.Identifier = ParserPeek()->Lexeme;
	ParserAdvance();

	if (ParserMatch(TokenColon)) {
		token = ParserConsume();
		if (token->Type != TokenMatrixShape) {
			// TODO: Source location
			DIAG_EMIT(DiagExpectedToken, token->Loc.Line, token->Loc.LinePos, DIAG_ARG_STRING("matrix shape declaration"));
			ParserSynchronize();
			*node = nullptr;
			return ResOk;
		}

		varDecl->VarDecl.Shape = token->MatrixShape;
		varDecl->VarDecl.HasDeclaredShape = true;
	}

	if (!ParserMatch(TokenEqual)) {
		if (!varDecl->VarDecl.HasDeclaredShape) {
			// TODO: Source location
			DIAG_EMIT(DiagExpectedToken, ParserPeek()->Loc.Line, ParserPeek()->Loc.LinePos, DIAG_ARG_CHAR(':'));
			ParserSynchronize();
			*node = nullptr;
			return ResOk;
		}

		varDecl->VarDecl.Expression = nullptr;
		*node = varDecl;
		return ResOk;
	}

	result = ParseExpression(&varDecl->VarDecl.Expression);
	if (result) {
		return result;
	}

	*node = varDecl;
	return ResOk;
}

static Result ParseBlock(ASTNode** node)
{
	SourceLoc loc = ParserPeek()->Loc;
	if (!ParserMatch(TokenLeftCurlyBracket)) {
		DIAG_EMIT(DiagExpectedToken, loc.Line, loc.LinePos, DIAG_ARG_CHAR('{'));
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
	block->Loc = loc;
	result = DynArenaAlloc(&g_parser.ArraysArena, (void**)&block->Block.Nodes, 64 * sizeof(ASTNode*));
	if (result) {
		return result;
	}

	usz i = 0;
	while (!ParserMatch(TokenRightCurlyBracket)) {
		Token* token = ParserPeek();
		if (token->Type == TokenEof) {
			DIAG_EMIT(DiagExpectedToken, token->Loc.Line, token->Loc.LinePos, DIAG_ARG_CHAR('}'));
			ParserSynchronize();
			*node = nullptr;
			return ResOk;
		}

		ASTNode* statement;
		result = ParseStatement(&statement);
		if (result) {
			return result;
		}

		block->Block.Nodes[i] = statement;

		++i;
	}

	block->Block.NodeCount = i;

	*node = block;
	return ResOk;
}

static Result ParseStatement(ASTNode** node)
{
	Token* token = ParserPeek();
	switch (token->Type) {
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
	result = DynArenaAllocZeroed(&g_parser.ArraysArena, (void**)&topLevelBlock->Block.Nodes, 64 * sizeof(ASTNode*));
	if (result) {
		return result;
	}

	usz i = 0;
	while (true) {
		Token* token = ParserPeek();
		if (token->Type == TokenEof) {
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

		topLevelBlock->Block.Nodes[i] = node;

		++i;
	}

	topLevelBlock->Block.NodeCount = i;

	return ResOk;
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
		printf("(call %.*s", (i32)node->FunctionCall.Identifier.SymbolLength, node->FunctionCall.Identifier.Symbol);
		for (size_t i = 0; i < node->FunctionCall.ArgCount; ++i) {
			printf(" (arg ");
			ParserPrintAST(node->FunctionCall.CallArgs[i], 0);
			printf(")");
		}
		printf(")");
		break;
	}
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
