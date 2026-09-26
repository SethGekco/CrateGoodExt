// Syringe hooks for CrateGoodExt. Feature logic lives in CrateGood.cpp.
//
// Addresses here are also hooked by Phobos/Antares; Syringe chains multiple hooks
// per address and we keep no shared framework state, so this coexists cleanly
// (the same model GiftBoxHost relies on).
#include "CrateGood.h"

#include <Helpers/Macro.h>   // DEFINE_HOOK, GET, GET_BASE, R
#include <CCINIClass.h>
#include <UnitTypeClass.h>
#include <CellClass.h>
#include <FootClass.h>
#include <HouseClass.h>

// After all TypeData (unit/infantry/SW types) is loaded, (re)parse our
// [CrateGood.Ext_N] registry from the rules INI. Verified against Phobos source:
//   0x679CAF = RulesClass load, after-type-data; CCINIClass* is in ESI (size 0x5).
DEFINE_HOOK(0x679CAF, CrateGoodExt_RulesData_LoadAfterTypeData, 0x5)
{
	GET(CCINIClass*, pINI, ESI);
	CrateGoodExt::LoadRegistry(pINI);
	return 0;
}

// CellClass::CollectCrate, the CRATE_UNIT result. By 0x4821FA the engine has its
// chosen unit type in EDI (reached by BOTH the UnitCrateType path — jne 0x4821FA —
// and the random-CrateGoodie loop). this(CellClass*) is held in ESI; the collector
// (FootClass*) is arg1 at [ebp+0x8]. Verified via objdump; see docs/ADDRESSES.md.
//
// If a Unit-category member wins the synced inner draw, we override EDI with our
// first unit (so the engine's own create/place/return-false runs unchanged) and
// self-spawn the rest of the bag. Members with no unit fall through to stock (M3
// will handle infantry/SW/addon-only members with clean vanilla suppression).
DEFINE_HOOK(0x4821FA, CrateGoodExt_CollectCrate_UnitResult, 0x6)
{
	GET(CellClass*, pCell, ESI);
	GET_BASE(FootClass*, pCollector, 0x8);
	if (!pCollector)
		return 0;

	const CrateGoodExt::MemberConfig* pMember = CrateGoodExt::PickMember(CrateGoodExt::Category::Unit);
	if (!pMember || !pMember->HasUnits())
		return 0;   // v1: only unit-bearing members participate

	UnitTypeClass* pFirst = nullptr;
	if (CrateGoodExt::ApplyUnitMember(*pMember, pCell, pCollector->Owner, &pFirst) && pFirst)
		R->EDI(pFirst);   // engine's own creation now makes our first unit

	return 0;
}
