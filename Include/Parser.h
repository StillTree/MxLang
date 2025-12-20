#pragma once

#include "Memory/StatArena.h"
#include "Memory/DynArena.h"
#include "Types.h"
#include "Tokenizer.h"

typedef enum ASTNodeType {
	ASTNodeLiteral,
	ASTNodeBlock,
	ASTNodeUnary,
	ASTNodeGrouping,
	ASTNodeBinary,
	ASTNodeVarDecl,
	ASTNodeWhileStmt,
	ASTNodeIfStmt,
	ASTNodeIndexSuffix,
	ASTNodeAssignment,
	ASTNodeIdentifier,
	ASTNodeNumber,
	ASTNodeFunctionCall
} ASTNodeType;

typedef struct ASTNode {
	ASTNodeType Type;

	union {
		double Number;

		struct {
			struct ASTNode** Matrix;
			usz Height;
			usz Width;
		} Literal;

		struct {
			struct ASTNode** Nodes;
			usz NodeCount;
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
			Token* Type;
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
			struct ASTNode* Index;
		} Identifier;

		struct {
			Token* Identifier;
			struct ASTNode** CallArgs;
			usz ArgCount;
		} FunctionCall;
	} Data;
} ASTNode;

typedef struct Parser {
	StatArena ASTArena;
	DynArena ArraysArena;
	StatArenaIter Iter;
	Token* CurToken;
} Parser;

extern Parser g_parser;
