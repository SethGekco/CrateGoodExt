// CrateGoodExt — standalone Yuri's Revenge Syringe DLL.
//
// Bootstrap only. Feature logic lives in CrateGood.cpp. This file
// carries the DLL entry point and a boot-time hook that confirms Syringe loaded
// us — with zero dependency on the Kratos framework.

#include "Log.h"

#include <Helpers/Macro.h>   // DEFINE_HOOK (pulls in Syringe.h)
#include <windows.h>

// Fires once early in YR's boot sequence. If this line appears in
// CrateGoodExt.log, Syringe loaded the DLL and executed our hook.
DEFINE_HOOK(0x52BA60, CrateGoodExt_Boot, 0x5)
{
	static bool logged = false;
	if (!logged)
	{
		logged = true;
		CrateGoodExt::Log("[CrateGoodExt] loaded; boot hook fired, DLL is live.");
	}
	return 0; // 0 = execute the original overwritten instructions and continue
}

// Standard Syringe/YRpp DLL entry. Mirrors Kratos's signature exactly.
bool __stdcall DllMain(HANDLE hInstance, DWORD dwReason, LPVOID /*reserved*/)
{
	(void)hInstance;
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		CrateGoodExt::Log("[CrateGoodExt] DllMain: DLL_PROCESS_ATTACH");
	}
	return true;
}
