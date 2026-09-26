# CrateGoodExt — Testing

CI only *compiles* the DLL; the hooks' behavior must be checked in-game. This
covers **M2** (custom weighted **unit** crate goodies).

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

[CrateGood.Ext_0]
Category=Unit
Weight=10                 ; common: a single Apocalypse
Unit.Types=APOC
Unit.Counts=1

[CrateGood.Ext_1]
Category=Unit
Weight=5                  ; rarer: a bag of 3 Rhinos + 2 Conscripts
Unit.Types=RHINO
Unit.Counts=3
Infantry.Types=E1
Infantry.Counts=2
```

After load, `CrateGoodExt.log` should report the parse:

```
[CrateGoodExt] LoadRegistry: 2 Unit member(s), weight sum 15; globals VanillaWeight=0 ...
```

## 3. Trigger a unit crate

The hook fires only when a crate's result is the **Unit** result. To see it often,
raise the `Unit` share in `[Powerups]`.

> ⚠ If you write a `[Powerups]` section you must list **every** result you want to
> keep — a partial section zeroes the shares of everything it omits. Otherwise just
> collect crates until a unit one lands.

On collecting a unit crate you should see, in `CrateGoodExt.log`:

```
[CrateGoodExt] Ext_0 fired: engine makes APOC + 0/0 extra spawned at (x,y).
```

and the goodie appears at the crate for your house — either the single APOC, or
the Rhino+Conscript bag (the engine makes the first Rhino, we spawn the other 2
Rhinos + 2 Conscripts).

## v1 scope

- ✅ Unit-bearing members: synced weighted pick → spawn units + infantry bag.
- ⬜ Infantry-only / SW-only / Explosion-only members: **M3** (need clean vanilla
  suppression via the verified exit path).
- Rarity = `Weight` within the Unit category. `Unit.VanillaWeight` keeps (or, at 0,
  removes) the stock random-unit outcome as an implicit member.
