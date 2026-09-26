#include "Spawn.h"

#include <TechnoTypeClass.h>
#include <TechnoClass.h>
#include <HouseClass.h>
#include <ScenarioClass.h>
#include <Unsorted.h>
#include <GeneralDefinitions.h>   // DirType

namespace CrateGoodExt::Spawn
{
	int PlaceListAt(const std::vector<TechnoTypeClass*>& types, HouseClass* pHouse,
		const CoordStruct& base, int range)
	{
		int ok = 0;
		for (TechnoTypeClass* pType : types)
		{
			if (!pType)
				continue;

			TechnoClass* pObj = static_cast<TechnoClass*>(pType->CreateObject(pHouse));
			if (!pObj)
				continue;

			const int tries = range > 0 ? 8 : 0;
			bool placed = false;
			for (int t = 0; t <= tries && !placed; ++t)
			{
				CoordStruct c = base;
				if (t < tries)   // last iteration always uses the base cell
				{
					c.X += (ScenarioClass::Instance->Random.RandomRanged(0, 2 * range) - range) * 256;
					c.Y += (ScenarioClass::Instance->Random.RandomRanged(0, 2 * range) - range) * 256;
				}

				// Bypass the "is this placement legal" veto the same way the engine's
				// own crate-unit placement and GiftBoxHost do.
				++Unsorted::IKnowWhatImDoing;
				placed = pObj->Unlimbo(c, DirType::East);
				--Unsorted::IKnowWhatImDoing;

				if (placed)
					pObj->SetLocation(c);
			}

			if (placed)
				++ok;
			// else: object remains in limbo (very rare — base-cell fallback nearly
			// always succeeds); left as-is to match GiftBoxHost's proven behavior.
		}
		return ok;
	}
}
