// Spawn — place a list of techno types at a world coordinate for a house.
// Adapted from GiftBoxHostExt's proven CreateObject + Unlimbo placement. All
// randomness uses the game's synchronized RNG, so it is netplay-safe.
#pragma once

#include <GeneralStructures.h>   // CoordStruct
#include <vector>

class TechnoTypeClass;
class HouseClass;

namespace CrateGoodExt::Spawn
{
	// Create each type for pHouse and place it near `base`, scattered up to
	// `range` cells (0 = base cell only), preferring clear cells and falling back
	// to the base cell. Returns how many were successfully placed. An object that
	// cannot be placed anywhere is left in limbo (very rare; matches GiftBoxHost).
	int PlaceListAt(const std::vector<TechnoTypeClass*>& types, HouseClass* pHouse,
		const CoordStruct& base, int range);
}
