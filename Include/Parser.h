/*
MxLang - Parser.h
Autor: Alexander Dębowski (293472)
Data: 07.01.2026
*/

#pragma once

#include "MxShape.h"
#include "Memory/DynArena.h"
#include "Memory/StatArena.h"
#include "Memory/SymbolTable.h"
#include "Tokenizer.h"
#include "Types.h"

typedef enum ASTNodeType {
	ASTNodeMxLiteral,
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
	SourceLoc Loc;

	union {
		f64 Number;

		struct {
			struct ASTNode** Matrix;
			MxShape Shape;
		} MxLiteral;

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
			MxShape Shape;
			struct ASTNode* Expression;
			bool IsConst;
			bool HasDeclaredShape;
			usz ID;
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
			usz ID;
		} Assignment;

		struct {
			SymbolView Identifier;
			struct ASTNode* Index;
			usz ID;
		} Identifier;

		struct {
			SymbolView Identifier;
			struct ASTNode** CallArgs;
			usz ArgCount;
		} FnCall;
	};
} ASTNode;

typedef struct Parser {
	StatArena ASTArena;
	DynArena ArraysArena;
} Parser;

void ParserInit();
void ParserParse();
void ParserPrintAST(const ASTNode* node, usz indents);
void ParserDeinit();

extern Parser g_parser;
