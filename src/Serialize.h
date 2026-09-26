// Minimal save-stream helpers. We persist our per-unit maps by appending to the
// savegame's object stream, writing raw TechnoClass* values and swizzling them
// back to the relocated objects on load (the game's standard pointer-fixup).
#pragma once

#include <windows.h>              // IStream
#include <SwizzleManagerClass.h>

namespace CrateGoodExt::Serialize
{
	template <class T>
	inline void Write(IStream* s, const T& v)
	{
		ULONG n = 0;
		s->Write(&v, sizeof(T), &n);
	}

	template <class T>
	inline bool Read(IStream* s, T& v)
	{
		ULONG n = 0;
		return s->Read(&v, sizeof(T), &n) == S_OK && n == sizeof(T);
	}

	inline void WritePtr(IStream* s, void* p) { Write(s, p); }

	// Reads a saved pointer and remaps it to the relocated object. Returns nullptr
	// if the read fails or the pointer can't be resolved.
	inline void* ReadSwizzled(IStream* s)
	{
		void* p = nullptr;
		if (!Read(s, p) || !p)
			return nullptr;
		SwizzleManagerClass::Instance->Swizzle(&p);
		return p;
	}
}
