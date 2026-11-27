#include "AP_VTXRetry.h"
#include <cassert>
#include <cstdio>
int main() {
    JTVTX::Retry freq, power;
    for (int i = 0; i < 5; ++i) { assert(freq.allow(5800)); }
    assert(!freq.allow(5800));
    assert(!freq.allow(5800));
    assert(freq.attempts() == 5);
    puts("PASS: maximum five attempts");
    assert(power.allow(0));
    puts("PASS: independent command and zero code");
    assert(freq.allow(5820));
    assert(freq.attempts() == 1);
    puts("PASS: changed value resets attempts");
    freq.reset();
    assert(freq.allow(5820));
    assert(freq.attempts() == 1);
    puts("PASS: explicit reset");
    JTVTX::Retry mode;
    for (int i = 0; i < 100; ++i) { assert(mode.available(1)); }
    assert(mode.attempts() == 0);
    for (int i = 0; i < 5; ++i) { assert(mode.available(1)); assert(mode.allow(1)); }
    assert(!mode.available(1));
    assert(mode.available(2));
    assert(mode.attempts() == 5);
    puts("PASS: availability check does not consume attempts");
    puts("ALL RETRY TESTS PASSED");
}
