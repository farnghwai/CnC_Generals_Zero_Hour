#include "Common/GameMemory.h" // Include the header to keep the declarations consistent

// Definitions of the placement new overloads (no inline here)
void* __cdecl operator new[](size_t s, void* p) { return p; }
void __cdecl operator delete[](void*, void* p) {}