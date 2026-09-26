// Anim — crates dropped by animations (M5). An AnimType tagged SpawnsCrate=yes drops
// a crate on its own cell when the anim ends. Config is parsed lazily from the rules
// INI and cached per AnimTypeClass.
//
// INI (on the AnimType's rules section):
//   [SOMEANIM]
//   SpawnsCrate=yes           ; drop a crate when this anim ends
//   SpawnsCrate.Powerup=Money ; which crate result (Powerup enum name; default Money)
#pragma once

class AnimClass;

namespace CrateGoodExt::Anim
{
	// Called at the anim end-effects point. If the anim's AnimType has SpawnsCrate=yes,
	// places a crate on the anim's cell; no-op otherwise.
	void OnAnimEnd(AnimClass* pThis);
}
