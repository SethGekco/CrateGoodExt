// Syringe hooks for CrateGoodExt. Feature logic lives in CrateGood.cpp / Anim.cpp.
//
// Addresses here are also hooked by Phobos/Antares; Syringe chains multiple hooks
// per address and we keep no shared framework state, so this coexists cleanly
// (the same model GiftBoxHost relies on).
#include "CrateGood.h"
#include "Anim.h"

#include <Helpers/Macro.h>   // DEFINE_HOOK, GET, GET_BASE, R
#include <CCINIClass.h>
#include <UnitTypeClass.h>
#include <CellClass.h>
#include <FootClass.h>
#include <HouseClass.h>
#include <AnimClass.h>

// After all TypeData (unit/infantry/SW types) is loaded, (re)parse our
// [CrateGood.Ext_N] registry from the rules INI. Verified against Phobos source:
//   0x679CAF = RulesClass load, after-type-data; CCINIClass* is in ESI (size 0x5).
DEFINE_HOOK(0x679CAF, CrateGoodExt_RulesData_LoadAfterTypeData, 0x5)
{
	GET(CCINIClass*, pINI, ESI);
	CrateGoodExt::LoadRegistry(pINI);
	return 0;
}

// CellClass::CollectCrate, the CRATE_UNIT result. By 0x4821FA the chosen unit type
// is in EDI (both the UnitCrateType path — jne 0x4821FA — and the random-CrateGoodie
// loop reach it). this(CellClass*) is held in ESI; the collector (FootClass*) is
// arg1 at [ebp+0x8]. Verified via objdump; see docs/ADDRESSES.md.
//
// When a Unit-category member wins the synced inner draw we apply its payload:
//   - unit-bearing member -> override EDI with our first unit; the engine's own
//     create/place/return-false runs unchanged, and we self-spawn the rest.
//   - member with no unit  -> fully applied here, then jump to 0x4832F5, the engine's
//     "no unit -> play the crate anim -> return true" exit. No vanilla unit is made
//     and no money is paid (the crate overlay was already removed earlier in the fn).
DEFINE_HOOK(0x4821FA, CrateGoodExt_CollectCrate_UnitResult, 0x6)
{
	enum { SuppressToNoUnitExit = 0x4832F5 };

	GET(CellClass*, pCell, ESI);
	GET_BASE(FootClass*, pCollector, 0x8);
	if (!pCollector)
		return 0;

	const CrateGoodExt::MemberConfig* pMember = CrateGoodExt::PickMember(CrateGoodExt::Category::Unit);
	if (!pMember)
		return 0;   // no member won the inner draw -> stock behavior

	UnitTypeClass* pFirst = nullptr;
	if (CrateGoodExt::ApplyMember(*pMember, pCell, pCollector->Owner, &pFirst))
	{
		R->EDI(pFirst);   // engine's own creation now makes our first unit
		return 0;
	}

	return SuppressToNoUnitExit;   // self-contained payload -> suppress the vanilla unit
}

// CellClass::CollectCrate replacement gate. Right after Remove_Crate (0x481D97),
// vanilla places a replacement crate only when NOT a campaign (Session.Type @
// 0xA8B238 != 0) AND crates are enabled (Unsorted::Crates @ 0xA8B261). With
// [CrateGoodExt]Chain.Default=yes we force the replacement on EVERY pickup (all
// modes, including unit-dropped crates) by jumping straight to the PlaceRandomCrate
// call site, bypassing both gate checks. This point runs before the effect switch,
// so it fires for every crate result. Verified via objdump; see docs/ADDRESSES.md.
DEFINE_HOOK(0x481D9C, CrateGoodExt_CollectCrate_ForceReplacement, 0x5)
{
	enum { ForcePlaceRandomCrate = 0x481DAE };   // mov ecx,&Map; call 0x56BD40

	if (CrateGoodExt::GetGlobals().chainDefault)
		return ForcePlaceRandomCrate;

	return 0;   // Chain.Default=no -> vanilla gated behavior
}

// AnimClass::AI end-effects point (where MakeInfantry/CreateUnit fire once as an anim
// completes). AnimClass* in ESI. Additive (return 0): if the anim's type has
// SpawnsCrate=yes, drop a crate on its cell, then let the vanilla end-effects run.
// NOTE: Phobos also hooks 0x424932 (its CreateUnit) and redirects control flow; with
// Phobos ALSO loaded, hook order may skip ours. Targets Antares. See docs/ADDRESSES.md.
DEFINE_HOOK(0x424932, CrateGoodExt_AnimClass_AI_SpawnsCrate, 0x6)
{
	GET(AnimClass* const, pThis, ESI);
	CrateGoodExt::Anim::OnAnimEnd(pThis);
	return 0;
}
