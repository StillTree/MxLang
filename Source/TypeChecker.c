#include "TypeChecker.h"

#include "Diagnostics.h"
#include "Mx.h"
#include "Parser.h"
#include <stdlib.h>
#include <string.h>

TypeChecker g_typeChecker = { 0 };

static usz GetSymbolID()
{
	static usz id = 0;
	return id++;
}

static void BindingEnterScope()
{
	BindingScope* temp = g_typeChecker.CurBindingScope;

	DIAG_PANIC_ON_ERR(DynArenaAllocZeroed(
		&g_typeChecker.BindingArena, (void**)&g_typeChecker.CurBindingScope, sizeof(BindingScope) + (64 * sizeof(BindingEntry))));

	g_typeChecker.CurBindingScope->Parent = temp;
}

static Result BindingExitScope()
{
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

static void SymbolBind(ASTNode* node)
{
	if (!node) {
		return;
	}

	switch (node->Type) {
	case ASTNodeMxLiteral: {
		for (usz i = 0; i < node->MxLiteral.Shape.Height * node->MxLiteral.Shape.Width; ++i) {
			SymbolBind(node->MxLiteral.Matrix[i]);
		}

		break;
	}
	case ASTNodeBlock: {
		BindingEnterScope();

		for (usz i = 0; i < node->Block.NodeCount; ++i) {
			SymbolBind(node->Block.Nodes[i]);
		}

		DIAG_PANIC_ON_ERR(BindingExitScope());
		break;
	}
	case ASTNodeUnary:
		SymbolBind(node->Unary.Operand);
		break;
	case ASTNodeGrouping:
		SymbolBind(node->Grouping.Expression);
		break;
	case ASTNodeBinary: {
		SymbolBind(node->Binary.Left);
		SymbolBind(node->Binary.Right);
		break;
	}
	case ASTNodeIfStmt: {
		SymbolBind(node->IfStmt.Condition);
		SymbolBind(node->IfStmt.ThenBlock);
		SymbolBind(node->IfStmt.ElseBlock);
		break;
	}
	case ASTNodeWhileStmt: {
		SymbolBind(node->WhileStmt.Condition);
		SymbolBind(node->WhileStmt.Body);
		break;
	}
	case ASTNodeVarDecl: {
		usz newID;
		Result result = BindingInsert(g_typeChecker.CurBindingScope, node->VarDecl.Identifier.Symbol, &newID);
		if (result) {
			DIAG_EMIT0(DiagRedeclarationInScope, node->Loc);
			return;
		}

		node->VarDecl.ID = newID;

		SymbolBind(node->VarDecl.Expression);

		break;
	}
	case ASTNodeAssignment: {
		usz id;
		Result result = BindingLookup(g_typeChecker.CurBindingScope, node->Assignment.Identifier.Symbol, &id);
		if (result) {
			DIAG_EMIT(DiagUndeclaredVarUsed, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->Assignment.Identifier));
			return;
		}

		node->Assignment.ID = id;

		SymbolBind(node->Assignment.Index);

		SymbolBind(node->Assignment.Expression);

		break;
	}
	case ASTNodeFunctionCall: {
		for (usz i = 0; i < node->FunctionCall.ArgCount; ++i) {
			SymbolBind(node->FunctionCall.CallArgs[i]);
		}

		break;
	}
	case ASTNodeIdentifier: {
		usz id;
		Result result = BindingLookup(g_typeChecker.CurBindingScope, node->Identifier.Identifier.Symbol, &id);
		if (result) {
			DIAG_EMIT(DiagUndeclaredVarUsed, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->Identifier.Identifier));
			return;
		}

		node->Identifier.ID = id;

		SymbolBind(node->Identifier.Index);

		break;
	}
	case ASTNodeIndexSuffix: {
		SymbolBind(node->IndexSuffix.I);
		SymbolBind(node->IndexSuffix.J);
		break;
	}
	default:
		break;
	}
}

Result TypeCheckCompTimeInteger(ASTNode* node, usz* num)
{
	if (node->Type != ASTNodeMxLiteral || node->MxLiteral.Shape.Height != 1 || node->MxLiteral.Shape.Width != 1
		|| node->MxLiteral.Matrix[0]->Type != ASTNodeNumber) {
		DIAG_EMIT0(DiagFunctionCallArgMustBeCompTime, node->Loc);
		return ResErr;
	}

	if (!IsF64Int(node->MxLiteral.Matrix[0]->Number)) {
		DIAG_EMIT(DiagNotInteger, node->FunctionCall.CallArgs[0]->Loc, DIAG_ARG_NUMBER(node->MxLiteral.Matrix[0]->Number));
		return ResErr;
	}

	*num = (usz)node->MxLiteral.Matrix[0]->Number;
	return ResOk;
}

MxShape* TypeCheck(ASTNode* node)
{
	if (!node) {
		return nullptr;
	}

	switch (node->Type) {
	case ASTNodeNumber: {
		MxShape* shape;
		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape));

		shape->Height = 1;
		shape->Width = 1;
		return shape;
	}
	case ASTNodeMxLiteral: {
		MxShape* shape;
		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape));

		*shape = node->MxLiteral.Shape;

		for (usz i = 0; i < node->MxLiteral.Shape.Height * node->MxLiteral.Shape.Width; ++i) {
			MxShape* mx = TypeCheck(node->MxLiteral.Matrix[i]);

			if (!mx) {
				if (node->MxLiteral.Matrix[i]) {
					DIAG_EMIT0(DiagExprDoesNotReturnValue, node->MxLiteral.Matrix[i]->Loc);
				}

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
		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape));

		MxShape* operand = TypeCheck(node->Unary.Operand);
		if (!operand) {
			if (node->Unary.Operand) {
				DIAG_EMIT0(DiagExprDoesNotReturnValue, node->Unary.Operand->Loc);
			}

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
		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape));

		MxShape* left = TypeCheck(node->Binary.Left);
		if (!left) {
			if (node->Binary.Left) {
				DIAG_EMIT0(DiagExprDoesNotReturnValue, node->Binary.Left->Loc);
			}

			return nullptr;
		}

		MxShape* right = TypeCheck(node->Binary.Right);
		if (!right) {
			if (node->Binary.Right) {
				DIAG_EMIT0(DiagExprDoesNotReturnValue, node->Binary.Right->Loc);
			}

			return nullptr;
		}

		switch (node->Binary.Operator) {
		case TokenAdd:
		case TokenSubtract:
			if ((left->Height == right->Height && left->Width == right->Width) || (right->Height == 1 && right->Width == 1)) {
				*shape = *left;
				break;
			} else if (left->Height == 1 && left->Width == 1) {
				*shape = *right;
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
			} else if (left->Height != left->Width) {
				DIAG_EMIT0(DiagMxLiteralInvalidPowerBase, node->Binary.Left->Loc);
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
		MxShape* condShape = TypeCheck(node->IfStmt.Condition);
		if (!condShape) {
			if (node->IfStmt.Condition) {
				DIAG_EMIT0(DiagExprDoesNotReturnValue, node->IfStmt.Condition->Loc);
			}

			return nullptr;
		}

		TypeCheck(node->IfStmt.ThenBlock);

		if (node->IfStmt.ElseBlock) {
			TypeCheck(node->IfStmt.ElseBlock);
		}

		return nullptr;
	}
	case ASTNodeWhileStmt: {
		MxShape* condShape = TypeCheck(node->WhileStmt.Condition);
		if (!condShape) {
			if (node->WhileStmt.Condition) {
				DIAG_EMIT0(DiagExprDoesNotReturnValue, node->WhileStmt.Condition->Loc);
			}

			return nullptr;
		}

		TypeCheck(node->WhileStmt.Body);

		return nullptr;
	}
	case ASTNodeVarDecl: {
		usz id = node->VarDecl.ID;
		MxShape* initShape = TypeCheck(node->VarDecl.Expression);

		if (!initShape) {
			if (node->VarDecl.Expression) {
				DIAG_EMIT0(DiagExprDoesNotReturnValue, node->VarDecl.Expression->Loc);
			}

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
			if (node->Assignment.Expression) {
				DIAG_EMIT0(DiagExprDoesNotReturnValue, node->Assignment.Expression->Loc);
			}

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
				if (node->Assignment.Index->IndexSuffix.I) {
					DIAG_EMIT0(DiagExprDoesNotReturnValue, node->Assignment.Index->IndexSuffix.I->Loc);
				}

				return nullptr;
			}

			if (mxI->Height != 1 || mxI->Width != 1) {
				DIAG_EMIT0(DiagMxLiteralOnly1x1, node->Assignment.Index->IndexSuffix.I->Loc);
				return nullptr;
			}

			if (node->Assignment.Index->IndexSuffix.J) {
				MxShape* mxJ = TypeCheck(node->Assignment.Index->IndexSuffix.J);
				if (!mxJ) {
					if (node->Assignment.Index->IndexSuffix.J) {
						DIAG_EMIT0(DiagExprDoesNotReturnValue, node->Assignment.Index->IndexSuffix.J->Loc);
					}

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
		if (node->FunctionCall.Identifier.SymbolLength == 7 && memcmp(node->FunctionCall.Identifier.Symbol, "display", 7) == 0) {
			for (usz i = 0; i < node->FunctionCall.ArgCount; ++i) {
				MxShape* argShape = TypeCheck(node->FunctionCall.CallArgs[i]);
				if (!argShape) {
					if (node->FunctionCall.CallArgs[i]) {
						DIAG_EMIT0(DiagExprDoesNotReturnValue, node->FunctionCall.CallArgs[i]->Loc);
					}

					return nullptr;
				}
			}

			return nullptr;
		}

		if (node->FunctionCall.Identifier.SymbolLength == 5 && memcmp(node->FunctionCall.Identifier.Symbol, "ident", 5) == 0) {
			if (node->FunctionCall.ArgCount < 1) {
				DIAG_EMIT(DiagTooLittleFunctionCallArgs, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->FunctionCall.Identifier));
				return nullptr;
			}

			if (node->FunctionCall.ArgCount > 1) {
				DIAG_EMIT(DiagTooManyFunctionCallArgs, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->FunctionCall.Identifier));
				return nullptr;
			}

			usz size;
			Result result = TypeCheckCompTimeInteger(node->FunctionCall.CallArgs[0], &size);
			if (result) {
				return nullptr;
			}

			MxShape* shape;
			DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape));

			shape->Height = size;
			shape->Width = size;
			return shape;
		}

		if (node->FunctionCall.Identifier.SymbolLength == 4 && memcmp(node->FunctionCall.Identifier.Symbol, "fill", 4) == 0) {
			if (node->FunctionCall.ArgCount < 3) {
				DIAG_EMIT(DiagTooLittleFunctionCallArgs, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->FunctionCall.Identifier));
				return nullptr;
			}

			if (node->FunctionCall.ArgCount > 3) {
				DIAG_EMIT(DiagTooManyFunctionCallArgs, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->FunctionCall.Identifier));
				return nullptr;
			}

			usz height;
			Result result = TypeCheckCompTimeInteger(node->FunctionCall.CallArgs[0], &height);
			if (result) {
				return nullptr;
			}

			usz width;
			result = TypeCheckCompTimeInteger(node->FunctionCall.CallArgs[1], &width);
			if (result) {
				return nullptr;
			}

			MxShape* fillShape = TypeCheck(node->FunctionCall.CallArgs[2]);
			if (fillShape->Height != 1 || fillShape->Width != 1) {
				DIAG_EMIT0(DiagMxLiteralOnly1x1, node->FunctionCall.CallArgs[2]->Loc);
				return nullptr;
			}

			MxShape* shape;
			DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape));

			shape->Height = height;
			shape->Width = width;
			return shape;
		}

		if (node->FunctionCall.Identifier.SymbolLength == 4 && memcmp(node->FunctionCall.Identifier.Symbol, "rand", 4) == 0) {
			if (node->FunctionCall.ArgCount < 2) {
				DIAG_EMIT(DiagTooLittleFunctionCallArgs, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->FunctionCall.Identifier));
				return nullptr;
			}

			if (node->FunctionCall.ArgCount > 2) {
				DIAG_EMIT(DiagTooManyFunctionCallArgs, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->FunctionCall.Identifier));
				return nullptr;
			}

			usz height;
			Result result = TypeCheckCompTimeInteger(node->FunctionCall.CallArgs[0], &height);
			if (result) {
				return nullptr;
			}

			usz width;
			result = TypeCheckCompTimeInteger(node->FunctionCall.CallArgs[1], &width);
			if (result) {
				return nullptr;
			}

			MxShape* shape;
			DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape));

			shape->Height = height;
			shape->Width = width;
			return shape;
		}

		if (node->FunctionCall.Identifier.SymbolLength == 5 && memcmp(node->FunctionCall.Identifier.Symbol, "input", 5) == 0) {
			if (node->FunctionCall.ArgCount > 0) {
				DIAG_EMIT(DiagTooManyFunctionCallArgs, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->FunctionCall.Identifier));
				return nullptr;
			}

			MxShape* shape;
			DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape));

			shape->Height = 1;
			shape->Width = 1;
			return shape;
		}

		if ((node->FunctionCall.Identifier.SymbolLength == 3 && memcmp(node->FunctionCall.Identifier.Symbol, "sin", 3) == 0)
			|| (node->FunctionCall.Identifier.SymbolLength == 3 && memcmp(node->FunctionCall.Identifier.Symbol, "cos", 3) == 0)
			|| (node->FunctionCall.Identifier.SymbolLength == 3 && memcmp(node->FunctionCall.Identifier.Symbol, "tan", 3) == 0)
			|| (node->FunctionCall.Identifier.SymbolLength == 3 && memcmp(node->FunctionCall.Identifier.Symbol, "cot", 3) == 0)
			|| (node->FunctionCall.Identifier.SymbolLength == 2 && memcmp(node->FunctionCall.Identifier.Symbol, "ln", 2) == 0)
			|| (node->FunctionCall.Identifier.SymbolLength == 4 && memcmp(node->FunctionCall.Identifier.Symbol, "sqrt", 4) == 0)
			|| (node->FunctionCall.Identifier.SymbolLength == 3 && memcmp(node->FunctionCall.Identifier.Symbol, "abs", 3) == 0)
			|| (node->FunctionCall.Identifier.SymbolLength == 5 && memcmp(node->FunctionCall.Identifier.Symbol, "floor", 5) == 0)
			|| (node->FunctionCall.Identifier.SymbolLength == 4 && memcmp(node->FunctionCall.Identifier.Symbol, "ceil", 4) == 0)) {
			if (node->FunctionCall.ArgCount < 1) {
				DIAG_EMIT(DiagTooLittleFunctionCallArgs, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->FunctionCall.Identifier));
				return nullptr;
			}

			if (node->FunctionCall.ArgCount > 1) {
				DIAG_EMIT(DiagTooManyFunctionCallArgs, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->FunctionCall.Identifier));
				return nullptr;
			}

			MxShape* argShape = TypeCheck(node->FunctionCall.CallArgs[0]);

			MxShape* shape;
			DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape));

			shape->Height = argShape->Height;
			shape->Width = argShape->Width;
			return shape;
		}

		if (node->FunctionCall.Identifier.SymbolLength == 3 && memcmp(node->FunctionCall.Identifier.Symbol, "log", 3) == 0) {
			if (node->FunctionCall.ArgCount < 2) {
				DIAG_EMIT(DiagTooLittleFunctionCallArgs, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->FunctionCall.Identifier));
				return nullptr;
			}

			if (node->FunctionCall.ArgCount > 2) {
				DIAG_EMIT(DiagTooManyFunctionCallArgs, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->FunctionCall.Identifier));
				return nullptr;
			}

			MxShape* baseShape = TypeCheck(node->FunctionCall.CallArgs[0]);
			if (baseShape->Height != 1 || baseShape->Width != 1) {
				DIAG_EMIT0(DiagMxLiteralOnly1x1, node->FunctionCall.CallArgs[0]->Loc);
				return nullptr;
			}

			MxShape* argShape = TypeCheck(node->FunctionCall.CallArgs[1]);

			MxShape* shape;
			DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape));

			shape->Height = argShape->Height;
			shape->Width = argShape->Width;
			return shape;
		}

		DIAG_EMIT(DiagUndeclaredFunction, node->Loc, DIAG_ARG_SYMBOL_VIEW(node->FunctionCall.Identifier));
		return nullptr;
	}
	case ASTNodeIdentifier: {
		MxShape* shape;
		DIAG_PANIC_ON_ERR(StatArenaAlloc(&g_typeChecker.ShapeArena, (void**)&shape));

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

void TypeCheckerInit()
{
	DIAG_PANIC_ON_ERR(DynArenaInit(&g_typeChecker.BindingArena));

	g_typeChecker.TypeCheckingTable = calloc(128, sizeof(MxShape));
	if (!g_typeChecker.TypeCheckingTable) {
		DIAG_PANIC_ON_ERR(ResErr);
	}

	DIAG_PANIC_ON_ERR(StatArenaInit(&g_typeChecker.ShapeArena, sizeof(MxShape)));

	g_typeChecker.CurBindingScope = nullptr;
}

void TypeCheckerSymbolBind() { SymbolBind((ASTNode*)g_parser.ASTArena.Blocks->Data); }

void TypeCheckerTypeCheck() { TypeCheck((ASTNode*)g_parser.ASTArena.Blocks->Data); }

void TypeCheckerDeinit()
{
	free(g_typeChecker.TypeCheckingTable);

	DIAG_PANIC_ON_ERR(StatArenaDeinit(&g_typeChecker.ShapeArena));

	DIAG_PANIC_ON_ERR(DynArenaDeinit(&g_typeChecker.BindingArena));
}
