#include "TypeChecker.h"

#include "Diagnostics.h"
#include "Parser.h"
#include <stdio.h>
#include <stdlib.h>

TypeChecker g_typeChecker = { 0 };

static usz GetSymbolID()
{
	static usz id = 0;
	return id++;
}

static Result BindingEnterScope()
{
	BindingScope* temp = g_typeChecker.CurBindingScope;

	Result result = DynArenaAllocZeroed(
		&g_typeChecker.BindingArena, (void**)&g_typeChecker.CurBindingScope, sizeof(BindingScope) + (64 * sizeof(BindingEntry)));
	if (result) {
		return result;
	}

	g_typeChecker.CurBindingScope->Parent = temp;
	return ResOk;
}

static Result BindingExitScope()
{
	// TODO: Deallocate the previous scope!
	g_typeChecker.CurBindingScope = g_typeChecker.CurBindingScope->Parent;
	return ResOk;
}

static Result BindingLookup(BindingScope* scope, const char* name, usz* id)
{
	for (usz i = 0; i < scope->EntryCount; ++i) {
		if (scope->Entries[i].InternedName == name) {
			*id = scope->Entries[i].ID;
			return ResOk;
		}
	}

	if (!scope->Parent) {
		return ResNotFound;
	}

	return BindingLookup(scope->Parent, name, id);
}

static Result BindingInsert(BindingScope* scope, const char* name, usz* id)
{
	for (usz i = 0; i < scope->EntryCount; ++i) {
		if (scope->Entries[i].InternedName == name) {
			return ResErr;
		}
	}

	*id = GetSymbolID();
	scope->Entries[scope->EntryCount].ID = *id;
	scope->Entries[scope->EntryCount].InternedName = name;
	++scope->EntryCount;
	return ResOk;
}

static Result SymbolBind(ASTNode* node)
{
	if (!node) {
		return ResOk;
	}

	switch (node->Type) {
	case ASTNodeBlock: {
		Result result = BindingEnterScope();
		if (result) {
			return result;
		}

		printf("Entering scope\n");
		for (usz i = 0; i < node->Block.NodeCount; ++i) {
			result = SymbolBind(node->Block.Nodes[i]);
			if (result) {
				return result;
			}
		}
		printf("Exitting scope\n");

		return BindingExitScope();
	}
	case ASTNodeUnary:
		return SymbolBind(node->Unary.Operand);
	case ASTNodeGrouping:
		return SymbolBind(node->Grouping.Expression);
	case ASTNodeBinary: {
		Result result = SymbolBind(node->Binary.Left);
		if (result) {
			return result;
		}

		return SymbolBind(node->Binary.Right);
	}
	case ASTNodeIfStmt: {
		Result result = SymbolBind(node->IfStmt.Condition);
		if (result) {
			return result;
		}

		result = SymbolBind(node->IfStmt.ThenBlock);
		if (result) {
			return result;
		}

		return SymbolBind(node->IfStmt.ElseBlock);
	}
	case ASTNodeWhileStmt: {
		Result result = SymbolBind(node->WhileStmt.Condition);
		if (result) {
			return result;
		}

		return SymbolBind(node->WhileStmt.Body);
	}
	case ASTNodeVarDecl: {
		usz newID;
		Result result = BindingInsert(g_typeChecker.CurBindingScope, node->VarDecl.Identifier.Symbol, &newID);
		if (result) {
			DIAG_EMIT(DiagUnexpectedToken, node->Loc.Line, node->Loc.LinePos, DIAG_ARG_STRING("redeclaration in current scope"));
			return ResOk;
		}

		printf("New ID: %.*s -> %zu\n", (i32)node->VarDecl.Identifier.SymbolLength, node->VarDecl.Identifier.Symbol, newID);
		node->VarDecl.ID = newID;
		return ResOk;
	}
	case ASTNodeAssignment: {
		usz id;
		Result result = BindingLookup(g_typeChecker.CurBindingScope, node->Assignment.Identifier.Symbol, &id);
		if (result) {
			DIAG_EMIT(DiagUnexpectedToken, node->Loc.Line, node->Loc.LinePos, DIAG_ARG_STRING("undeclared variable used"));
			return ResOk;
		}

		printf("Bound ID: %.*s -> %zu\n", (i32)node->Assignment.Identifier.SymbolLength, node->Assignment.Identifier.Symbol, id);
		node->Assignment.ID = id;

		return SymbolBind(node->Assignment.Expression);
	}
	case ASTNodeFunctionCall: {
		for (usz i = 0; i < node->FunctionCall.ArgCount; ++i) {
			Result result = SymbolBind(node->FunctionCall.CallArgs[i]);
			if (result) {
				return result;
			}
		}

		return ResOk;
	}
	case ASTNodeIdentifier: {
		usz id;
		Result result = BindingLookup(g_typeChecker.CurBindingScope, node->Identifier.Identifier.Symbol, &id);
		if (result) {
			DIAG_EMIT(DiagUnexpectedToken, node->Loc.Line, node->Loc.LinePos, DIAG_ARG_STRING("undeclared variable used"));
			return ResOk;
		}

		printf("Bound ID: %.*s -> %zu\n", (i32)node->Identifier.Identifier.SymbolLength, node->Identifier.Identifier.Symbol, id);
		node->Identifier.ID = id;
		return ResOk;
	}
	default:
		return ResOk;
	}
}

MxShape* TypeCheck(ASTNode* node)
{
	if (!node) {
		return nullptr;
	}

	switch (node->Type) {
	case ASTNodeNumber: {
		MxShape* shape;
		Result result = StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape);
		if (result) {
			return nullptr;
		}

		shape->Height = 1;
		shape->Width = 1;
		shape->Type = MxStat;
		return shape;
	}
	case ASTNodeMxLiteral: {
		MxShape* shape;
		Result result = StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape);
		if (result) {
			return nullptr;
		}

		*shape = node->MxLiteral.Shape;
		return shape;
	}
	case ASTNodeUnary: {
		MxShape* shape;
		Result result = StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape);
		if (result) {
			return nullptr;
		}

		switch (node->Unary.Operator) {
		case TokenSubtract:
			*shape = node->Unary.Operand->MxLiteral.Shape;
			break;
		case TokenTranspose:
			shape->Height = node->Unary.Operand->MxLiteral.Shape.Width;
			shape->Width = node->Unary.Operand->MxLiteral.Shape.Height;
			shape->Type = MxStat;
			break;
		default:
			return nullptr;
		}
		return shape;
	}
	case ASTNodeGrouping:
		return TypeCheck(node->Grouping.Expression);
	case ASTNodeBinary: {
		MxShape* left = TypeCheck(node->Binary.Left);
		MxShape* right = TypeCheck(node->Binary.Right);

		switch (node->Binary.Operator) {
		case TokenAdd:
		case TokenSubtract:
			if ((left->Height == right->Height && left->Width == right->Width) || (right->Height == 1 && right->Width == 1)
				|| left->Type == MxDyn) {
				return left;
			} else if ((left->Height == 1 && left->Width == 1) || right->Type == MxDyn) {
				return right;
			} else {
				DIAG_EMIT(DiagUnexpectedToken, node->Loc.Line, node->Loc.LinePos, DIAG_ARG_STRING("incompatible matrix shapes add/sub"));
				return nullptr;
			}
		case TokenMultiply:
		case TokenDivide:
			if (left->Width == right->Height) {
				MxShape* shape;
				Result result = StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape);
				if (result) {
					return nullptr;
				}

				shape->Height = left->Height;
				shape->Width = right->Width;
				shape->Type = MxStat;
				return shape;
			} else if ((left->Height == 1 && left->Width == 1) || right->Type == MxDyn) {
				return right;
			} else if ((right->Height == 1 && right->Width == 1) || left->Type == MxDyn) {
				return left;
			} else {
				DIAG_EMIT(DiagUnexpectedToken, node->Loc.Line, node->Loc.LinePos, DIAG_ARG_STRING("incompatible matrix shapes mul/div"));
				return nullptr;
			}
		case TokenToPower:
			if (right->Type == MxDyn) {
				return right;
			} else if (right->Height != 1 || right->Width != 1) {
				DIAG_EMIT(DiagUnexpectedToken, node->Loc.Line, node->Loc.LinePos, DIAG_ARG_STRING("incompatible matrix shapes pow"));
				return nullptr;
			} else {
				return left;
			}
		case TokenGreater:
		case TokenGreaterEqual:
		case TokenLess:
		case TokenLessEqual:
		case TokenEqualEqual:
		case TokenNotEqual:
			if ((left->Height == right->Height && left->Width == right->Width) || left->Type == MxDyn) {
				return left;
			} else if (right->Type == MxDyn) {
				return right;
			} else {
				DIAG_EMIT(DiagUnexpectedToken, node->Loc.Line, node->Loc.LinePos, DIAG_ARG_STRING("incompatible matrix shapes comp/eq"));
				return nullptr;
			}
		case TokenOr:
		case TokenAnd: {
			MxShape* shape;
			Result result = StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape);
			if (result) {
				return nullptr;
			}

			shape->Height = 1;
			shape->Width = 1;
			shape->Type = MxStat;
			return shape;
		}
		default:
			return nullptr;
		}
	}
	case ASTNodeIfStmt: {
		TypeCheck(node->IfStmt.Condition);
		TypeCheck(node->IfStmt.ThenBlock);

		if (node->IfStmt.ElseBlock) {
			TypeCheck(node->IfStmt.ElseBlock);
		}

		return nullptr;
	}
	case ASTNodeWhileStmt: {
		TypeCheck(node->WhileStmt.Condition);
		TypeCheck(node->WhileStmt.Body);

		return nullptr;
	}
	case ASTNodeVarDecl: {
		usz id = node->VarDecl.ID;
		MxShape* initShape = TypeCheck(node->VarDecl.Expression);
		MxShape* varShape = &node->VarDecl.Shape;

		if (node->VarDecl.IsConst && !initShape) {
			DIAG_EMIT(DiagUnexpectedToken, node->VarDecl.Expression->Loc.Line, node->VarDecl.Expression->Loc.LinePos,
				DIAG_ARG_STRING("uninitialized const var"));
			return nullptr;
		}

		if (initShape && initShape->Type == MxStat) {
			if (varShape->Type == MxStat && (varShape->Height != initShape->Height || varShape->Width != initShape->Width)) {
				DIAG_EMIT(DiagUnexpectedToken, node->VarDecl.Expression->Loc.Line, node->VarDecl.Expression->Loc.LinePos,
					DIAG_ARG_STRING("uncompatible matrix shapes assign"));
				return nullptr;
			}

			g_typeChecker.TypeCheckingTable[id].Shape = *varShape;
			g_typeChecker.TypeCheckingTable[id].IsConst = node->VarDecl.IsConst;
			return nullptr;
		}

		if (initShape && initShape->Type == MxDyn) {
			if (varShape->Type == MxStat) {
				DIAG_EMIT(DiagUnexpectedToken, node->VarDecl.Expression->Loc.Line, node->VarDecl.Expression->Loc.LinePos,
					DIAG_ARG_STRING("assigning dyn mx to stat"));
				return nullptr;
			}

			g_typeChecker.TypeCheckingTable[id].Shape = *varShape;
			g_typeChecker.TypeCheckingTable[id].IsConst = node->VarDecl.IsConst;
			return nullptr;
		}

		if (!initShape && varShape->Type == MxDyn) {
			DIAG_EMIT(DiagUnexpectedToken, node->Loc.Line, node->Loc.LinePos, DIAG_ARG_STRING("uninitialized dyn var"));
			return nullptr;
		}

		g_typeChecker.TypeCheckingTable[id].Shape = *varShape;
		g_typeChecker.TypeCheckingTable[id].IsConst = node->VarDecl.IsConst;
		return nullptr;
	}
	case ASTNodeAssignment: {
		usz id = node->Assignment.ID;

		if (g_typeChecker.TypeCheckingTable[id].IsConst) {
			DIAG_EMIT(DiagUnexpectedToken, node->Loc.Line, node->Loc.LinePos, DIAG_ARG_STRING("assignment to const"));
			return nullptr;
		}

		MxShape* assignShape = TypeCheck(node->Assignment.Expression);
		MxShape* varShape = &g_typeChecker.TypeCheckingTable[id].Shape;

		if (!assignShape) {
			DIAG_EMIT(DiagUnexpectedToken, node->Loc.Line, node->Loc.LinePos, DIAG_ARG_STRING("expression does not return value"));
			return nullptr;
		}

		if (!node->Assignment.Index) {
			if (varShape->Type == MxStat && assignShape->Type == MxDyn) {
				DIAG_EMIT(DiagUnexpectedToken, node->VarDecl.Expression->Loc.Line, node->VarDecl.Expression->Loc.LinePos,
					DIAG_ARG_STRING("assigning dyn mx to stat"));
				return nullptr;
			}

			if (varShape->Type == MxStat && assignShape->Type == MxStat
				&& (varShape->Height != assignShape->Height || varShape->Width != assignShape->Width)) {
				DIAG_EMIT(DiagUnexpectedToken, node->Assignment.Expression->Loc.Line, node->Assignment.Expression->Loc.LinePos,
					DIAG_ARG_STRING("uncompatible matrix shapes assign"));
				return nullptr;
			}
		} else {
			if (varShape->Type == MxStat && assignShape->Type == MxStat) {
				if (node->Assignment.Index->IndexSuffix.J && (assignShape->Height != 1 || assignShape->Width != 1)) {
					DIAG_EMIT(DiagUnexpectedToken, node->Assignment.Expression->Loc.Line, node->Assignment.Expression->Loc.LinePos,
						DIAG_ARG_STRING("uncompatible matrix shapes assign"));
					return nullptr;
				}

				if (!node->Assignment.Index->IndexSuffix.J) {
					if (assignShape->Height != 1 || assignShape->Width != varShape->Width) {
						DIAG_EMIT(DiagUnexpectedToken, node->Assignment.Expression->Loc.Line, node->Assignment.Expression->Loc.LinePos,
							DIAG_ARG_STRING("uncompatible matrix shapes assign"));
						return nullptr;
					}
				}
			} else if (varShape->Type == MxStat && assignShape->Type == MxDyn) {
				DIAG_EMIT(DiagUnexpectedToken, node->VarDecl.Expression->Loc.Line, node->VarDecl.Expression->Loc.LinePos,
					DIAG_ARG_STRING("assigning dyn mx to stat"));
				return nullptr;
			}
		}

		return nullptr;
	}
	case ASTNodeFunctionCall: {
		for (usz i = 0; i < node->FunctionCall.ArgCount; ++i) {
			TypeCheck(node->FunctionCall.CallArgs[i]);
		}

		return nullptr;
	}
	case ASTNodeIdentifier: {
		MxShape* shape;
		Result result = StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape);
		if (result) {
			return nullptr;
		}

		usz id = node->Identifier.ID;
		MxShape* varShape = &g_typeChecker.TypeCheckingTable[id].Shape;

		if (varShape->Type == MxDyn) {
			shape->Type = MxDyn;
			return shape;
		}

		if (node->Identifier.Index) {
			if (node->Identifier.Index->IndexSuffix.J) {
				shape->Height = 1;
				shape->Width = 1;
				shape->Type = MxStat;
				return shape;
			}

			shape->Height = 1;
			shape->Width = varShape->Width;
			shape->Type = MxStat;
			return shape;
		}

		shape->Height = varShape->Height;
		shape->Width = varShape->Width;
		shape->Type = MxStat;
		return shape;
	}
	case ASTNodeBlock: {
		for (usz i = 0; i < node->Block.NodeCount; ++i) {
			MxShape* shape = TypeCheck(node->Block.Nodes[i]);
			if (shape) {
				DIAG_EMIT0(DiagUnusedExpressionResult, node->Block.Nodes[i]->Loc.Line, node->Block.Nodes[i]->Loc.LinePos);
				return nullptr;
			}
		}

		return nullptr;
	}
	default:
		return nullptr;
	}
}

Result TypeCheckerInit()
{
	Result result = DynArenaInit(&g_typeChecker.BindingArena);
	if (result) {
		return result;
	}

	g_typeChecker.TypeCheckingTable = calloc(128, sizeof(MxShape));
	if (!g_typeChecker.TypeCheckingTable) {
		return ResErr;
	}

	result = StatArenaInit(&g_typeChecker.ShapeArena, sizeof(MxShape));
	if (result) {
		return result;
	}

	g_typeChecker.CurBindingScope = nullptr;
	return ResOk;
}

Result TypeCheckerSymbolBind() { return SymbolBind((ASTNode*)g_parser.ASTArena.Blocks->Data); }

void TypeCheckerTypeCheck() { TypeCheck((ASTNode*)g_parser.ASTArena.Blocks->Data); }

Result TypeCheckerDeinit()
{
	free(g_typeChecker.TypeCheckingTable);

	Result result = StatArenaDeinit(&g_typeChecker.ShapeArena);
	if (result) {
		return result;
	}

	return DynArenaDeinit(&g_typeChecker.BindingArena);
}
