/*
	ScriptHookRDR2 V2
*/

#pragma once

#include <cstdint>
#include <windows.h>

#include "inc\types.h"
#include "inc\main.h"
#include "inc\natives.h"

////////////
// main.h //
////////////

#ifndef DLL_IMPORT
#define DLL_IMPORT IMPORT
#endif

IMPORT int worldGetAllBlips(int *arr, int arrSize);
IMPORT int worldGetAllDoors(int *arr, int arrSize);

DLL_IMPORT uint64_t *getStaticPtr(const char *scriptName, int staticIndex);

DLL_IMPORT void switchLabel(const char *oldLabel, const char *newLabel);

DLL_IMPORT void *getCommandFromHash(uint64_t hash);

DLL_IMPORT void addCustomLabel(const char *oldLabel, const char *newLabel);

DLL_IMPORT void registerSprite(const char *name, const char *textureDict, const char *textureName);

DLL_IMPORT void scriptCleanup(HMODULE module, void(*LP_SCRIPT_MAIN)());

///////////////
// natives.h //
///////////////

namespace PED
{
	static int _GET_NUM_COMPONENT_CATEGORIES_IN_PED(Ped ped) { return invoke<int>(0xA622E66EEE92A08D, ped); }
	static Any _GET_PED_COMPONENT_CATEGORY_BY_INDEX(Ped ped, int index) { return invoke<Any>(0xCCB97B51893C662F, ped, index); }
	static BOOL _IS_META_PED_USING_COMPONENT(Ped ped, Any componentHash) { return invoke<BOOL>(0xFB4891BD7578CDC1, ped, componentHash); }
}

namespace UI
{
	static void SET_TEXT_JUSTIFICATION(int justifyType) { invoke<Void>(0x1B929974, justifyType); }
	static void SET_TEXT_FONT(int fontType) { invoke<Void>(0x3EDE2F10, fontType); }
}

namespace GRAPHICS
{
	static void DRAW_LINE(float fromX, float fromY, float fromZ, float toX, float toY, float toZ, int r, int g, int b, int a) { invoke<Void>(0xec2ab68b, fromX, fromY, fromZ, toX, toY, toZ, r, g, b, a); }
}
