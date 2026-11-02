#pragma once

namespace test {

// Counts heap allocations and frees made on this thread while it is alive.
//
// The test executable replaces the global operator new and operator delete
// (allocation_counter.cpp), so every allocation through them is seen: plain new,
// std::vector growth, std::string, an allocating std::function. Only the thread
// that created the counter is counted, so other threads can't make a test fail.
//
// Not seen: direct calls to malloc/free. Our code and its libraries don't make
// any at run time; Clang's RealtimeSanitizer would catch those too, but it does
// not support MSVC on Windows.
//
// Usage:
//     {
//         test::CountAllocations count;
//         engine.process(...);
//         EXPECT_EQ(count.allocations(), 0);
//     }
class CountAllocations {
public:
    CountAllocations();
    ~CountAllocations();
    CountAllocations(const CountAllocations&) = delete;
    CountAllocations& operator=(const CountAllocations&) = delete;

    int allocations() const;
    int frees() const;
};

} // namespace test
