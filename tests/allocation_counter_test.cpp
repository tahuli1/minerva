#include "allocation_counter.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

// The counter must really see allocations; otherwise a test that expects zero
// would pass for the wrong reason.

TEST(AllocationCounter, SeesNewAndDelete) {
    test::CountAllocations count;
    int* p = new int(1);
    delete p;
    EXPECT_EQ(count.allocations(), 1);
    EXPECT_EQ(count.frees(), 1);
}

TEST(AllocationCounter, SeesStandardContainers) {
    test::CountAllocations count;
    std::vector<int> v;
    v.push_back(1); // the first push_back allocates
    EXPECT_GE(count.allocations(), 1);
}

TEST(AllocationCounter, SeesOverAlignedAllocations) {
    struct alignas(64) CacheLine {
        char bytes[64];
    };
    test::CountAllocations count;
    auto p = std::make_unique<CacheLine>();
    p.reset();
    EXPECT_EQ(count.allocations(), 1);
    EXPECT_EQ(count.frees(), 1);
}

TEST(AllocationCounter, CountsNothingOnceItIsGone) {
    int allocationsInside = 0;
    {
        test::CountAllocations count;
        allocationsInside = count.allocations();
    }
    test::CountAllocations later; // starts from zero
    EXPECT_EQ(allocationsInside, 0);
    EXPECT_EQ(later.allocations(), 0);
}
