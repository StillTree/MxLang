#include "TypeChecker.h"

#include "Diagnostics.h"
#include "Parser.h"
#include <stdio.h>

TypeChecker g_typeChecker = { 0 };

static usz GetSymbolID()
{
	static usz id = 0;
	return id++;
}

static Result BindingEnterScope()
{
	BindingScope* temp = g_typeChecker.CurScope;

	Result result = DynArenaAllocZeroed(
		&g_typeChecker.BindingArena, (void**)&g_typeChecker.CurScope, sizeof(BindingScope) + (64 * sizeof(BindingEntry)));
	if (result) {
		return result;
	}

	g_typeChecker.CurScope->Parent = temp;
	return ResOk;
}

static Result BindingExitScope()
{
	// TODO: Deallocate the previous scope!
	g_typeChecker.CurScope = g_typeChecker.CurScope->Parent;
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
		Result result = BindingInsert(g_typeChecker.CurScope, node->VarDecl.Identifier.Symbol, &newID);
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
		Result result = BindingLookup(g_typeChecker.CurScope, node->Assignment.Identifier.Symbol, &id);
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
		Result result = BindingLookup(g_typeChecker.CurScope, node->Identifier.Identifier.Symbol, &id);
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

Result TypeCheckerInit()
{
	Result result = DynArenaInit(&g_typeChecker.BindingArena);
	if (result) {
		return result;
	}

	g_typeChecker.CurScope = nullptr;
	return ResOk;
}

Result TypeCheckerSymbolBind() { return SymbolBind((ASTNode*)g_parser.ASTArena.Blocks->Data); }

Result TypeCheckerDeinit() { return DynArenaDeinit(&g_typeChecker.BindingArena); }
