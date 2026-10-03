#pragma once

// FUCK_API.h (FLICK) calls GetModuleHandleW/GetProcAddress unqualified, so it needs windows.h; LoadImage would be
// renamed to LoadImageW by its macro in this translation unit only, so it is dropped before the API header.
#pragma warning(push, 0)
#include <Windows.h>
#undef LoadImage
#include <FUCK_API.h>
#pragma warning(pop)
