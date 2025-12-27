#include "TypeChecker.h"

#include "Diagnostics.h"
#include "Parser.h"
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
	case ASTNodeMxLiteral: {
		for (usz i = 0; i < node->MxLiteral.Shape.Height * node->MxLiteral.Shape.Width; ++i) {
			Result result = SymbolBind(node->MxLiteral.Matrix[i]);
			if (result) {
				return result;
			}
		}

		return ResOk;
	}
	case ASTNodeBlock: {
		Result result = BindingEnterScope();
		if (result) {
			return result;
		}

		for (usz i = 0; i < node->Block.NodeCount; ++i) {
			result = SymbolBind(node->Block.Nodes[i]);
			if (result) {
				return result;
			}
		}

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
			DIAG_EMIT0(DiagRedeclarationInScope, node->Loc);
			return ResOk;
		}

		node->VarDecl.ID = newID;

		if (node->VarDecl.Expression) {
			result = SymbolBind(node->VarDecl.Expression);
			if (result) {
				return result;
			}
		}

		return ResOk;
	}
	case ASTNodeAssignment: {
		usz id;
		Result result = BindingLookup(g_typeChecker.CurBindingScope, node->Assignment.Identifier.Symbol, &id);
		if (result) {
			DIAG_EMIT(DiagUndeclaredVarUsed, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->Assignment.Identifier));
			return ResOk;
		}

		node->Assignment.ID = id;

		if (node->Assignment.Index) {
			result = SymbolBind(node->Assignment.Index);
			if (result) {
				return result;
			}
		}

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
			DIAG_EMIT(DiagUndeclaredVarUsed, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->Identifier.Identifier));
			return ResOk;
		}

		node->Identifier.ID = id;

		if (node->Identifier.Index) {
			result = SymbolBind(node->Identifier.Index);
			if (result) {
				return result;
			}
		}

		return ResOk;
	}
	case ASTNodeIndexSuffix: {
		Result result = SymbolBind(node->IndexSuffix.I);
		if (result) {
			return result;
		}

		if (node->IndexSuffix.J) {
			result = SymbolBind(node->IndexSuffix.J);
			if (result) {
				return result;
			}
		}

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
		return shape;
	}
	case ASTNodeMxLiteral: {
		MxShape* shape;
		Result result = StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape);
		if (result) {
			return nullptr;
		}

		*shape = node->MxLiteral.Shape;

		for (usz i = 0; i < node->MxLiteral.Shape.Height * node->MxLiteral.Shape.Width; ++i) {
			MxShape* mx = TypeCheck(node->MxLiteral.Matrix[i]);

			if (!mx) {
				DIAG_EMIT0(DiagExprDoesNotReturnValue, node->MxLiteral.Matrix[i]->Loc);
				return nullptr;
			}

			if (mx->Height != 1 || mx->Width != 1) {
				DIAG_EMIT0(DiagMxLiteralOnly1x1, node->MxLiteral.Matrix[i]->Loc);
				return nullptr;
			}
		}

		return shape;
	}
	case ASTNodeUnary: {
		MxShape* shape;
		Result result = StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape);
		if (result) {
			return nullptr;
		}

		MxShape* operand = TypeCheck(node->Unary.Operand);
		if (!operand) {
			DIAG_EMIT0(DiagExprDoesNotReturnValue, node->Unary.Operand->Loc);
			return nullptr;
		}

		switch (node->Unary.Operator) {
		case TokenSubtract:
			*shape = *operand;
			break;
		case TokenTranspose:
			shape->Height = operand->Width;
			shape->Width = operand->Height;
			break;
		default:
			return nullptr;
		}
		return shape;
	}
	case ASTNodeGrouping:
		return TypeCheck(node->Grouping.Expression);
	case ASTNodeBinary: {
		MxShape* shape;
		Result result = StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape);
		if (result) {
			return nullptr;
		}

		MxShape* left = TypeCheck(node->Binary.Left);
		MxShape* right = TypeCheck(node->Binary.Right);

		switch (node->Binary.Operator) {
		case TokenAdd:
		case TokenSubtract:
			if (left->Height == right->Height && left->Width == right->Width) {
				*shape = *left;
				break;
			} else if (left->Height == 1 && left->Width == 1) {
				*shape = *right;
				break;
			} else if (right->Height == 1 && right->Width == 1) {
				*shape = *left;
				break;
			} else {
				DIAG_EMIT(DiagMxLiteralShapesDifferAddSub, node->Loc, DIAG_ARG_MX_SHAPE(*left), DIAG_ARG_MX_SHAPE(*right));
				return nullptr;
			}
		case TokenMultiply:
			if (left->Width == right->Height) {
				shape->Height = left->Height;
				shape->Width = right->Width;
				break;
			} else if (left->Height == 1 && left->Width == 1) {
				*shape = *right;
				break;
			} else if (right->Height == 1 && right->Width == 1) {
				*shape = *left;
				break;
			} else {
				DIAG_EMIT(DiagMxLiteralShapesDifferMul, node->Loc, DIAG_ARG_MX_SHAPE(*left), DIAG_ARG_MX_SHAPE(*right));
				return nullptr;
			}
		case TokenDivide:
			if (left->Height == 1 && left->Width == 1) {
				*shape = *right;
				break;
			} else if (right->Height == 1 && right->Width == 1) {
				*shape = *left;
				break;
			} else {
				DIAG_EMIT(DiagMxLiteralShapesDifferDiv, node->Loc, DIAG_ARG_MX_SHAPE(*left), DIAG_ARG_MX_SHAPE(*right));
				return nullptr;
			}
		case TokenToPower:
			if (right->Height != 1 || right->Width != 1) {
				DIAG_EMIT0(DiagMxLiteralInvalidPower, node->Binary.Right->Loc);
				return nullptr;
			}

			*shape = *left;
			break;
		case TokenGreater:
		case TokenGreaterEqual:
		case TokenLess:
		case TokenLessEqual:
		case TokenEqualEqual:
		case TokenNotEqual:
			if (left->Height != right->Height || left->Width != right->Width) {
				DIAG_EMIT(DiagMxLiteralShapesDifferComp, node->Loc, DIAG_ARG_MX_SHAPE(*left), DIAG_ARG_MX_SHAPE(*right));
				return nullptr;
			}

			shape->Height = 1;
			shape->Width = 1;
			break;
		case TokenOr:
		case TokenAnd:
			shape->Height = 1;
			shape->Width = 1;
			break;
		default:
			return nullptr;
		}

		return shape;
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

		if (node->VarDecl.Expression && !initShape) {
			DIAG_EMIT0(DiagExprDoesNotReturnValue, node->VarDecl.Expression->Loc);
			return nullptr;
		}

		MxShape* varShape;
		if (!node->VarDecl.HasDeclaredShape) {
			if (!initShape) {
				DIAG_EMIT(DiagUninitializedUntypedVar, node->VarDecl.Expression->Loc, DIAG_ARG_SYMBOL_VIEW(node->VarDecl.Identifier));
				return nullptr;
			}

			varShape = initShape;
		} else {
			varShape = &node->VarDecl.Shape;
		}

		g_typeChecker.TypeCheckingTable[id].Shape = *varShape;
		g_typeChecker.TypeCheckingTable[id].IsConst = node->VarDecl.IsConst;
		node->VarDecl.Shape = *varShape;

		if (node->VarDecl.IsConst && !initShape) {
			DIAG_EMIT(DiagUninitializedConstVar, node->VarDecl.Expression->Loc, DIAG_ARG_SYMBOL_VIEW(node->VarDecl.Identifier));
			return nullptr;
		}

		if (initShape) {
			if (varShape->Height != initShape->Height || varShape->Width != initShape->Width) {
				DIAG_EMIT(DiagMxLiteralShapesDifferAssign, node->VarDecl.Expression->Loc, DIAG_ARG_MX_SHAPE(*initShape),
					DIAG_ARG_MX_SHAPE(*varShape));
				return nullptr;
			}

			return nullptr;
		}

		return nullptr;
	}
	case ASTNodeAssignment: {
		usz id = node->Assignment.ID;

		if (g_typeChecker.TypeCheckingTable[id].IsConst) {
			DIAG_EMIT0(DiagAssignToConstVar, node->Loc);
			return nullptr;
		}

		MxShape* assignShape = TypeCheck(node->Assignment.Expression);
		MxShape* varShape = &g_typeChecker.TypeCheckingTable[id].Shape;

		if (!assignShape) {
			DIAG_EMIT0(DiagExprDoesNotReturnValue, node->Loc);
			return nullptr;
		}

		if (!node->Assignment.Index) {
			if (varShape->Height != assignShape->Height || varShape->Width != assignShape->Width) {
				DIAG_EMIT(DiagMxLiteralShapesDifferAssign, node->Assignment.Expression->Loc, DIAG_ARG_MX_SHAPE(*assignShape),
					DIAG_ARG_MX_SHAPE(*varShape));
				return nullptr;
			}
		} else {
			MxShape* mxI = TypeCheck(node->Assignment.Index->IndexSuffix.I);
			if (!mxI) {
				DIAG_EMIT0(DiagExprDoesNotReturnValue, node->Loc);
				return nullptr;
			}

			if (mxI->Height != 1 || mxI->Width != 1) {
				DIAG_EMIT0(DiagMxLiteralOnly1x1, node->Assignment.Index->IndexSuffix.I->Loc);
				return nullptr;
			}

			if (node->Assignment.Index->IndexSuffix.J) {
				MxShape* mxJ = TypeCheck(node->Assignment.Index->IndexSuffix.J);
				if (!mxJ) {
					DIAG_EMIT0(DiagExprDoesNotReturnValue, node->Loc);
					return nullptr;
				}

				if (mxJ->Height != 1 || mxJ->Width != 1) {
					DIAG_EMIT0(DiagMxLiteralOnly1x1, node->Assignment.Index->IndexSuffix.I->Loc);
					return nullptr;
				}

				if (assignShape->Height != 1 || assignShape->Width != 1) {
					MxShape correctShape = { .Height = 1, .Width = 1 };
					DIAG_EMIT(DiagMxLiteralShapesDifferAssign, node->Assignment.Expression->Loc, DIAG_ARG_MX_SHAPE(*assignShape),
						DIAG_ARG_MX_SHAPE(correctShape));
					return nullptr;
				}
			} else {
				if (assignShape->Height != 1 || assignShape->Width != varShape->Width) {
					MxShape correctShape = { .Height = 1, .Width = varShape->Width };
					DIAG_EMIT(DiagMxLiteralShapesDifferAssign, node->Assignment.Expression->Loc, DIAG_ARG_MX_SHAPE(*assignShape),
						DIAG_ARG_MX_SHAPE(correctShape));
					return nullptr;
				}
			}

			return nullptr;
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

		if (node->Identifier.Index) {
			if (node->Identifier.Index->IndexSuffix.J) {
				shape->Height = 1;
				shape->Width = 1;
				return shape;
			}

			shape->Height = 1;
			shape->Width = varShape->Width;
			return shape;
		}

		shape->Height = varShape->Height;
		shape->Width = varShape->Width;
		return shape;
	}
	case ASTNodeBlock: {
		for (usz i = 0; i < node->Block.NodeCount; ++i) {
			MxShape* shape = TypeCheck(node->Block.Nodes[i]);
			if (shape) {
				DIAG_EMIT0(DiagUnusedExpressionResult, node->Block.Nodes[i]->Loc);
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
