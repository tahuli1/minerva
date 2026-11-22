#include "options.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

bool parse(std::vector<const char*> args, cli::Options& options, std::string& error) {
    args.insert(args.begin(), "minerva_cli");
    return cli::parseOptions(static_cast<int>(args.size()), args.data(), options, error);
}

} // namespace

TEST(Options, DefaultsAreForPlaying) {
    cli::Options o;
    std::string error;
    ASSERT_TRUE(parse({}, o, error));
    EXPECT_FALSE(o.showStats);
    EXPECT_EQ(o.bufferFrames, 128u);
    EXPECT_EQ(o.loadThreads, 0);
    EXPECT_TRUE(o.csvPath.empty());
    EXPECT_EQ(o.seconds, 0);
}

TEST(Options, ReadsEveryOption) {
    cli::Options o;
    std::string error;
    ASSERT_TRUE(parse({"--stats", "--buffer", "64", "--load", "4", "--csv", "run.csv", "--seconds", "30"},
                      o, error));
    EXPECT_TRUE(o.showStats);
    EXPECT_EQ(o.bufferFrames, 64u);
    EXPECT_EQ(o.loadThreads, 4);
    EXPECT_EQ(o.csvPath, "run.csv");
    EXPECT_EQ(o.seconds, 30);
}

TEST(Options, RejectsUnknownOptions) {
    cli::Options o;
    std::string error;
    EXPECT_FALSE(parse({"--bogus"}, o, error));
    EXPECT_EQ(error, "Unknown option: --bogus");
}

TEST(Options, RejectsAMissingValue) {
    cli::Options o;
    std::string error;
    EXPECT_FALSE(parse({"--buffer"}, o, error));
    EXPECT_EQ(error, "--buffer needs a value");
}

TEST(Options, RejectsValuesThatAreNotWholeNumbersInRange) {
    cli::Options o;
    std::string error;
    EXPECT_FALSE(parse({"--buffer", "abc"}, o, error));
    EXPECT_FALSE(parse({"--buffer", "64x"}, o, error));
    EXPECT_FALSE(parse({"--buffer", "8"}, o, error));
    EXPECT_FALSE(parse({"--load", "-1"}, o, error));
    EXPECT_FALSE(parse({"--seconds", "0"}, o, error));
}
