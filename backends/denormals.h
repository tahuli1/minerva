#pragma once

// x86/x64 only (SSE control register). Lives in backends/, not core/, because it is
// CPU-specific: an ARM port (STM32) would set the FZ bit in FPSCR instead.
#include <xmmintrin.h>

namespace ukefx {

// Turns on Flush-To-Zero and Denormals-Are-Zero for the current thread while in scope.
//
// Floats below ~1e-38 become "denormal", and x86 math on them is tens to hundreds
// of times slower. Decaying tails (delay feedback, reverb, IIR filters) drift into
// exactly that range, so CPU load can spike during *silence*. FTZ makes results that
// would be denormal come out as 0; DAZ treats denormal inputs as 0.
//
// MXCSR is per thread, so this must run on the audio thread itself. The previous
// value is restored on exit so code we share the thread with is not affected.
class ScopedNoDenormals {
public:
    ScopedNoDenormals() : saved_(_mm_getcsr()) { _mm_setcsr(saved_ | kFlushToZero | kDenormalsAreZero); }
    ~ScopedNoDenormals() { _mm_setcsr(saved_); }
    ScopedNoDenormals(const ScopedNoDenormals&) = delete;
    ScopedNoDenormals& operator=(const ScopedNoDenormals&) = delete;

private:
    static constexpr unsigned int kFlushToZero = 0x8000;     // MXCSR bit 15
    static constexpr unsigned int kDenormalsAreZero = 0x0040; // MXCSR bit 6
    unsigned int saved_;
};

} // namespace ukefx
