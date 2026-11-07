// cxx.cpp - minimal C++ runtime for PlusOS: new/delete on the ROM heap.
// Build C++ code with -fno-exceptions -fno-rtti -fno-threadsafe-statics.
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void *operator new(size_t n)                    { return malloc(n ? n : 1); }
void *operator new[](size_t n)                  { return malloc(n ? n : 1); }
void  operator delete(void *p) noexcept         { free(p); }
void  operator delete[](void *p) noexcept       { free(p); }
void  operator delete(void *p, size_t) noexcept { free(p); }
void  operator delete[](void *p, size_t) noexcept { free(p); }

// placement new
void *operator new(size_t, void *where) noexcept   { return where; }
void *operator new[](size_t, void *where) noexcept { return where; }

extern "C" void __cxa_pure_virtual()
{
    puts("\n*** pure virtual function called ***");
    abort();
}

extern "C" void __cxa_deleted_virtual()
{
    puts("\n*** deleted virtual function called ***");
    abort();
}
