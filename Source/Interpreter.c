#include "Interpreter.h"

#include "MxShape.h"

Interpreter g_interpreter = {0};

Result InterpreterInit()
{
	Result result = DynArenaInit(&g_interpreter.DataArena);
	if (result) {
		return result;
	}

	return StatArenaInit(&g_interpreter.VarTableArena, sizeof(MxShape));
}

Result InterpreterInterpret()
{
	return ResOk;
}

Result InterpreterDeinit()
{
	Result result = DynArenaDeinit(&g_interpreter.DataArena);
	if (result) {
		return result;
	}

	return StatArenaDeinit(&g_interpreter.VarTableArena);
}
