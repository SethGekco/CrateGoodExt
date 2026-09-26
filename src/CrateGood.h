// CrateGood — standalone customizable crate goodies for Yuri's Revenge.
//
// A [CrateGood.Ext_N] section is a "member": it joins one crate-result CATEGORY
// with a rarity WEIGHT and carries an independent payload (units / infantry /
// superweapons / addon). When the vanilla outer draw lands on a category we've
// extended, we run a synced weighted draw over that category's members and
// execute the winner. The vanilla [Powerups] outer draw is never modified; a
// category with no members behaves exactly as stock.
//
// INI (standalone numbered sections in rulesmd.ini):
//   [CrateGood.Ext_0]
//   Category=Unit             ; which crate result's inner pick to join
//   Weight=10                 ; rarity within that category (relative; >=0)
//   Unit.Types=RHINO,APOC     ; payload: spawn vehicles ...
//   Unit.Counts=2,1           ; ... this many each (parallel; default 1)
//   Infantry.Types=E1,GGI     ; payload: spawn infantry
//   Infantry.Counts=3,2
//   SuperWeapons=NUKESPECIAL  ; payload: grant one-time SWs to the collector
//   Explosion=no              ; addon payload (independent of the above)
//   Explosion.Damage=500
//   Explosion.Warhead=HE
//
//   [CrateGoodExt]            ; global tuning
//   Unit.VanillaWeight=0      ; weight of the stock unit pick as an implicit member
//   Chain.Default=no          ; replacement crate on any pickup
//   Chain.MaxDepth=3          ; hard guard against infinite chains
#pragma once

#include <string>
#include <vector>

class CCINIClass;    // YRpp; handed to LoadRegistry by the rules-load hook
class CellClass;     // the crate's cell (spawn origin)
class HouseClass;    // the collector's house
class UnitTypeClass; // the member's first unit, handed to the engine's own creation

namespace CrateGoodExt
{
	// Which vanilla crate-result category a member joins. Starts with Unit (the
	// stock unit-crate inner pick); more categories are a later extension.
	enum class Category
	{
		Unit,
		Unknown,
	};

	Category    ParseCategory(const std::string& s);   // case-insensitive; Unknown on miss
	const char* CategoryName(Category c);

	// One [CrateGood.Ext_N] member. Payloads are all optional and independent.
	struct MemberConfig
	{
		int      index = -1;                 // N in [CrateGood.Ext_N]
		Category category = Category::Unit;
		int      weight = 1;                 // rarity within its category (>= 0)

		std::vector<std::string> unitTypes;      // Unit.Types
		std::vector<int>         unitCounts;     // Unit.Counts     (parallel; default 1)
		std::vector<std::string> infantryTypes;  // Infantry.Types
		std::vector<int>         infantryCounts; // Infantry.Counts (parallel; default 1)
		std::vector<std::string> superWeapons;   // SuperWeapons    (grant one-time)

		bool        explosion = false;           // Explosion=
		int         explosionDamage = 0;         // Explosion.Damage
		std::string explosionWarhead;            // Explosion.Warhead

		bool HasUnits()     const { return !unitTypes.empty(); }
		bool HasInfantry()  const { return !infantryTypes.empty(); }
		bool HasSW()        const { return !superWeapons.empty(); }
		bool HasAddon()     const { return explosion; }
		bool IsEmpty()      const { return !HasUnits() && !HasInfantry() && !HasSW() && !HasAddon(); }
	};

	// Global tuning from the [CrateGoodExt] section.
	struct Globals
	{
		int  unitVanillaWeight = 0;   // weight of the stock unit pick as an implicit
		                              // member of the Unit category (0 = replace fully)
		bool chainDefault = false;    // place a replacement crate on any pickup
		int  chainMaxDepth = 3;       // hard guard against infinite chains
	};

	// Parse every [CrateGood.Ext_N] section + [CrateGoodExt] globals from the rules
	// INI (falls back to CCINIClass::INI_Rules if pINI is null). Idempotent: safe to
	// call again on a rules reload. No engine mutation — pure read + a log summary.
	void LoadRegistry(CCINIClass* pINI);

	// Members registered for a category (empty => that category behaves as stock).
	const std::vector<MemberConfig>& MembersFor(Category cat);
	const Globals&                   GetGlobals();

	// Synced weighted pick among a category's members (plus the optional implicit
	// vanilla member via Globals::unitVanillaWeight). Returns nullptr to mean
	// "fall through to stock behavior". MUST use the game's network-synced RNG
	// only (ScenarioClass random) — never rand() — or multiplayer desyncs.
	const MemberConfig* PickMember(Category cat);

	// M2: apply a Unit-category member that has >=1 unit. Resolves the member's
	// first unit type into *outFirstUnit (hand to the engine's own single-unit
	// creation via EDI) and spawns the REST of the bag (remaining units + all
	// infantry) at pCrateCell for pHouse on the synced RNG. Returns false (and
	// *outFirstUnit=nullptr) when the first unit type can't be resolved.
	bool ApplyUnitMember(const MemberConfig& m, CellClass* pCrateCell,
		HouseClass* pHouse, UnitTypeClass** outFirstUnit);
}
