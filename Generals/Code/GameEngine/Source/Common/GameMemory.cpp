#include "PreRTS.h"
#include "Common/GameMemory.h" // Include the header to keep the declarations consistent

// Definitions of the placement new overloads (no inline here)
// [DX9] unsure usage. Might no longer needed after include PreRTS.h
//void* __cdecl operator new[](size_t s, void* p) { return p; }
//void __cdecl operator delete[](void*, void* p) {}