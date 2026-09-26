# CrateGoodExt — Design

A standalone [Syringe](https://github.com/Ares-Developers/Syringe) DLL for **Command & Conquer: Yuri's Revenge** that makes crate goodies fully customizable. Loads **alongside Antares** (the open-source Ares 3.0p1 reimplementation) — not Ares — but **never depends on it**. Mirrors the `GiftBoxHostExt` standalone scaffold: **YRpp** + a hand-rolled Ext/Ini/Log/Serialize layer, built by GitHub Actions. No Phobos or Ares runtime dependency.

> Status: **foundation / blueprint.** No engine hooks written yet. Addresses are TBD (see Deconfliction).

---

## 1. Goals

1. **Superweapons as a crate payload** — grant one or more configurable SWTypes to the collector's house.
2. **Indexed custom goodies with individual probability** — any number of `[CrateGood.Ext_N]` *members* that join a crate-result category with their own rarity `Weight`, each carrying an independent payload (units / infantry / SW / addon). Per-member rarity replaces the vanilla uniform pick; the outer category draw is untouched.
3. **Pickup chaining** — a collected crate can place another crate, including crates dropped by `CarriesCrate=yes` vehicles.
4. **Crates from animations** — an AnimType can drop a crate on its cell when it ends.

---

## 2. Grounding — the vanilla crate result flow

Source of truth: **OpenYR `manual/content/systems/crates.md`** (reverse-engineered from EA's GPL source; treat as verified) and `code/crate.*`. Condensed pipeline, **outside a campaign**:

1. **Placement** — random cell, up to 256 tracking slots, lifetime drawn from `CrateRegen`. Also: map-authored crates, and `CarriesCrate=yes` vehicles dropping a crate on death.
2. **Collection** — a walking / mech / hover / driving locomotor commits to the crate's cell.
3. **CrateTrigger** — if set, springs `TEVENT_PICKUP_CRATE` before the result is chosen.
4. **Weighted (outer) draw** — every `[Powerups]` share is summed; a synced random `1..sum` walks the results in fixed order. *Shares are absolute weights, recomputed live each collection.*
5. **MCV override** — replaces the draw with the unit result when: no buildings + >1500 credits + no `BaseUnit` vehicle + bases enabled.
6. **Money conversions** — `Unit` (>50 vehicles), `Squad` (always), `Armor`/`Speed`/`Firepower`/`Cloak` (collector already boosted) convert to money.
7. **Result handler** runs.
8. **Unit pick order (inner)** — `BaseUnit` rescue → `HarvesterUnit` rescue → `UnitCrateType` → random `CrateGoodie=yes` UnitType (redraw until ownable + base-legal). **This uniform inner pick is what we replace with a weighted member draw.**
9. **Result animation** placed (from the result that actually ran).
10. **Replacement crate** placed when the match Crates option **and** `[MultiplayerDefaults]Crates` are both enabled.

### Feature → insertion point

| Feature | Hook target (behavioral) | Notes |
|---|---|---|
| Weighted member draw (units/infantry/addons, per-member rarity) | step **8** — the *inner* pick of a result category | Replace the vanilla uniform pick with a weighted member draw; the outer `[Powerups]` category draw stays untouched |
| Superweapons | SW-grant payload module, invoked by a selected member (optionally also generalizes the `Missile` handler) | Independent of the unit/infantry payload |
| Pickup chaining | step **10** (+ the `CarriesCrate` drop path) | Extend replacement to always/config; **guard depth** |
| Anim-sourced crates | `AnimClass` expire/end → step **1** placement routine | Reuse the wood-crate placement + legality test |

---

## 3. INI schema — two-tier weighted goodies

The vanilla **outer draw is untouched**: the `[Powerups]` category shares (Money, Unit, Armor, Explosion, …) keep working exactly as-is, and a category with no custom members behaves 100% vanilla. What we add is a **second, inner draw**: when the outer draw lands on a category we've extended, we pick *which specific thing happens* from a weighted member list — so members have individual rarity instead of the vanilla uniform pick.

Each `[CrateGood.Ext_N]` is a **member**: it joins one category with a `Weight` (its rarity inside that category) and carries an independent **payload**. Payloads compose freely and never depend on each other — a member can spawn units, spawn infantry, grant SWs, or fire an addon effect, in any combination or alone.

```ini
; ---- A member of the Unit category: a specific unit, made rare ----
[CrateGood.Ext_0]
Category=Unit            ; which vanilla crate result's inner pick to join
Weight=3                 ; rarity within that category (relative to sibling members)
Unit.Types=APOC          ; payload: spawn these vehicles ...
Unit.Counts=1            ; ... this many each (parallel list; default 1)

; ---- Another Unit member: common basic tank ----
[CrateGood.Ext_1]
Category=Unit
Weight=10
Unit.Types=RHINO
Unit.Counts=1

; ---- An "addon" member in the SAME category: an explosive, not a unit ----
[CrateGood.Ext_2]
Category=Unit            ; recycles the Unit category with a new addon
Weight=1                 ; make it rare
Explosion=yes            ; payload is independent of unit spawning
Explosion.Damage=500
Explosion.Warhead=HE

; ---- A member that grants superweapons (independent payload, no unit needed) ----
[CrateGood.Ext_3]
Category=Unit
Weight=1
SuperWeapons=NUKESPECIAL ; grant one-time SWs to the collector's house

; ---- Members can mix payloads too: a bag of units + infantry ----
[CrateGood.Ext_4]
Category=Unit
Weight=2
Unit.Types=RHINO,APOC
Unit.Counts=2,1
Infantry.Types=E1,GGI
Infantry.Counts=3,2
```

Payloads (all optional, all independent):
- `Unit.Types` / `Unit.Counts` — spawn vehicles (counts default 1; parallel lists).
- `Infantry.Types` / `Infantry.Counts` — spawn infantry.
- `SuperWeapons` — grant one-time SWTypes to the collector's house.
- `Explosion=` (+ `Explosion.Damage`, `Explosion.Warhead`, …) — an addon effect; more addon types to follow.

Global + other surfaces:

```ini
[CrateGoodExt]
Unit.VanillaWeight=0      ; weight of the ordinary vanilla unit pick as an implicit
                          ; member of the Unit category (0 = replaced entirely)
Chain.Default=no          ; place a replacement crate on any pickup by default
Chain.MaxDepth=3          ; hard guard against infinite chains

[SOMEANIM]                ; anim-sourced crates (on an AnimType)
SpawnsCrate=yes           ; drop a wood crate on this anim's cell when it ends
; SpawnsCrateGood=0        ; (optional) force a specific member/category at that crate
```

### Why this shape
- **Outer and inner draws stay independent** — vanilla category weighting is unchanged; our inner draw only engages for categories that have members. Either works without the other.
- **Payloads stay independent** — SW-granting, unit-spawning, infantry-spawning and addons never require one another; a goodie can be any one alone.
- **Categories are reusable** — drop new members (units or addons) into a category and tune their `Weight` to control rarity, without touching the outer draw.

> `Category=` starts with **Unit** (clearest inner pick to hook). Extending to other result categories, and letting one member join several categories, are straightforward follow-ons.

---

## 4. Architecture (mirror GiftBoxHostExt)

```
CrateGoodExt/
  CrateGoodExt.sln
  CrateGoodExt.vcxproj          ; MSVC v142, Win10 SDK, /LD -> CrateGoodExt.dll
  .github/workflows/build.yml   ; CI build (mirror GiftBoxHostExt)
  .gitmodules                   ; YRpp submodule
  include/atlbase.h             ; stub for YRpp Interfaces.h (no ATL)
  YRpp/                         ; submodule
  src/
    Main.cpp                    ; DLL entry / Syringe glue
    Hooks.cpp                   ; DEFINE_HOOK sites (inner unit pick, chain, anim)
    CrateGood.h / .cpp          ; the [CrateGood.Ext_N] member type + registry (M1)
    Ini.h                       ; INI read helpers (mirror)
    Log.h                       ; logging (mirror)
    Serialize.h / SaveLoad.cpp  ; save/load (mirror)
  docs/
    DESIGN.md                   ; this file
    TAGS.md                     ; INI reference for modders
```

Registry model: `CrateGood.cpp` parses every `[CrateGood.Ext_*]` section into a `Member` { category, weight, payload } and groups members by category. The inner-pick hook, when it fires for a hooked category, runs a synced weighted draw over that category's member list and executes the winning member's payload.

---

## 5. Hard constraints

- **Multiplayer determinism.** All draws (inner member draw, chain chance, any randomness) **must** use the game's network-synced RNG (`ScenarioClass` random / `Random2`), never `rand()` or any unsynced source. A single unsynced draw desyncs the match.
- **Independence.** CrateGoodExt must build and run with **no dependency on Antares** (or Ares/Phobos). Feature payloads (SW / unit / infantry / addon) must be independently usable. The outer `[Powerups]` draw must remain fully functional with zero `[CrateGood.Ext_N]` sections present.
- **Save/Load.** The inner draw is stateless (recomputed per collection from rules), so M1–M3 need no new serialized state. Chain depth, if tracked per-crate, must be serialized via the `Serialize.h` pattern.
- **Antares coexistence.** Confirm Antares does not hook the same crate functions before shipping (see Deconfliction). Antares.dll and Ares.dll cannot coexist; we target Antares.
- **Placement legality.** Reuse the engine's own placement/legality path so anim-sourced and chained crates respect terrain/occupancy (avoid the "silent drop on water" trap documented in OpenYR).

---

## 6. Build order / milestones

- **M0 — Scaffold.** Mirror GiftBoxHostExt; empty DLL loads under Syringe alongside Antares; CI green.
- **M1 — Member type + parse (no engine risk).** Parse `[CrateGood.Ext_N]` into the registry, grouped by category. Pure INI + container; log what was parsed.
- **M2 — Inner weighted draw.** Hook the Unit result's inner pick; run the synced weighted member draw; execute unit/infantry payloads. First real behavior.
- **M3 — Superweapons + addons.** SW-grant payload module; Explosion addon. Both independent of the unit payload.
- **M4 — Chaining.** Extend the replacement path; enforce `Chain.MaxDepth`.
- **M5 — Anim-sourced crates.** Hook `AnimClass` end → placement routine.

Each milestone: verify in-game, then log results to the YR-Hook-Encyclopedia.

---

## 7. Deconfliction TODO (encyclopedia workflow)

Consult **before**, contribute **after** — the crate subsystem is currently **undocumented** in the YR-Hook-Encyclopedia, so this project is net-new knowledge to add.

- [ ] **Addresses** — locate in `gamemd.exe` (`/home/rex/gamemd.exe`): crate-result draw, the Unit-result inner pick, Missile/SW grant, replacement placement, `AnimClass` expire. Cross-reference ModEnc + the encyclopedia registry (`registry/hooks.csv`) + objdump.
- [ ] **Antares** — the local `/home/rex/Claude/Antares` clone is **empty**; pull real source and grep for crate/goodie/powerup/unitcrate. GitHub tree shows no Crate/Cell/Anim Ext class, but inline `Misc/` hooks are still possible.
- [ ] **Contribute** — add crate-subsystem page(s) to `encyclopedia/` (copy `_TEMPLATE.md`), grounded in the OpenYR RE and our verified hook results; state how each fact was confirmed.

### Provenance
- Crate flow & mechanics: **OpenYR RE** (EA GPL reimpl) — treat as verified.
- Engine addresses: **TBD** (not yet confirmed this session).
- Antares crate hooks: **GitHub tree inspection only** — source grep pending.
