#pragma once

#include "inc\natives.h"
#include "inc\types.h"
#include "inc\main.h"

void ScriptMain();

extern HMODULE g_hModule;

// Stable persistence (script.cpp): donor-based slot registration in vanilla stable globals
void OnlineHorses_Init();
void OnlineHorses_Tick(bool nearStable);
void OnlineHorses_Shutdown();
