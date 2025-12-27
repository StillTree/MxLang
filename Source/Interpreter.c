#include "Interpreter.h"

#include "Diagnostics.h"
#include "Mx.h"
#include "Parser.h"
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

static Mx* AllocMx(usz height, usz width)
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

static Mx* Interpret(ASTNode* node)
{
	switch (node->Type) {
	case ASTNodeNumber: {
		Mx* num = AllocMx(1, 1);
		num->Data[0] = node->Number;
		return num;
	}
	case ASTNodeMxLiteral: {
		Mx* mx = AllocMx(node->MxLiteral.Shape.Height, node->MxLiteral.Shape.Width);

		for (usz i = 0; i < node->MxLiteral.Shape.Height * node->MxLiteral.Shape.Width; ++i) {
			Mx* num = Interpret(node->MxLiteral.Matrix[i]);

			mx->Data[i] = num->Data[0];
		}

		return mx;
	}
	case ASTNodeUnary: {
		Mx* operand = Interpret(node->Unary.Operand);

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
			Mx* mx = AllocMx(operand->Shape.Width, operand->Shape.Height);
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
		return Interpret(node->Grouping.Expression);
	case ASTNodeBinary: {
		Mx* left = Interpret(node->Binary.Left);
		Mx* right = Interpret(node->Binary.Right);

		switch (node->Binary.Operator) {
		case TokenAdd: {
			Mx* mx = nullptr;

			if (left->Shape.Height == 1 && left->Shape.Width == 1) {
				mx = AllocMx(right->Shape.Height, right->Shape.Width);
			} else if (right->Shape.Height == 1 && right->Shape.Width == 1) {
				mx = AllocMx(left->Shape.Height, left->Shape.Width);
			} else {
				mx = AllocMx(left->Shape.Height, left->Shape.Width);
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
				mx = AllocMx(right->Shape.Height, right->Shape.Width);
			} else if (right->Shape.Height == 1 && right->Shape.Width == 1) {
				mx = AllocMx(left->Shape.Height, left->Shape.Width);
			} else {
				mx = AllocMx(left->Shape.Height, left->Shape.Width);
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
				mx = AllocMx(right->Shape.Height, right->Shape.Width);
			} else if (right->Shape.Height == 1 && right->Shape.Width == 1) {
				mx = AllocMx(left->Shape.Height, left->Shape.Width);
			} else {
				mx = AllocMx(left->Shape.Height, right->Shape.Width);
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
				mx = AllocMx(right->Shape.Height, right->Shape.Width);
			} else if (right->Shape.Height == 1 && right->Shape.Width == 1) {
				mx = AllocMx(left->Shape.Height, left->Shape.Width);
			}

			Result result = MxDivide(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenToPower: {
			Mx* mx = AllocMx(left->Shape.Height, left->Shape.Width);
			Result result = MxToPower(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenGreater: {
			Mx* mx = AllocMx(1, 1);
			Result result = MxGreater(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenGreaterEqual: {
			Mx* mx = AllocMx(1, 1);
			Result result = MxGreaterEqual(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenLess: {
			Mx* mx = AllocMx(1, 1);
			Result result = MxLess(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenLessEqual: {
			Mx* mx = AllocMx(1, 1);
			Result result = MxLessEqual(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenEqualEqual: {
			Mx* mx = AllocMx(1, 1);
			Result result = MxEqualEqual(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenNotEqual: {
			Mx* mx = AllocMx(1, 1);
			Result result = MxNotEqual(left, right, mx);
			if (result) {
				// TODO: Panic!
				return nullptr;
			}
			return mx;
		}
		case TokenOr: {
			Mx* mx = AllocMx(1, 1);
			MxLogicalOr(left, right, mx);
			return mx;
		}
		case TokenAnd: {
			Mx* mx = AllocMx(1, 1);
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
			Interpret(node->Block.Nodes[i]);
		}

		return nullptr;
	}
	case ASTNodeIfStmt: {
		Mx* cond = Interpret(node->IfStmt.Condition);
		if (!cond) {
			// TODO: Panic!
			return nullptr;
		}

		if (MxTruthy(cond)) {
			Interpret(node->IfStmt.ThenBlock);
		} else if (node->IfStmt.ElseBlock) {
			Interpret(node->IfStmt.ElseBlock);
		}

		return nullptr;
	}
	case ASTNodeWhileStmt: {
		while (true) {
			Mx* cond = Interpret(node->WhileStmt.Condition);
			if (!cond) {
				// TODO: Panic!
				return nullptr;
			}

			if (!MxTruthy(cond)) {
				break;
			}

			Interpret(node->WhileStmt.Body);
		}

		return nullptr;
	}
	case ASTNodeVarDecl: {
		usz id = node->VarDecl.ID;

		g_interpreter.VarTable[id] = AllocMx(node->VarDecl.Shape.Height, node->VarDecl.Shape.Width);

		if (node->VarDecl.Expression) {
			Mx* initExpr = Interpret(node->VarDecl.Expression);
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
		Mx* newVal = Interpret(node->Assignment.Expression);
		if (!newVal) {
			// TODO: Panic!
			return nullptr;
		}

		usz id = node->Assignment.ID;
		Mx* var = g_interpreter.VarTable[id];

		if (!node->Assignment.Index) {
			memcpy(var->Data, newVal->Data, var->Shape.Height * var->Shape.Width * sizeof(f64));
		} else {
			Mx* mxI = Interpret(node->Assignment.Index->IndexSuffix.I);
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
				Mx* mxJ = Interpret(node->Assignment.Index->IndexSuffix.J);
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
			for (size_t i = 0; i < node->FunctionCall.ArgCount; ++i) {
				if (node->FunctionCall.CallArgs[i]->Type == ASTNodeIdentifier) {
					printf("%.*s: ", (i32)node->FunctionCall.CallArgs[i]->Identifier.Identifier.SymbolLength,
						node->FunctionCall.CallArgs[i]->Identifier.Identifier.Symbol);
				} else {
					printf("imm: ");
				}

				Mx* mx = Interpret(node->FunctionCall.CallArgs[i]);
				if (!mx) {
					// TODO: Panic!
					return nullptr;
				}

				MxPrint(mx);

				if (i < node->FunctionCall.ArgCount - 1) {
					printf(", ");
				}
			}

			printf("\n");
		}

		return nullptr;
	}
	case ASTNodeIdentifier: {
		usz id = node->Identifier.ID;
		Mx* var = g_interpreter.VarTable[id];

		if (node->Identifier.Index) {
			Mx* mxI = Interpret(node->Identifier.Index->IndexSuffix.I);
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
				Mx* mxJ = Interpret(node->Identifier.Index->IndexSuffix.J);
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

				Mx* mx = AllocMx(1, 1);
				mx->Data[0] = var->Data[((i - 1) * var->Shape.Width) + j - 1];
				return mx;
			}

			Mx* mx = AllocMx(1, var->Shape.Width);
			memcpy(mx->Data, var->Data + ((i - 1) * var->Shape.Width), var->Shape.Width * sizeof(f64));
			return mx;
		}

		Mx* mx = AllocMx(var->Shape.Height, var->Shape.Width);
		memcpy(mx->Data, var->Data, var->Shape.Height * var->Shape.Width * sizeof(f64));
		return mx;
	}
	default:
		// TODO: Panic!
		return nullptr;
	}
}

void InterpreterInterpret() { Interpret((ASTNode*)g_parser.ASTArena.Blocks->Data); }

Result InterpreterDeinit()
{
	free((void*)g_interpreter.VarTable);

	return DynArenaDeinit(&g_interpreter.MxArena);
}
