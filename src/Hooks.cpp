// Syringe hooks for CrateGoodExt. Feature logic lives in CrateGood.cpp.
//
// Addresses here are also hooked by Phobos/Antares; Syringe chains multiple hooks
// per address and we keep no shared framework state, so this coexists cleanly
// (the same model GiftBoxHost relies on).
#include "CrateGood.h"

#include <Helpers/Macro.h>   // DEFINE_HOOK, GET
#include <CCINIClass.h>

// After all TypeData (unit/infantry/SW types) is loaded, (re)parse our
// [CrateGood.Ext_N] registry from the rules INI. Verified against Phobos source:
//   0x679CAF = RulesClass load, after-type-data; CCINIClass* is in ESI (size 0x5).
DEFINE_HOOK(0x679CAF, CrateGoodExt_RulesData_LoadAfterTypeData, 0x5)
{
	GET(CCINIClass*, pINI, ESI);
	CrateGoodExt::LoadRegistry(pINI);
	return 0;
}
