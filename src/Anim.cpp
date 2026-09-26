#include "Anim.h"
#include "Ini.h"
#include "Log.h"

#include <AnimClass.h>
#include <AnimTypeClass.h>
#include <CellClass.h>
#include <CCINIClass.h>
#include <GeneralStructures.h>    // CellStruct
#include <GeneralDefinitions.h>   // Powerup

#include <unordered_map>
#include <cstring>

namespace CrateGoodExt::Anim
{
	struct Config
	{
		bool    enabled = false;
		Powerup powerup = Powerup::Money;
	};

	static std::unordered_map<AnimTypeClass*, Config> g_cfgs;

	static Powerup ParsePowerup(const char* s)
	{
		struct Entry { const char* name; Powerup value; };
		static const Entry table[] = {
			{ "Money",     Powerup::Money },    { "Unit",      Powerup::Unit },
			{ "HealBase",  Powerup::HealBase }, { "Cloak",     Powerup::Cloak },
			{ "Explosion", Powerup::Explosion },{ "Napalm",    Powerup::Napalm },
			{ "Darkness",  Powerup::Darkness }, { "Reveal",    Powerup::Reveal },
			{ "Armor",     Powerup::Armor },    { "Speed",     Powerup::Speed },
			{ "Firepower", Powerup::Firepower },{ "ICBM",      Powerup::ICBM },
			{ "Veteran",   Powerup::Veteran },  { "Tiberium",  Powerup::Tiberium },
		};
		for (const Entry& e : table)
			if (_stricmp(s, e.name) == 0)
				return e.value;
		return Powerup::Money;
	}

	static const Config& GetConfig(AnimTypeClass* pType)
	{
		auto it = g_cfgs.find(pType);
		if (it != g_cfgs.end())
			return it->second;

		Config cfg;
		CCINIClass* pINI = CCINIClass::INI_Rules;
		if (pINI && pType)
		{
			const char* section = pType->ID;
			cfg.enabled = pINI->ReadBool(section, "SpawnsCrate", false);
			if (cfg.enabled)
			{
				char buf[64];
				pINI->ReadString(section, "SpawnsCrate.Powerup", "Money", buf, sizeof(buf));
				cfg.powerup = ParsePowerup(Ini::Trim(buf).c_str());
			}
		}
		return g_cfgs.emplace(pType, cfg).first->second;
	}

	// MapClass::PlacePowerupCrate(CellStruct, Powerup) is not declared in this YRpp.
	// thiscall @ 0x56BEC0, this = &MapClass::Instance (0x87F7E8, per objdump).
	static bool PlacePowerupCrate(const CellStruct& cell, Powerup type)
	{
		using Fn = bool(__thiscall*)(void*, CellStruct, Powerup);
		return reinterpret_cast<Fn>(0x56BEC0)(reinterpret_cast<void*>(0x87F7E8), cell, type);
	}

	void OnAnimEnd(AnimClass* pThis)
	{
		if (!pThis || !pThis->Type)
			return;

		const Config& cfg = GetConfig(pThis->Type);
		if (!cfg.enabled)
			return;

		CellClass* pCell = pThis->GetCell();
		if (!pCell)
			return;

		const CellStruct cell = pCell->MapCoords;
		const bool ok = PlacePowerupCrate(cell, cfg.powerup);
		Log("[CrateGoodExt] anim %s ended -> crate (powerup=%d) at (%d,%d) ok=%d",
			pThis->Type->ID, (int)cfg.powerup, (int)cell.X, (int)cell.Y, ok ? 1 : 0);
	}
}
