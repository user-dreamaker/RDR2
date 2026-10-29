#include "inc\main.h"
#include "script.h"
#include "keyboard.h"

extern void SniperHook_Shutdown();

HMODULE g_hModule = nullptr;

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		g_hModule = hInstance;
		scriptRegister(hInstance, ScriptMain);
		keyboardHandlerRegister(OnKeyboardMessage);
		break;
	case DLL_PROCESS_DETACH:
		SniperHook_Shutdown();
		scriptUnregister(hInstance);
		keyboardHandlerUnregister(OnKeyboardMessage);
		break;
	}
	return TRUE;
}
