#pragma once

#include "Mx.h"
#include "Parser.h"

Mx* FuncInterpretDisplay(ASTNode* functionCall);
Mx* FuncInterpretFill(ASTNode* functionCall);
Mx* FuncInterpretIdent(ASTNode* functionCall);
Mx* FuncInterpretLog(ASTNode* functionCall);
Mx* FuncInterpretLn(ASTNode* functionCall);
Mx* FuncInterpretSqrt(ASTNode* functionCall);
Mx* FuncInterpretAbs(ASTNode* functionCall);
Mx* FuncInterpretCeil(ASTNode* functionCall);
Mx* FuncInterpretFloor(ASTNode* functionCall);
Mx* FuncInterpretSin(ASTNode* functionCall);
Mx* FuncInterpretCos(ASTNode* functionCall);
Mx* FuncInterpretTan(ASTNode* functionCall);
Mx* FuncInterpretCot(ASTNode* functionCall);
Mx* FuncInterpretRand(ASTNode* functionCall);
Mx* FuncInterpretInput(ASTNode* functionCall);
