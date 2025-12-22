#pragma once

#include "Memory/StatArena.h"
#include "Memory/DynArena.h"
#include "Memory/SymbolTable.h"
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
			TokenType Operator;
			struct ASTNode* Operand;
		} Unary;

		struct {
			struct ASTNode* Expression;
		} Grouping;

		struct {
			struct ASTNode* Left;
			TokenType Operator;
			struct ASTNode* Right;
		} Binary;

		struct {
			SymbolView Identifier;
			SymbolView Type;
			struct ASTNode* Expression;
			bool IsConst;
			bool HasType;
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
			SymbolView Identifier;
			struct ASTNode* Index;
			struct ASTNode* Expression;
		} Assignment;

		struct {
			SymbolView Identifier;
			struct ASTNode* Index;
		} Identifier;

		struct {
			SymbolView Identifier;
			struct ASTNode** CallArgs;
			usz ArgCount;
		} FunctionCall;
	} Data;
} ASTNode;

typedef struct Parser {
	StatArena ASTArena;
	DynArena ArraysArena;
} Parser;

Result ParserInit();
Result ParserParse();
void ParserPrintAST(const ASTNode* node);
Result ParserDeinit();

extern Parser g_parser;
