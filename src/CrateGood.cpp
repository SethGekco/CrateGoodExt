// CrateGood — parse [CrateGood.Ext_N] members + [CrateGoodExt] globals, and pick a
// member with the game's synced RNG. No engine mutation here; the actual spawning
// / SW grant / addon effects arrive in later milestones. See docs/DESIGN.md.
#include "CrateGood.h"
#include "Ini.h"
#include "Log.h"

#include <CCINIClass.h>
#include <ScenarioClass.h>

#include <cstdio>
#include <cstring>
#include <utility>

namespace CrateGoodExt
{
	static std::vector<MemberConfig> g_unitMembers;   // Category::Unit members
	static Globals                   g_globals;

	Category ParseCategory(const std::string& s)
	{
		if (_stricmp(s.c_str(), "Unit") == 0)
			return Category::Unit;
		return Category::Unknown;
	}

	const char* CategoryName(Category c)
	{
		switch (c)
		{
		case Category::Unit: return "Unit";
		default:             return "Unknown";
		}
	}

	const std::vector<MemberConfig>& MembersFor(Category cat)
	{
		static const std::vector<MemberConfig> empty;
		switch (cat)
		{
		case Category::Unit: return g_unitMembers;
		default:             return empty;
		}
	}

	const Globals& GetGlobals() { return g_globals; }

	// Pad/truncate counts to match types (missing default to 1; clamp to >= 1).
	static void NormalizeCounts(const std::vector<std::string>& types, std::vector<int>& counts)
	{
		std::vector<int> out;
		out.reserve(types.size());
		for (size_t i = 0; i < types.size(); ++i)
		{
			int c = (i < counts.size()) ? counts[i] : 1;
			if (c < 1) c = 1;
			out.push_back(c);
		}
		counts.swap(out);
	}

	static bool ParseMember(CCINIClass* pINI, int index, MemberConfig& m)
	{
		char section[64];
		std::snprintf(section, sizeof(section), "CrateGood.Ext_%d", index);
		if (pINI->GetKeyCount(section) <= 0)
			return false;

		m.index = index;
		char buf[256];

		pINI->ReadString(section, "Category", "Unit", buf, sizeof(buf));
		m.category = ParseCategory(Ini::Trim(buf));
		const bool badCategory = (m.category == Category::Unknown);
		if (badCategory)
			m.category = Category::Unit;   // safe default

		m.weight = pINI->ReadInteger(section, "Weight", 1);
		if (m.weight < 0) m.weight = 0;

		pINI->ReadString(section, "Unit.Types", "", buf, sizeof(buf));
		m.unitTypes = Ini::SplitList(buf);
		pINI->ReadString(section, "Unit.Counts", "", buf, sizeof(buf));
		m.unitCounts = Ini::SplitInts(buf);
		NormalizeCounts(m.unitTypes, m.unitCounts);

		pINI->ReadString(section, "Infantry.Types", "", buf, sizeof(buf));
		m.infantryTypes = Ini::SplitList(buf);
		pINI->ReadString(section, "Infantry.Counts", "", buf, sizeof(buf));
		m.infantryCounts = Ini::SplitInts(buf);
		NormalizeCounts(m.infantryTypes, m.infantryCounts);

		pINI->ReadString(section, "SuperWeapons", "", buf, sizeof(buf));
		m.superWeapons = Ini::SplitList(buf);

		m.explosion = pINI->ReadBool(section, "Explosion", false);
		m.explosionDamage = pINI->ReadInteger(section, "Explosion.Damage", 0);
		pINI->ReadString(section, "Explosion.Warhead", "", buf, sizeof(buf));
		m.explosionWarhead = Ini::Trim(buf);

		if (badCategory)
			Log("[CrateGoodExt] [%s]: unknown Category; defaulted to Unit.", section);

		return true;
	}

	void LoadRegistry(CCINIClass* pINI)
	{
		g_unitMembers.clear();
		g_globals = Globals{};

		if (!pINI)
			pINI = CCINIClass::INI_Rules;
		if (!pINI)
		{
			Log("[CrateGoodExt] LoadRegistry: no rules INI available; skipped.");
			return;
		}

		g_globals.unitVanillaWeight = pINI->ReadInteger("CrateGoodExt", "Unit.VanillaWeight", 0);
		g_globals.chainDefault      = pINI->ReadBool("CrateGoodExt", "Chain.Default", false);
		g_globals.chainMaxDepth     = pINI->ReadInteger("CrateGoodExt", "Chain.MaxDepth", 3);
		if (g_globals.unitVanillaWeight < 0) g_globals.unitVanillaWeight = 0;
		if (g_globals.chainMaxDepth < 0)     g_globals.chainMaxDepth = 0;

		// Enumerate [CrateGood.Ext_N] from 0. Tolerate small gaps so commenting one
		// out mid-list doesn't silently truncate the rest.
		int misses = 0;
		const int kMaxGap = 8;
		for (int i = 0; misses <= kMaxGap && i < 100000; ++i)
		{
			MemberConfig m;
			if (!ParseMember(pINI, i, m))
			{
				++misses;
				continue;
			}
			misses = 0;

			if (m.IsEmpty())
			{
				Log("[CrateGoodExt] [CrateGood.Ext_%d]: no payload; ignored.", i);
				continue;
			}
			g_unitMembers.push_back(std::move(m));   // only Unit is wired for now
		}

		int wsum = 0;
		for (const auto& m : g_unitMembers) wsum += m.weight;
		Log("[CrateGoodExt] LoadRegistry: %d Unit member(s), weight sum %d; "
			"globals VanillaWeight=%d Chain.Default=%d Chain.MaxDepth=%d.",
			(int)g_unitMembers.size(), wsum, g_globals.unitVanillaWeight,
			g_globals.chainDefault ? 1 : 0, g_globals.chainMaxDepth);

		for (const auto& m : g_unitMembers)
			Log("[CrateGoodExt]   Ext_%d: weight=%d units=%d infantry=%d sw=%d explosion=%d",
				m.index, m.weight, (int)m.unitTypes.size(), (int)m.infantryTypes.size(),
				(int)m.superWeapons.size(), m.explosion ? 1 : 0);
	}

	const MemberConfig* PickMember(Category cat)
	{
		const std::vector<MemberConfig>& members = MembersFor(cat);

		int vanillaWeight = (cat == Category::Unit) ? g_globals.unitVanillaWeight : 0;
		if (vanillaWeight < 0) vanillaWeight = 0;

		int total = vanillaWeight;
		for (const auto& m : members) total += m.weight;
		if (total <= 0)
			return nullptr;   // nothing registered -> stock behavior

		// Network-synced RNG ONLY (RandomRanged is inclusive on both ends).
		int roll = ScenarioClass::Instance->Random.RandomRanged(0, total - 1);

		if (roll < vanillaWeight)
			return nullptr;   // implicit vanilla member won -> stock behavior
		roll -= vanillaWeight;

		for (const auto& m : members)
		{
			if (roll < m.weight)
				return &m;
			roll -= m.weight;
		}
		return nullptr;
	}
}
