// Stub atlbase.h
// YRpp's Interfaces.h #includes <atlbase.h>, but only needs the COM basics
// (IUnknown / IStream / GUID). We don't use ATL at all, so this stub avoids the
// heavyweight ATL component (and its atls.lib link dependency) entirely.
#pragma once
#include <windows.h>
#include <ocidl.h>
