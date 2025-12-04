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
	Identifier,
	Number,
	FunctionCall
} ASTNodeType;

struct ASTNode;

typedef struct ASTNode {
	ASTNodeType Type;

	union {
		double Number;

		struct {
			struct ASTNode** Matrix;
			size_t Height;
			size_t Width;
		} Literal;

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

		struct {
			Token* Identifier;
			// Nullable
			struct ASTNode* Index;
		} Identifier;

		struct {
			Token* Identifier;
			struct ASTNode** CallArgs;
			size_t ArgCount;
		} FunctionCall;
	} Data;
} ASTNode;

void ASTNodePrint(const ASTNode* node)
{
	switch (node->Type) {
	case Number:
		printf("%lf", node->Data.Number);
		break;
	case Literal:
		printf("(lit %lux%lu ", node->Data.Literal.Height, node->Data.Literal.Width);
		for (size_t i = 0; i < node->Data.Literal.Height; ++i) {
			printf("(row ");
			for (size_t j = 0; j < node->Data.Literal.Width; ++j) {
				ASTNodePrint(node->Data.Literal.Matrix[(i * node->Data.Literal.Width) + j]);

				if (j < node->Data.Literal.Width - 1) {
					printf(" ");
				}
			}
			printf(")");
		}
		printf(")");
		break;
	case Block:
		printf("(block\n");
		for (size_t i = 0; i < node->Data.Block.NodeCount; ++i) {
			printf("  ");
			ASTNodePrint(node->Data.Block.Nodes[i]);
			printf("\n");
		}
		printf(")\n");
		break;
	case Unary:
		printf("(un%s ", node->Data.Unary.Operator->Lexeme);
		ASTNodePrint(node->Data.Unary.Operand);
		printf(")");
		break;
	case Grouping:
		printf("(grouping ");
		ASTNodePrint(node->Data.Grouping.Expression);
		printf(")");
		break;
	case Binary:
		printf("(bin%s ", node->Data.Binary.Operator->Lexeme);
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
		if (node->Data.IfStmt.ElseBlock) {
			printf(" else ");
			ASTNodePrint(node->Data.IfStmt.ElseBlock);
		}
		printf(" )");
		break;
	case IndexSuffix:
		printf("(index ");
		ASTNodePrint(node->Data.IndexSuffix.I);
		if (node->Data.IndexSuffix.J) {
			printf(" ");
			ASTNodePrint(node->Data.IndexSuffix.J);
		}
		printf(")");
		break;
	case Assignment:
		printf("(assignment %s ", node->Data.Assignment.Identifier->Lexeme);
		if (node->Data.Assignment.Index) {
			ASTNodePrint(node->Data.Assignment.Index);
		}
		printf(" ");
		ASTNodePrint(node->Data.Assignment.Expression);
		printf(")");
		break;
	case Identifier:
		printf("(ident %s ", node->Data.Identifier.Identifier->Lexeme);
		if (node->Data.Identifier.Index) {
			ASTNodePrint(node->Data.Identifier.Index);
		}
		printf(")");
		break;
	case FunctionCall:
		printf("(call %s ", node->Data.FunctionCall.Identifier->Lexeme);
		for (size_t i = 0; i < node->Data.FunctionCall.ArgCount; ++i) {
			printf("(arg ");
			ASTNodePrint(node->Data.FunctionCall.CallArgs[i]);
			printf(")");
		}
		printf(")");
		break;
	}
}

typedef struct Mx {
	double* Data;
	size_t Height;
	size_t Width;
} Mx;

void MxPrint(const Mx* mx)
{
	for (size_t i = 0; i < mx->Height; ++i) {
		printf("[");

		for (size_t j = 0; j < mx->Width; ++j) {
			printf("%lf", mx->Data[(i * mx->Width) + j]);

			if (j < mx->Width - 1) {
				printf(" ");
			}
		}

		printf("]");
	}
}

Mx* MxAdd(const Mx* left, const Mx* right)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(left->Width * left->Height * sizeof(double));
	result->Height = left->Height;
	result->Width = left->Width;
	memset(result->Data, 0, result->Height * result->Width * sizeof(double));

	for (size_t i = 0; i < left->Height * left->Width; ++i) {
		result->Data[i] = left->Data[i] + right->Data[i];
	}

	return result;
}

Mx* MxSubtract(const Mx* left, const Mx* right)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(left->Width * left->Height * sizeof(double));
	result->Height = left->Height;
	result->Width = left->Width;
	memset(result->Data, 0, result->Height * result->Width * sizeof(double));

	for (size_t i = 0; i < left->Height * left->Width; ++i) {
		result->Data[i] = left->Data[i] - right->Data[i];
	}

	return result;
}

Mx* MxMultiply(const Mx* left, const Mx* right)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(left->Height * right->Width * sizeof(double));
	result->Height = left->Height;
	result->Width = right->Width;
	memset(result->Data, 0, result->Height * result->Width * sizeof(double));

	for (size_t i = 0; i < left->Height; ++i) {
		for (size_t j = 0; j < right->Height; ++j) {
			for (size_t k = 0; k < right->Width; ++k) {
				result->Data[(i * result->Width) + k] += left->Data[(i * left->Width) + j] * right->Data[(j * right->Width) + k];
			}
		}
	}

	return result;
}

Mx* MxDivide(const Mx* left, const Mx* right)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(left->Height * right->Width * sizeof(double));
	result->Height = left->Height;
	result->Width = right->Width;
	memset(result->Data, 0, result->Height * result->Width * sizeof(double));

	for (size_t i = 0; i < left->Height; ++i) {
		for (size_t j = 0; j < right->Height; ++j) {
			for (size_t k = 0; k < right->Width; ++k) {
				result->Data[(i * result->Width) + k] += left->Data[(i * left->Width) + j] / right->Data[(j * right->Width) + k];
			}
		}
	}

	return result;
}

Mx* MxPower(const Mx* left, double right)
{
	Mx* mx = (Mx*)left;
	for (size_t i = 0; i < (size_t)right - 1; ++i) {
		mx = MxMultiply(mx, left);
	}

	return mx;
}

Mx* MxTranspose(const Mx* mx)
{
	Mx* result = malloc(sizeof(Mx));
	result->Height = mx->Width;
	result->Width = mx->Height;
	result->Data = malloc(result->Height * result->Width * sizeof(double));

	for (size_t i = 0; i < mx->Height; ++i) {
		for (size_t j = 0; j < mx->Width; ++j) {
			result->Data[(j * result->Width) + i] = mx->Data[(i * mx->Width) + j];
		}
	}

	return result;
}

Mx* MxNegate(const Mx* mx)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(mx->Width * mx->Height * sizeof(double));
	result->Height = mx->Height;
	result->Width = mx->Width;
	memset(result->Data, 0, result->Height * result->Width * sizeof(double));

	for (size_t i = 0; i < mx->Height * mx->Width; ++i) {
		result->Data[i] = -mx->Data[i];
	}

	return result;
}

Mx* MxGreater(const Mx* left, const Mx* right)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(sizeof(double));
	result->Data[0] = 1;
	result->Height = 1;
	result->Width = 1;

	for (size_t i = 0; i < left->Height * left->Width; ++i) {
		if (left->Data[i] <= right->Data[i]) {
			result->Data[0] = 0;
			break;
		}
	}

	return result;
}

Mx* MxGreaterEqual(const Mx* left, const Mx* right)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(sizeof(double));
	result->Data[0] = 1;
	result->Height = 1;
	result->Width = 1;

	for (size_t i = 0; i < left->Height * left->Width; ++i) {
		if (left->Data[i] < right->Data[i]) {
			result->Data[0] = 0;
			break;
		}
	}

	return result;
}

Mx* MxLess(const Mx* left, const Mx* right)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(sizeof(double));
	result->Data[0] = 1;
	result->Height = 1;
	result->Width = 1;

	for (size_t i = 0; i < left->Height * left->Width; ++i) {
		if (left->Data[i] >= right->Data[i]) {
			result->Data[0] = 0;
			break;
		}
	}

	return result;
}

Mx* MxLessEqual(const Mx* left, const Mx* right)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(sizeof(double));
	result->Data[0] = 1;
	result->Height = 1;
	result->Width = 1;

	for (size_t i = 0; i < left->Height * left->Width; ++i) {
		if (left->Data[i] > right->Data[i]) {
			result->Data[0] = 0;
			break;
		}
	}

	return result;
}

Mx* MxEqualEqual(const Mx* left, const Mx* right)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(sizeof(double));
	result->Data[0] = 1;
	result->Height = 1;
	result->Width = 1;

	for (size_t i = 0; i < left->Height * left->Width; ++i) {
		if (left->Data[i] != right->Data[i]) {
			result->Data[0] = 0;
			break;
		}
	}

	return result;
}

Mx* MxNotEqual(const Mx* left, const Mx* right)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(sizeof(double));
	result->Data[0] = 1;
	result->Height = 1;
	result->Width = 1;

	for (size_t i = 0; i < left->Height * left->Width; ++i) {
		if (left->Data[i] == right->Data[i]) {
			result->Data[0] = 0;
			break;
		}
	}

	return result;
}

bool MxIsTruthy(const Mx* mx)
{
	for (size_t i = 0; i < mx->Height * mx->Width; ++i) {
		if (mx->Data[i] == 0) {
			return false;
		}
	}

	return true;
}

Mx* MxLogicalOr(const Mx* left, const Mx* right)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(sizeof(double));
	result->Data[0] = 1;
	result->Height = 1;
	result->Width = 1;

	size_t leftSize = left->Height * left->Width;
	size_t rightSize = right->Height * right->Width;
	size_t smallerSize = leftSize < rightSize ? leftSize : rightSize;

	for (size_t i = 0; i < smallerSize; ++i) {
		if (!((bool)left[i].Data[i] || (bool)right[i].Data[i])) {
			result->Data[0] = 0;
			break;
		}
	}

	return result;
}

Mx* MxLogicalAnd(const Mx* left, const Mx* right)
{
	Mx* result = malloc(sizeof(Mx));
	result->Data = malloc(sizeof(double));
	result->Data[0] = 1;
	result->Height = 1;
	result->Width = 1;

	size_t leftSize = left->Height * left->Width;
	size_t rightSize = right->Height * right->Width;
	size_t smallerSize = leftSize < rightSize ? leftSize : rightSize;

	for (size_t i = 0; i < smallerSize; ++i) {
		if (!((bool)left[i].Data[i] && (bool)right[i].Data[i])) {
			result->Data[0] = 0;
			break;
		}
	}

	return result;
}

typedef struct MxShape {
	size_t Height;
	size_t Width;
} MxShape;

typedef struct VarType {
	const char* Name;
	MxShape* ValueType;
	bool IsConst;
} VarType;

typedef struct EnvType {
	struct EnvType* Parent;
	VarType* Vars;
	size_t VarCount;
	size_t NextFreeVar;
} EnvType;

EnvType* g_currentEnvType = nullptr;

VarType* EnvTypeGetVarType(EnvType* env, const char* name)
{
	for (size_t i = 0; i < env->VarCount; ++i) {
		if (strcmp(env->Vars[i].Name, name) == 0) {
			return env->Vars + i;
		}
	}

	if (env->Parent) {
		return EnvTypeGetVarType(env->Parent, name);
	}

	return nullptr;
}

VarType* EnvTypeGetVarTypeNoParent(EnvType* env, const char* name)
{
	for (size_t i = 0; i < env->VarCount; ++i) {
		if (strcmp(env->Vars[i].Name, name) == 0) {
			return env->Vars + i;
		}
	}

	return nullptr;
}

MxShape* ProcessVarType(const char* type)
{
	size_t heightEnd = 0;
	while (isdigit(type[heightEnd])) {
		++heightEnd;
	}

	size_t widthBegin = heightEnd + 1;
	size_t widthEnd = widthBegin;

	while (isdigit(type[widthEnd])) {
		++widthEnd;
	}

	char* heightStr = malloc((heightEnd + 1) * sizeof(char));
	heightStr[heightEnd] = '\0';
	memcpy(heightStr, type, heightEnd);

	char* widthStr = malloc((widthEnd - widthBegin + 1) * sizeof(char));
	widthStr[widthEnd - widthBegin] = '\0';
	memcpy(widthStr, type + widthBegin, widthEnd - widthBegin);

	MxShape* result = malloc(sizeof(MxShape));
	result->Height = (size_t)atoll(heightStr);
	result->Width = (size_t)atoll(widthStr);

	return result;
}

MxShape* ASTTypeEval(ASTNode* node)
{
	switch (node->Type) {
	case Number: {
		MxShape* result = malloc(sizeof(MxShape));
		result->Height = 1;
		result->Width = 1;

		return result;
	}
	case Literal: {
		MxShape* result = malloc(sizeof(MxShape));
		result->Height = node->Data.Literal.Height;
		result->Width = node->Data.Literal.Width;

		return result;
	}
	case Block: {
		EnvType* temp = g_currentEnvType;
		g_currentEnvType = malloc(sizeof(EnvType));
		g_currentEnvType->Vars = malloc(64 * sizeof(VarType));
		g_currentEnvType->VarCount = 0;
		g_currentEnvType->NextFreeVar = 0;
		g_currentEnvType->Parent = temp;

		for (size_t i = 0; i < node->Data.Block.NodeCount; ++i) {
			ASTTypeEval(node->Data.Block.Nodes[i]);
		}

		g_currentEnvType = temp;
		return nullptr;
	}
	case Unary: {
		MxShape* operand = ASTTypeEval(node->Data.Unary.Operand);

		MxShape* result = malloc(sizeof(MxShape));

		switch (node->Data.Unary.Operator->Type) {
		case TokenMinus:
			result->Height = operand->Height;
			result->Width = operand->Width;
			return result;
		case TokenApostrophe:
			result->Height = operand->Width;
			result->Width = operand->Height;
			return result;
		default:
			printf("Invalid unary operator!\n");
			exit(1);
		}
	}
	case Grouping:
		return ASTTypeEval(node->Data.Grouping.Expression);
	case Binary: {
		MxShape* left = ASTTypeEval(node->Data.Binary.Left);
		MxShape* right = ASTTypeEval(node->Data.Binary.Right);

		switch (node->Data.Binary.Operator->Type) {
		case TokenPlus:
		case TokenMinus:
			if (left->Height == right->Height && left->Width == right->Width) {
				MxShape* result = malloc(sizeof(MxShape));
				result->Height = left->Height;
				result->Width = left->Width;
				return result;
			} else if (left->Height == 1 && left->Width == 1) {
				MxShape* result = malloc(sizeof(MxShape));
				result->Height = right->Height;
				result->Width = right->Width;
				return result;
			} else if (right->Height == 1 && right->Width == 1) {
				MxShape* result = malloc(sizeof(MxShape));
				result->Height = left->Height;
				result->Width = left->Width;
				return result;
			} else {
				printf("Incompatible matrix shapes!\n");
				exit(1);
			}
		case TokenStar:
		case TokenSlash:
			if (left->Width == right->Height) {
				MxShape* result = malloc(sizeof(MxShape));
				result->Height = left->Height;
				result->Width = right->Width;
				return result;
			} else if (left->Height == 1 && left->Width == 1) {
				MxShape* result = malloc(sizeof(MxShape));
				result->Height = right->Height;
				result->Width = right->Width;
				return result;
			} else if (right->Height == 1 && right->Width == 1) {
				MxShape* result = malloc(sizeof(MxShape));
				result->Height = left->Height;
				result->Width = left->Width;
				return result;
			} else {
				printf("Incompatible matrix shapes!\n");
				exit(1);
			}
		case TokenUpArrow:
			if (right->Height != 1 || right->Width != 1) {
				printf("Invalid matrix right operand for powering!\n");
				exit(1);
			} else {
				MxShape* result = malloc(sizeof(MxShape));
				result->Height = left->Height;
				result->Width = left->Width;
				return result;
			}
		case TokenGreater:
		case TokenGreaterEqual:
		case TokenLess:
		case TokenLessEqual:
		case TokenEqualEqual:
		case TokenNotEqual:
			if (left->Height == right->Height && left->Width == right->Width) {
				MxShape* result = malloc(sizeof(MxShape));
				result->Height = 1;
				result->Width = 1;
				return result;
			} else {
				printf("Incompatible matrix shapes!\n");
				exit(1);
			}
		case TokenOr: {
			MxShape* result = malloc(sizeof(MxShape));
			result->Height = 1;
			result->Width = 1;
			return result;
		}
		default:
			printf("Error: Unknown binary operator '%s'!\n", node->Data.Binary.Operator->Lexeme);
			exit(1);
		}
	}
	case IfStmt: {
		ASTTypeEval(node->Data.IfStmt.Condition);
		ASTTypeEval(node->Data.IfStmt.ThenBlock);

		if (node->Data.IfStmt.ElseBlock) {
			ASTTypeEval(node->Data.IfStmt.ElseBlock);
		}

		return nullptr;
	}
	case WhileStmt: {
		ASTTypeEval(node->Data.WhileStmt.Condition);
		ASTTypeEval(node->Data.WhileStmt.Body);

		return nullptr;
	}
	case VarDecl: {
		if (!node->Data.VarDecl.Expression) {
			printf("Variable '%s' has not been initialized!\n", node->Data.VarDecl.Identifier->Lexeme);
			exit(1);
		}

		if (EnvTypeGetVarTypeNoParent(g_currentEnvType, node->Data.VarDecl.Identifier->Lexeme)) {
			printf("Variable '%s' has already been declared in the current scope!\n", node->Data.VarDecl.Identifier->Lexeme);
			exit(1);
		}

		size_t i = g_currentEnvType->NextFreeVar++;
		g_currentEnvType->Vars[i].Name = node->Data.VarDecl.Identifier->Lexeme;
		g_currentEnvType->Vars[i].ValueType = ASTTypeEval(node->Data.VarDecl.Expression);
		g_currentEnvType->Vars[i].IsConst = node->Data.VarDecl.IsConst;
		g_currentEnvType->VarCount++;

		if (node->Data.VarDecl.Type) {
			MxShape* type = ProcessVarType(node->Data.VarDecl.Type->Lexeme);

			if (type->Height != g_currentEnvType->Vars[i].ValueType->Height || type->Width != g_currentEnvType->Vars[i].ValueType->Width) {
				printf("Variable declaration has different explicitly declared type than initializer!\n");
				exit(1);
			}
		}

		return nullptr;
	}
	case Assignment: {
		MxShape* newValueType = ASTTypeEval(node->Data.Assignment.Expression);
		if (!newValueType) {
			printf("Expression does not return a value!\n");
			exit(1);
		}

		VarType* var = EnvTypeGetVarType(g_currentEnvType, node->Data.Assignment.Identifier->Lexeme);
		if (!var) {
			printf("Undeclared variable '%s'!\n", node->Data.Assignment.Identifier->Lexeme);
			exit(1);
		}

		if (var->IsConst) {
			printf("Attempted assignment to const variable '%s'!\n", node->Data.Assignment.Identifier->Lexeme);
			exit(1);
		}

		if (node->Data.Assignment.Index) {
			if (!node->Data.Assignment.Index->Data.IndexSuffix.J) {
				if (newValueType->Height != 1 || newValueType->Width != var->ValueType->Width) {
					printf("Incompatible matrix shapes!\n");
					exit(1);
				}
			} else {
				if (newValueType->Height != 1 || newValueType->Width != 1) {
					printf("Incompatible matrix shapes!\n");
					exit(1);
				}
			}
		} else {
			if (newValueType->Height != var->ValueType->Height || newValueType->Width != var->ValueType->Width) {
				printf("Incompatible matrix shapes!\n");
				exit(1);
			}
		}

		return nullptr;
	}
	case FunctionCall:
		// Only print is implemented for now, the other functions don't really make sense without a static type checker
		if (strcmp(node->Data.FunctionCall.Identifier->Lexeme, "display") != 0) {
			printf("Unimplemented function '%s'!\n", node->Data.FunctionCall.Identifier->Lexeme);
			exit(1);
		}

		if (node->Data.FunctionCall.ArgCount < 1) {
			printf("Invalid print usage, requires at least one argument!\n");
			exit(1);
		}

		for (size_t i = 0; i < node->Data.FunctionCall.ArgCount; ++i) {
			ASTTypeEval(node->Data.FunctionCall.CallArgs[i]);
		}

		return nullptr;
	case Identifier: {
		VarType* varType = EnvTypeGetVarType(g_currentEnvType, node->Data.Identifier.Identifier->Lexeme);
		if (!varType) {
			printf("Undeclared variable '%s'!\n", node->Data.Identifier.Identifier->Lexeme);
			exit(1);
		}

		if (node->Data.Identifier.Index) {
			if (!node->Data.Identifier.Index->Data.IndexSuffix.J) {
				MxShape* result = malloc(sizeof(MxShape));
				result->Height = 1;
				result->Width = varType->ValueType->Width;
				return result;
			} else {
				MxShape* result = malloc(sizeof(MxShape));
				result->Height = 1;
				result->Width = 1;
				return result;
			}
		} else {
			MxShape* result = malloc(sizeof(MxShape));
			result->Height = varType->ValueType->Height;
			result->Width = varType->ValueType->Width;
			return result;
		}
	}
	default:
		printf("Error: Unimplemented ASTNode type %u!\n", node->Type);
		exit(1);
	}
}

typedef struct Var {
	const char* Name;
	Mx* Value;
	bool IsConst;
} Var;

typedef struct Env {
	struct Env* Parent;
	Var* Vars;
	size_t VarCount;
	size_t NextFreeVar;
} Env;

Env* g_currentEnv = nullptr;

Var* EnvGetVar(Env* env, const char* name)
{
	for (size_t i = 0; i < env->VarCount; ++i) {
		if (strcmp(env->Vars[i].Name, name) == 0) {
			return env->Vars + i;
		}
	}

	if (env->Parent) {
		return EnvGetVar(env->Parent, name);
	}

	return nullptr;
}

Var* EnvGetVarNoParent(Env* env, const char* name)
{
	for (size_t i = 0; i < env->VarCount; ++i) {
		if (strcmp(env->Vars[i].Name, name) == 0) {
			return env->Vars + i;
		}
	}

	return nullptr;
}

Mx* ASTNodeEval(ASTNode* node)
{
	switch (node->Type) {
	case Number: {
		Mx* result = malloc(sizeof(Mx));
		result->Data = malloc(sizeof(double));
		result->Data[0] = node->Data.Number;
		result->Height = 1;
		result->Width = 1;

		return result;
	}
	case Literal: {
		Mx* result = malloc(sizeof(Mx));
		result->Data = malloc(node->Data.Literal.Height * node->Data.Literal.Width * sizeof(double));
		result->Height = node->Data.Literal.Height;
		result->Width = node->Data.Literal.Width;

		for (size_t i = 0; i < result->Height * result->Width; ++i) {
			// This must be a single number
			Mx* number = ASTNodeEval(node->Data.Literal.Matrix[i]);
			result->Data[i] = number->Data[0];
		}

		return result;
	}
	case Unary: {
		Mx* operand = ASTNodeEval(node->Data.Unary.Operand);

		switch (node->Data.Unary.Operator->Type) {
		case TokenMinus:
			return MxNegate(operand);
		case TokenApostrophe:
			return MxTranspose(operand);
		default:
			printf("Invalid unary operator!\n");
			exit(1);
		}
	}
	case Grouping:
		return ASTNodeEval(node->Data.Grouping.Expression);
	case Binary: {
		Mx* left = ASTNodeEval(node->Data.Binary.Left);
		Mx* right = ASTNodeEval(node->Data.Binary.Right);

		switch (node->Data.Binary.Operator->Type) {
		case TokenPlus:
			return MxAdd(left, right);
		case TokenMinus:
			return MxSubtract(left, right);
		case TokenStar:
			return MxMultiply(left, right);
		case TokenSlash:
			return MxDivide(left, right);
		case TokenUpArrow:
			return MxPower(left, right->Data[0]);
		case TokenGreater:
			return MxGreater(left, right);
		case TokenGreaterEqual:
			return MxGreaterEqual(left, right);
		case TokenLess:
			return MxLess(left, right);
		case TokenLessEqual:
			return MxLessEqual(left, right);
		case TokenEqualEqual:
			return MxEqualEqual(left, right);
		case TokenNotEqual:
			return MxNotEqual(left, right);
		case TokenOr:
			return MxLogicalOr(left, right);
		case TokenAnd:
			return MxLogicalAnd(left, right);
		default:
			printf("Error: Unknown binary operator '%s'!\n", node->Data.Binary.Operator->Lexeme);
			exit(1);
		}
	}
	case Block: {
		Env* temp = g_currentEnv;
		g_currentEnv = malloc(sizeof(Env));
		g_currentEnv->Vars = malloc(64 * sizeof(Var));
		g_currentEnv->VarCount = 0;
		g_currentEnv->NextFreeVar = 0;
		g_currentEnv->Parent = temp;

		for (size_t i = 0; i < node->Data.Block.NodeCount; ++i) {
			ASTNodeEval(node->Data.Block.Nodes[i]);
		}

		free(g_currentEnv->Vars);
		free(g_currentEnv);

		g_currentEnv = temp;
		return nullptr;
	}
	case IfStmt: {
		if (MxIsTruthy(ASTNodeEval(node->Data.IfStmt.Condition))) {
			ASTNodeEval(node->Data.IfStmt.ThenBlock);
		} else if (node->Data.IfStmt.ElseBlock) {
			ASTNodeEval(node->Data.IfStmt.ElseBlock);
		}

		return nullptr;
	}
	case WhileStmt: {
		while (MxIsTruthy(ASTNodeEval(node->Data.WhileStmt.Condition))) {
			ASTNodeEval(node->Data.WhileStmt.Body);
		}

		return nullptr;
	}
	case VarDecl: {
		size_t i = g_currentEnv->NextFreeVar++;
		g_currentEnv->Vars[i].Name = node->Data.VarDecl.Identifier->Lexeme;
		g_currentEnv->Vars[i].Value = ASTNodeEval(node->Data.VarDecl.Expression);
		g_currentEnv->Vars[i].IsConst = node->Data.VarDecl.IsConst;
		g_currentEnv->VarCount++;

		return nullptr;
	}
	case Assignment: {
		Mx* newValue = ASTNodeEval(node->Data.Assignment.Expression);
		if (!newValue) {
			printf("Expression does not return a value!\n");
			exit(1);
		}

		Var* var = EnvGetVar(g_currentEnv, node->Data.Assignment.Identifier->Lexeme);

		if (node->Data.Assignment.Index) {
			Mx* mxI = ASTNodeEval(node->Data.Assignment.Index->Data.IndexSuffix.I);
			if (mxI->Height > 1 || mxI->Width > 1) {
				printf("Matrix's indexes can only be natural numbers!\n");
				exit(1);
			}
			size_t i = (size_t)mxI->Data[0];

			if (node->Data.Assignment.Index->Data.IndexSuffix.J) {
				Mx* mxJ = ASTNodeEval(node->Data.Assignment.Index->Data.IndexSuffix.J);
				if (mxJ->Height > 1 || mxJ->Width > 1) {
					printf("Matrix's indexes can only be natural numbers!\n");
					exit(1);
				}
				size_t j = (size_t)mxJ->Data[0];

				var->Value->Data[((i - 1) * var->Value->Width) + j - 1] = newValue->Data[0];
			}

			memcpy(var->Value->Data + ((i - 1) * var->Value->Width), newValue->Data, newValue->Width * sizeof(double));
		} else {
			var->Value = newValue;
		}
		return nullptr;
	}
	case FunctionCall: {
		for (size_t i = 0; i < node->Data.FunctionCall.ArgCount; ++i) {
			if (node->Data.FunctionCall.CallArgs[i]->Type == Identifier) {
				printf("%s: ", node->Data.FunctionCall.CallArgs[i]->Data.Identifier.Identifier->Lexeme);
			} else {
				printf("imm: ");
			}

			MxPrint(ASTNodeEval(node->Data.FunctionCall.CallArgs[i]));

			if (i < node->Data.FunctionCall.ArgCount - 1) {
				printf(", ");
			}
		}
		printf("\n");

		return nullptr;
	}
	case Identifier: {
		Var* var = EnvGetVar(g_currentEnv, node->Data.Identifier.Identifier->Lexeme);

		if (node->Data.Identifier.Index) {
			Mx* mxI = ASTNodeEval(node->Data.Identifier.Index->Data.IndexSuffix.I);
			if (mxI->Height > 1 || mxI->Width > 1) {
				printf("Matrix's indexes can only be natural numbers!\n");
				exit(1);
			}
			size_t i = (size_t)mxI->Data[0];

			if (node->Data.Identifier.Index->Data.IndexSuffix.J) {
				Mx* mxJ = ASTNodeEval(node->Data.Identifier.Index->Data.IndexSuffix.J);
				if (mxJ->Height > 1 || mxJ->Width > 1) {
					printf("Matrix's indexes can only be natural numbers!\n");
					exit(1);
				}
				size_t j = (size_t)mxJ->Data[0];

				Mx* result = malloc(sizeof(Mx));
				result->Data = malloc(sizeof(double));
				result->Data[0] = var->Value->Data[((i - 1) * var->Value->Width) + j - 1];
				result->Height = 1;
				result->Width = 1;

				return result;
			}

			Mx* result = malloc(sizeof(Mx));
			result->Data = malloc(sizeof(double));
			memcpy(result->Data, var->Value->Data + ((i - 1) * var->Value->Width), var->Value->Width * sizeof(double));
			result->Height = 1;
			result->Width = var->Value->Width;

			return result;
		}

		return var->Value;
	}
	default:
		printf("Error: Unimplemented ASTNode type %u!\n", node->Type);
		exit(1);
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
		number->Data.Literal.Matrix = (ASTNode**)malloc(sizeof(ASTNode*));
		number->Data.Literal.Matrix[0] = malloc(sizeof(ASTNode));
		number->Data.Literal.Matrix[0]->Type = Number;
		number->Data.Literal.Matrix[0]->Data.Number = parser->Tokens[parser->CurrentToken - 1].Number;
		number->Data.Literal.Width = 1;
		number->Data.Literal.Height = 1;

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
		ParserAdvance(parser);

		ASTNode* matrixLit = malloc(sizeof(ASTNode));
		matrixLit->Type = Literal;
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

ASTNode* ParseIdentifierPrimary(Parser* parser)
{
	Token* identifier = parser->Tokens + parser->CurrentToken;
	ParserAdvance(parser);

	// A function call
	if (ParserMatch(parser, TokenLeftRoundBracket)) {
		ASTNode* functionCall = malloc(sizeof(ASTNode));
		functionCall->Type = FunctionCall;
		functionCall->Data.FunctionCall.Identifier = identifier;
		functionCall->Data.FunctionCall.CallArgs = (ASTNode**)malloc(8 * sizeof(ASTNode*));

		size_t i = 0;
		while (parser->Tokens[parser->CurrentToken].Type != TokenRightRoundBracket) {
			if (i > 0) {
				ParserExpect(parser, TokenComma);
			}

			functionCall->Data.FunctionCall.CallArgs[i] = ParseExpression(parser);
			++i;
		}

		ParserExpect(parser, TokenRightRoundBracket);
		functionCall->Data.FunctionCall.ArgCount = i;

		return functionCall;
	}

	// Not a function call
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

	ASTNode* astIdentifier = malloc(sizeof(ASTNode));
	astIdentifier->Type = Identifier;
	astIdentifier->Data.Identifier.Identifier = identifier;
	astIdentifier->Data.Identifier.Index = indexSuffix;

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
				++g_scanner.LexemeCurrent;
				ScannerAddToken(TokenLessEqual, 0);
			} else if (g_scanner.Source[g_scanner.LexemeCurrent] == '<') {
				++g_scanner.LexemeCurrent;
				ScannerAddToken(TokenLeftVectorBracket, 0);
			} else {
				ScannerAddToken(TokenLess, 0);
			}
			break;
		case '>':
			if (g_scanner.Source[g_scanner.LexemeCurrent] == '=') {
				++g_scanner.LexemeCurrent;
				ScannerAddToken(TokenGreaterEqual, 0);
			} else if (g_scanner.Source[g_scanner.LexemeCurrent] == '>') {
				++g_scanner.LexemeCurrent;
				ScannerAddToken(TokenRightVectorBracket, 0);
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
				++g_scanner.LexemeCurrent;
				ScannerAddToken(TokenEqualEqual, 0);
			} else {
				ScannerAddToken(TokenEqual, 0);
			}
			break;
		case '!':
			if (g_scanner.Source[g_scanner.LexemeCurrent] == '=') {
				++g_scanner.LexemeCurrent;
				ScannerAddToken(TokenNotEqual, 0);
			} else {
				printf("Unexpected character '!' at line %lu.\n", g_scanner.Line);
			}
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

					while (isdigit(g_scanner.Source[g_scanner.LexemeCurrent])) {
						++g_scanner.LexemeCurrent;
					}
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

	printf("\nType checker:\n");

	ASTTypeEval(program);

	printf("\nInterpreter:\n");

	ASTNodeEval(program);

	return 0;
}
