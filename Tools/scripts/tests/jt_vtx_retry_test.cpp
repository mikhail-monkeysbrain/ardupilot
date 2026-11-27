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
    puts("ALL RETRY TESTS PASSED");
}
