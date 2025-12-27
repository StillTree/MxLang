#include "Interpreter.h"

#include "Diagnostics.h"
#include "Functions.h"
#include "Mx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Interpreter g_interpreter = { 0 };

Result InterpreterInit()
{
	g_interpreter.VarTable = (Mx**)calloc(128, sizeof(Mx*));
	if (!g_interpreter.VarTable) {
		return ResErr;
	}

	return DynArenaInit(&g_interpreter.MxArena);
}

Mx* InterpreterAllocMx(usz height, usz width)
{
	Mx* mx;
	Result result = DynArenaAlloc(&g_interpreter.MxArena, (void**)&mx, sizeof(Mx) + (height * width * sizeof(double)));
	if (result) {
		// TODO: Panic!
	}

	mx->Shape.Height = height;
	mx->Shape.Width = width;

	return mx;
}

Mx* InterpreterEval(ASTNode* node)
{
	switch (node->Type) {
	case ASTNodeNumber: {
		Mx* num = InterpreterAllocMx(1, 1);
		num->Data[0] = node->Number;
		return num;
	}
	case ASTNodeMxLiteral: {
		Mx* mx = InterpreterAllocMx(node->MxLiteral.Shape.Height, node->MxLiteral.Shape.Width);

		for (usz i = 0; i < node->MxLiteral.Shape.Height * node->MxLiteral.Shape.Width; ++i) {
			Mx* num = InterpreterEval(node->MxLiteral.Matrix[i]);

			mx->Data[i] = num->Data[0];
		}

		return mx;
	}
	case ASTNodeUnary: {
		Mx* operand = InterpreterEval(node->Unary.Operand);

		switch (node->Unary.Operator) {
		case TokenSubtract: {
			Result result = MxNegate(operand, operand);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}

			return operand;
		}
		case TokenTranspose: {
			Mx* mx = InterpreterAllocMx(operand->Shape.Width, operand->Shape.Height);
			Result result = MxTranspose(operand, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}

			return mx;
		}
		default:
			// TODO: Panic!
			return nullptr;
		}
	}
	case ASTNodeGrouping:
		return InterpreterEval(node->Grouping.Expression);
	case ASTNodeBinary: {
		Mx* left = InterpreterEval(node->Binary.Left);
		Mx* right = InterpreterEval(node->Binary.Right);

		switch (node->Binary.Operator) {
		case TokenAdd: {
			Mx* mx = nullptr;

			if (left->Shape.Height == 1 && left->Shape.Width == 1) {
				mx = InterpreterAllocMx(right->Shape.Height, right->Shape.Width);
			} else if (right->Shape.Height == 1 && right->Shape.Width == 1) {
				mx = InterpreterAllocMx(left->Shape.Height, left->Shape.Width);
			} else {
				mx = InterpreterAllocMx(left->Shape.Height, left->Shape.Width);
			}

			Result result = MxAdd(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenSubtract: {
			Mx* mx = nullptr;

			if (left->Shape.Height == 1 && left->Shape.Width == 1) {
				mx = InterpreterAllocMx(right->Shape.Height, right->Shape.Width);
			} else if (right->Shape.Height == 1 && right->Shape.Width == 1) {
				mx = InterpreterAllocMx(left->Shape.Height, left->Shape.Width);
			} else {
				mx = InterpreterAllocMx(left->Shape.Height, left->Shape.Width);
			}

			Result result = MxSubtract(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenMultiply: {
			Mx* mx = nullptr;

			if (left->Shape.Height == 1 && left->Shape.Width == 1) {
				mx = InterpreterAllocMx(right->Shape.Height, right->Shape.Width);
			} else if (right->Shape.Height == 1 && right->Shape.Width == 1) {
				mx = InterpreterAllocMx(left->Shape.Height, left->Shape.Width);
			} else {
				mx = InterpreterAllocMx(left->Shape.Height, right->Shape.Width);
			}

			Result result = MxMultiply(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenDivide: {
			Mx* mx = nullptr;

			if (left->Shape.Height == 1 && left->Shape.Width == 1) {
				mx = InterpreterAllocMx(right->Shape.Height, right->Shape.Width);
			} else if (right->Shape.Height == 1 && right->Shape.Width == 1) {
				mx = InterpreterAllocMx(left->Shape.Height, left->Shape.Width);
			}

			Result result = MxDivide(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenToPower: {
			Mx* mx = InterpreterAllocMx(left->Shape.Height, left->Shape.Width);
			Result result = MxToPower(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenGreater: {
			Mx* mx = InterpreterAllocMx(1, 1);
			Result result = MxGreater(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenGreaterEqual: {
			Mx* mx = InterpreterAllocMx(1, 1);
			Result result = MxGreaterEqual(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenLess: {
			Mx* mx = InterpreterAllocMx(1, 1);
			Result result = MxLess(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenLessEqual: {
			Mx* mx = InterpreterAllocMx(1, 1);
			Result result = MxLessEqual(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenEqualEqual: {
			Mx* mx = InterpreterAllocMx(1, 1);
			Result result = MxEqualEqual(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenNotEqual: {
			Mx* mx = InterpreterAllocMx(1, 1);
			Result result = MxNotEqual(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenOr: {
			Mx* mx = InterpreterAllocMx(1, 1);
			MxLogicalOr(left, right, mx);
			return mx;
		}
		case TokenAnd: {
			Mx* mx = InterpreterAllocMx(1, 1);
			MxLogicalOr(left, right, mx);
			return mx;
		}
		default:
			// TODO: Panic!
			return nullptr;
		}
	}
	case ASTNodeBlock: {
		for (usz i = 0; i < node->Block.NodeCount; ++i) {
			InterpreterEval(node->Block.Nodes[i]);
		}

		return nullptr;
	}
	case ASTNodeIfStmt: {
		Mx* cond = InterpreterEval(node->IfStmt.Condition);
		if (!cond) {
			// TODO: Panic!
			return nullptr;
		}

		if (MxTruthy(cond)) {
			InterpreterEval(node->IfStmt.ThenBlock);
		} else if (node->IfStmt.ElseBlock) {
			InterpreterEval(node->IfStmt.ElseBlock);
		}

		return nullptr;
	}
	case ASTNodeWhileStmt: {
		while (true) {
			Mx* cond = InterpreterEval(node->WhileStmt.Condition);
			if (!cond) {
				// TODO: Panic!
				return nullptr;
			}

			if (!MxTruthy(cond)) {
				break;
			}

			InterpreterEval(node->WhileStmt.Body);
		}

		return nullptr;
	}
	case ASTNodeVarDecl: {
		usz id = node->VarDecl.ID;

		g_interpreter.VarTable[id] = InterpreterAllocMx(node->VarDecl.Shape.Height, node->VarDecl.Shape.Width);

		if (node->VarDecl.Expression) {
			Mx* initExpr = InterpreterEval(node->VarDecl.Expression);
			if (!initExpr) {
				// TODO: Panic!
				return nullptr;
			}

			for (usz i = 0; i < node->VarDecl.Shape.Height * node->VarDecl.Shape.Width; ++i) {
				g_interpreter.VarTable[id]->Data[i] = initExpr->Data[i];
			}
		}

		return nullptr;
	}
	case ASTNodeAssignment: {
		Mx* newVal = InterpreterEval(node->Assignment.Expression);
		if (!newVal) {
			// TODO: Panic!
			return nullptr;
		}

		usz id = node->Assignment.ID;
		Mx* var = g_interpreter.VarTable[id];

		if (!node->Assignment.Index) {
			memcpy(var->Data, newVal->Data, var->Shape.Height * var->Shape.Width * sizeof(f64));
		} else {
			Mx* mxI = InterpreterEval(node->Assignment.Index->IndexSuffix.I);
			if (!mxI) {
				// TODO: Panic!
				return nullptr;
			}

			if (!IsF64Int(mxI->Data[0])) {
				// TODO: Panic!
				DIAG_EMIT(DiagIndexNotInteger, node->Assignment.Index->IndexSuffix.I->Loc, DIAG_ARG_NUMBER(mxI->Data[0]));
				return nullptr;
			}

			usz i = (usz)mxI->Data[0];

			if (i < 1 || i > var->Shape.Height) {
				// TODO: Panic!
				DIAG_EMIT(DiagIndexOutOfRange, node->Assignment.Index->IndexSuffix.I->Loc, DIAG_ARG_NUMBER(mxI->Data[0]),
					DIAG_ARG_MX_SHAPE(var->Shape));
				return nullptr;
			}

			if (node->Assignment.Index->IndexSuffix.J) {
				Mx* mxJ = InterpreterEval(node->Assignment.Index->IndexSuffix.J);
				if (!mxJ) {
					// TODO: Panic!
					return nullptr;
				}

				if (!IsF64Int(mxJ->Data[0])) {
					// TODO: Panic!
					DIAG_EMIT(DiagIndexNotInteger, node->Assignment.Index->IndexSuffix.J->Loc, DIAG_ARG_NUMBER(mxJ->Data[0]));
					return nullptr;
				}

				usz j = (usz)mxJ->Data[0];

				if (j < 1 || j > var->Shape.Width) {
					// TODO: Panic!
					DIAG_EMIT(DiagIndexOutOfRange, node->Assignment.Index->IndexSuffix.J->Loc, DIAG_ARG_NUMBER(mxJ->Data[0]),
						DIAG_ARG_MX_SHAPE(var->Shape));
					return nullptr;
				}

				var->Data[((i - 1) * var->Shape.Width) + j - 1] = newVal->Data[0];
			} else {
				memcpy(var->Data + ((i - 1) * var->Shape.Width), newVal->Data, newVal->Shape.Width * sizeof(f64));
			}
		}

		return nullptr;
	}
	case ASTNodeFunctionCall: {
		if (node->FunctionCall.Identifier.SymbolLength == 7 && memcmp(node->FunctionCall.Identifier.Symbol, "display", 7) == 0) {
			return FuncInterpretDisplay(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 4 && memcmp(node->FunctionCall.Identifier.Symbol, "fill", 4) == 0) {
			return FuncInterpretFill(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 5 && memcmp(node->FunctionCall.Identifier.Symbol, "ident", 5) == 0) {
			return FuncInterpretIdent(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 3 && memcmp(node->FunctionCall.Identifier.Symbol, "log", 3) == 0) {
			return FuncInterpretLog(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 2 && memcmp(node->FunctionCall.Identifier.Symbol, "ln", 2) == 0) {
			return FuncInterpretLn(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 4 && memcmp(node->FunctionCall.Identifier.Symbol, "sqrt", 4) == 0) {
			return FuncInterpretSqrt(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 3 && memcmp(node->FunctionCall.Identifier.Symbol, "abs", 3) == 0) {
			return FuncInterpretAbs(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 4 && memcmp(node->FunctionCall.Identifier.Symbol, "ceil", 4) == 0) {
			return FuncInterpretCeil(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 5 && memcmp(node->FunctionCall.Identifier.Symbol, "floor", 5) == 0) {
			return FuncInterpretFloor(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 3 && memcmp(node->FunctionCall.Identifier.Symbol, "sin", 3) == 0) {
			return FuncInterpretSin(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 3 && memcmp(node->FunctionCall.Identifier.Symbol, "cos", 3) == 0) {
			return FuncInterpretCos(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 3 && memcmp(node->FunctionCall.Identifier.Symbol, "tan", 3) == 0) {
			return FuncInterpretTan(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 3 && memcmp(node->FunctionCall.Identifier.Symbol, "cot", 3) == 0) {
			return FuncInterpretCot(node);
		}

		if (node->FunctionCall.Identifier.SymbolLength == 4 && memcmp(node->FunctionCall.Identifier.Symbol, "rand", 4) == 0) {
			return FuncInterpretRand(node);
		}

		return nullptr;
	}
	case ASTNodeIdentifier: {
		usz id = node->Identifier.ID;
		Mx* var = g_interpreter.VarTable[id];

		if (node->Identifier.Index) {
			Mx* mxI = InterpreterEval(node->Identifier.Index->IndexSuffix.I);
			if (!mxI) {
				// TODO: Panic!
				return nullptr;
			}

			if (!IsF64Int(mxI->Data[0])) {
				// TODO: Panic!
				DIAG_EMIT(DiagIndexNotInteger, node->Identifier.Index->IndexSuffix.I->Loc, DIAG_ARG_NUMBER(mxI->Data[0]));
				return nullptr;
			}

			usz i = (usz)mxI->Data[0];

			if (i < 1 || i > var->Shape.Height) {
				// TODO: Panic!
				DIAG_EMIT(DiagIndexOutOfRange, node->Identifier.Index->IndexSuffix.I->Loc, DIAG_ARG_NUMBER(mxI->Data[0]),
					DIAG_ARG_MX_SHAPE(var->Shape));
				return nullptr;
			}

			if (node->Identifier.Index->IndexSuffix.J) {
				Mx* mxJ = InterpreterEval(node->Identifier.Index->IndexSuffix.J);
				if (!mxJ) {
					// TODO: Panic!
					return nullptr;
				}

				if (!IsF64Int(mxJ->Data[0])) {
					// TODO: Panic!
					DIAG_EMIT(DiagIndexNotInteger, node->Identifier.Index->IndexSuffix.J->Loc, DIAG_ARG_NUMBER(mxJ->Data[0]));
					return nullptr;
				}

				usz j = (usz)mxJ->Data[0];

				if (j < 1 || j > var->Shape.Width) {
					// TODO: Panic!
					DIAG_EMIT(DiagIndexOutOfRange, node->Identifier.Index->IndexSuffix.J->Loc, DIAG_ARG_NUMBER(mxJ->Data[0]),
						DIAG_ARG_MX_SHAPE(var->Shape));
					return nullptr;
				}

				Mx* mx = InterpreterAllocMx(1, 1);
				mx->Data[0] = var->Data[((i - 1) * var->Shape.Width) + j - 1];
				return mx;
			}

			Mx* mx = InterpreterAllocMx(1, var->Shape.Width);
			memcpy(mx->Data, var->Data + ((i - 1) * var->Shape.Width), var->Shape.Width * sizeof(f64));
			return mx;
		}

		Mx* mx = InterpreterAllocMx(var->Shape.Height, var->Shape.Width);
		memcpy(mx->Data, var->Data, var->Shape.Height * var->Shape.Width * sizeof(f64));
		return mx;
	}
	default:
		// TODO: Panic!
		return nullptr;
	}
}

void InterpreterInterpret() { InterpreterEval((ASTNode*)g_parser.ASTArena.Blocks->Data); }

Result InterpreterDeinit()
{
	free((void*)g_interpreter.VarTable);

	return DynArenaDeinit(&g_interpreter.MxArena);
}
