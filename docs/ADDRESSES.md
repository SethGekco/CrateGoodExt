# CrateGoodExt — Engine Address Map (crate subsystem)

`gamemd.exe` image base `0x400000`. Provenance tags: **[P]** verified Phobos hook source, **[Y]** YRpp declaration, **[O]** OpenYR `cell.cpp` reimplementation (`Goodie_Check`), **[D]** gamemd.exe objdump this session.

## Core function

- **`CellClass::CollectCrate(FootClass* pCollector)` @ `0x481A00`** [Y][O][D] — YR's crate collection + result routine (Westwood name `Goodie_Check`). `thiscall`: `ECX = this` at entry, **saved to `ESI` for the whole body** (`mov esi,ecx` in prologue); `pCollector` = `EDI` at entry, reloaded as `[ebp+0x8]` throughout; `[pCollector+0x21C] = HouseClass*`; frame locals `[esp+0x13]` (bool) and `[esp+0x14]` (force_money int). Prologue confirms the function: reads `OverlayTypeClass::IsCrate` @ `[type+0x2AA]` and bails if not a crate. All hooks below live inside it.

## Inner points inside CollectCrate — [P] `Phobos/src/Misc/Hooks.Crates.cpp`

| Addr | Size | What | Regs |
|---|---|---|---|
| `0x481BB8` | 6 | FreeMCV / MCV override | `HouseClass* EDI`; `UnitTypeClass* BaseUnit` @ stack `STACK_OFFSET(0x188,-0x138)` |
| `0x481C27` | 5 | Unit→money vehicle cap (`CurUnits>50`) | `HouseClass* EDX` |
| `0x4821BD` | 6 | **Unit-pick `CrateGoodie` test** | `UnitTypeClass* EDI`; reads `CrateGoodie` @ `[EDI+0xE0D]` |
| `0x481F9D` | 8 | Reveal-map result | `FootClass*` @ base `[ebp+0x8]` |

Related: `MapClass::PlaceRandomCrate` sampling hook @ `0x56BD8B` [P].

## Unit-result (`CRATE_UNIT`) pick — the M2 target — [D]

Random-pick loop ~`0x482145`–`0x482210`:
- `RulesClass::Instance` global @ **`0x8871E0`**; `UnitCrateType` @ `+0x1148` (if set → used directly, `jne 0x4821FA`); `BaseUnit` list @ `+0xB24` (items) / `+0xB30` (count).
- else `Random_Pick(0, count-1)` (`call 0x65C7E0`) over `UnitTypeClass::Array` data @ `[0xA83CE4]`, count @ `[0xA83CF0]`; ownable check `call 0x50B730`; `CrateGoodie` test @ `0x4821BD`.
- Create chosen type via virtual `[type+0x8C]` @ `0x482205`.
- No-unit → pay-money fallback path @ `0x4832F5`.
- Crate-enabled/lottery bytes seen: `0xA8B258`, `0xA8B261` (`Unsorted::Crates`) [Y].

**M2 hook (pinned, CI-compiled): `0x4821FA` (size 6)** — the choke point where the chosen unit type is in `EDI` (both the `UnitCrateType` `jne 0x4821FA` path and the random-`CrateGoodie` loop fall through here). `this`(CellClass*) = `ESI`; collector(FootClass*) = `[ebp+0x8]`; `[collector]->Owner` = house. Strategy: **EDI-override** — on a Unit-member hit, set `EDI` to our first unit (the engine's own create/place/`return false` runs unchanged) and self-spawn the rest of the bag; `nullptr` → stock. Overwritten instr `mov eax,[ebx+0x21c]` re-executes via `return 0`. Does not overlap Phobos's `0x4821BD`. v1 = unit-bearing members; infantry/SW/addon-only → M3 (clean suppression via the verified exit).

## Placement primitives (M4 chaining / M5 anim)

- Replacement on pickup [O] `cell.cpp:3606-3611`: `Map.Remove_Crate(...)` then `Map.Place_Random_Crate()` (gated on MP + `IsMPCrates` + Goodies option).
- `MapClass::PlacePowerupCrate(CellStruct, Powerup)` — targeted placement (Phobos `WarheadType SpawnsCrate` uses it). Address: **TBD from YRpp `MapClass.h`**.

## Outer-draw data (NOT modified — outer draw stays vanilla per design) — [Y] `Powerups.h`

19-entry arrays: `Weights@0x81DA8C`, `Effects@0x7E523C`, `Arguments@0x89EC28`, `Naval@0x89ECC0`, `Anims@0x81DAD8`.

## Flags / enums

- `UnitTypeClass::CrateGoodie` @ offset `0xE0D` (bool) [P][D].
- `enum class Powerup` (`GeneralDefinitions.h:1017`) [Y]; `RulesClass::{SilverCrate,WoodCrate,WaterCrate,UnitCrateType}` [Y].

## Conflict status (encyclopedia deconfliction)

- **Phobos** hooks `0x481BB8 / 0x481C27 / 0x4821BD / 0x481F9D / 0x56BD8B`. My M2 hook targets the `CRATE_UNIT` case entry (a distinct offset). Syringe chains per address, so even a shared point coexists (no shared state), per the GiftBoxHost model.
- **Antares/Ares**: crate hooks **unconfirmed** (local clone was empty). Verify against real Antares source before shipping.
- **Encyclopedia**: crate subsystem still undocumented there → this map is the basis for the crate page to contribute once M2 is verified in-game.
