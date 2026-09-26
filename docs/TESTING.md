# CrateGoodExt — Testing

CI only *compiles* the DLL; the hooks' behavior must be checked in-game. This
covers **M2** (unit goodies) and **M3** (infantry / superweapon / explosion goodies).

## 1. Load the DLL

Put `CrateGoodExt.dll` in your YR folder and let Syringe load it the same way you
load `GiftBoxHost.dll`. On boot it writes `CrateGoodExt.log` next to the game; the
first line confirms it's live:

```
[CrateGoodExt] loaded; boot hook fired, DLL is live.
```

## 2. Add test goodies to rulesmd.ini

```ini
[CrateGoodExt]
Unit.VanillaWeight=0      ; 0 = unit crates always use our members (no vanilla unit)

; --- M2: units ---
[CrateGood.Ext_0]
Category=Unit
Weight=10                 ; common: a single Apocalypse
Unit.Types=APOC
Unit.Counts=1

[CrateGood.Ext_1]
Category=Unit
Weight=5                  ; a bag: 3 Rhinos + 2 Conscripts
Unit.Types=RHINO
Unit.Counts=3
Infantry.Types=E1
Infantry.Counts=2

; --- M3: infantry-only (no unit) ---
[CrateGood.Ext_2]
Category=Unit
Weight=5                  ; a squad of 5 GIs
Infantry.Types=E1
Infantry.Counts=5

; --- M3: superweapon grant (no unit) ---
[CrateGood.Ext_3]
Category=Unit
Weight=2
SuperWeapons=NukeSpecial  ; one-time nuke for the collector's house

; --- M3: explosion addon (no unit) ---
[CrateGood.Ext_4]
Category=Unit
Weight=2
Explosion=yes
Explosion.Damage=800
Explosion.Warhead=SA      ; omit to default to [General]C4Warhead
```

After load, `CrateGoodExt.log` should report the parse:

```
[CrateGoodExt] LoadRegistry: 5 Unit member(s), weight sum 24; globals VanillaWeight=0 ...
```

## 3. Trigger unit crates

The hook fires only when a crate's result is the **Unit** result. To see it often,
raise the `Unit` share in `[Powerups]`.

> ⚠ If you write a `[Powerups]` section you must list **every** result you want to
> keep — a partial section zeroes the shares of everything it omits. Otherwise just
> collect crates until unit ones land.

On each unit crate you should see one of these in `CrateGoodExt.log`, and the effect:

| Member | Log line | In-game |
| --- | --- | --- |
| Ext_0 / Ext_1 (has units) | `Ext_N fired (unit-handoff): units=… at (x,y)` | vehicle(s) + any infantry appear at the crate |
| Ext_2 (infantry only) | `Ext_2 fired (self-contained): infantry=1 …` | a squad appears; **no** vehicle, **no** money |
| Ext_3 (SW only) | `Ext_3 fired (self-contained): sw=1 …` | one-time superweapon added to your sidebar |
| Ext_4 (explosion) | `Ext_4 fired (self-contained): explosion=1 …` | a blast at the crate cell |

`self-contained` = the vanilla unit was cleanly suppressed (jump to the no-unit
exit): crate consumed, its result animation plays, no unit and no money.

## M4 — perpetual crates (chaining)

```ini
[CrateGoodExt]
Chain.Default=yes         ; force a replacement crate on EVERY pickup (all modes,
                          ; incl. unit-dropped crates), bypassing the vanilla gate
```
With this on, collecting any crate immediately places a fresh random crate — crates
never deplete. Off (default) = vanilla.

## M5 — crates from animations

```ini
[SOMEANIM]                ; any AnimType (its rules section)
SpawnsCrate=yes           ; drop a crate on this anim's cell when it ends
SpawnsCrate.Powerup=Money ; result: Money/Unit/Cloak/Firepower/Armor/Speed/Veteran/
                          ; Explosion/Napalm/Darkness/Reveal/ICBM/Tiberium (default Money)
```
Log on anim end: `anim SOMEANIM ended -> crate (powerup=0) at (x,y) ok=1`.
> Targets Antares. If Phobos is also loaded (it hooks the same anim-end point and
> redirects), the anim-crate drop may not fire.

## Scope

- ✅ **M2** units · **M3** infantry/SW/explosion (+ clean suppression) · **M4**
  perpetual crates · **M5** anim-sourced crates — all built & CI-green; **behavior
  pending in-game test**.
- Rarity = `Weight` within the Unit category. `Unit.VanillaWeight` keeps (or, at 0,
  removes) the stock random-unit outcome as an implicit member.
