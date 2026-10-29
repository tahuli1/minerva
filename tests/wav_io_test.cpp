#include "wav_io.h"

#include <dr_wav.h>
#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <string>

namespace {

// A file in the system temp folder, named after the running test and removed afterwards.
class TempWav {
public:
    TempWav() {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        path_ = (std::filesystem::temp_directory_path() /
                 (std::string("minerva_") + info->test_suite_name() + "_" + info->name() + ".wav"))
                    .string();
    }
    ~TempWav() { std::filesystem::remove(path_); }
    const std::string& path() const { return path_; }

private:
    std::string path_;
};

} // namespace

TEST(WavIo, RoundTripIsExact) {
    TempWav file;
    minerva::Audio original;
    original.sampleRate = 44100;
    for (int i = 0; i < 1000; ++i) original.samples.push_back(0.5f * std::sin(0.01f * i));

    std::string error;
    ASSERT_TRUE(minerva::writeWav(file.path(), original, error)) << error;
    const auto loaded = minerva::readWav(file.path(), error);
    ASSERT_TRUE(loaded) << error;

    EXPECT_EQ(loaded->sampleRate, 44100);
    EXPECT_EQ(loaded->samples, original.samples); // float WAV: bit-exact
}

TEST(WavIo, StereoIsAveragedToMono) {
    TempWav file;
    drwav_data_format format{};
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = 2;
    format.sampleRate = 48000;
    format.bitsPerSample = 32;
    const float interleaved[] = {1.0f, 0.0f, 0.5f, -0.5f, -1.0f, -0.5f}; // L R L R L R
    drwav wav;
    ASSERT_TRUE(drwav_init_file_write(&wav, file.path().c_str(), &format, nullptr));
    drwav_write_pcm_frames(&wav, 3, interleaved);
    drwav_uninit(&wav);

    std::string error;
    const auto loaded = minerva::readWav(file.path(), error);
    ASSERT_TRUE(loaded) << error;
    EXPECT_EQ(loaded->samples, (std::vector<float>{0.5f, 0.0f, -0.75f}));
}

TEST(WavIo, MissingFileReportsAnError) {
    std::string error;
    EXPECT_FALSE(minerva::readWav("this/file/does/not/exist.wav", error));
    EXPECT_NE(error.find("exist.wav"), std::string::npos);
}
