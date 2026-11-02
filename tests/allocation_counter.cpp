#include "allocation_counter.h"

#include <cstdlib>
#include <malloc.h> // _aligned_malloc, _aligned_free
#include <new>

namespace {

// Per thread: a test arms the counter on its own thread only.
thread_local bool counting = false;
thread_local int allocationCount = 0;
thread_local int freeCount = 0;

void* allocate(std::size_t size) {
    if (counting) ++allocationCount;
    return std::malloc(size == 0 ? 1 : size);
}

void* allocateAligned(std::size_t size, std::align_val_t alignment) {
    if (counting) ++allocationCount;
    // MSVC has no std::aligned_alloc; its own aligned memory needs _aligned_free.
    return _aligned_malloc(size == 0 ? 1 : size, static_cast<std::size_t>(alignment));
}

void release(void* p) {
    if (p == nullptr) return;
    if (counting) ++freeCount;
    std::free(p);
}

void releaseAligned(void* p) {
    if (p == nullptr) return;
    if (counting) ++freeCount;
    _aligned_free(p);
}

void* orThrow(void* p) {
    if (p == nullptr) throw std::bad_alloc();
    return p;
}

} // namespace

namespace test {

CountAllocations::CountAllocations() {
    allocationCount = 0;
    freeCount = 0;
    counting = true;
}

CountAllocations::~CountAllocations() { counting = false; }

int CountAllocations::allocations() const { return allocationCount; }
int CountAllocations::frees() const { return freeCount; }

} // namespace test

// The replacements. The standard allows a program to replace these; every
// allocation in the test executable, ours or the standard library's, comes here.

void* operator new(std::size_t size) { return orThrow(allocate(size)); }
void* operator new[](std::size_t size) { return orThrow(allocate(size)); }
void* operator new(std::size_t size, const std::nothrow_t&) noexcept { return allocate(size); }
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept { return allocate(size); }
void* operator new(std::size_t size, std::align_val_t a) { return orThrow(allocateAligned(size, a)); }
void* operator new[](std::size_t size, std::align_val_t a) { return orThrow(allocateAligned(size, a)); }
void* operator new(std::size_t size, std::align_val_t a, const std::nothrow_t&) noexcept {
    return allocateAligned(size, a);
}
void* operator new[](std::size_t size, std::align_val_t a, const std::nothrow_t&) noexcept {
    return allocateAligned(size, a);
}

void operator delete(void* p) noexcept { release(p); }
void operator delete[](void* p) noexcept { release(p); }
void operator delete(void* p, std::size_t) noexcept { release(p); }
void operator delete[](void* p, std::size_t) noexcept { release(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { release(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { release(p); }
void operator delete(void* p, std::align_val_t) noexcept { releaseAligned(p); }
void operator delete[](void* p, std::align_val_t) noexcept { releaseAligned(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { releaseAligned(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { releaseAligned(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { releaseAligned(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { releaseAligned(p); }
