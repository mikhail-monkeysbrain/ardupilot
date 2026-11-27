#include "AP_VTXPowerTable.h"
#include <cassert>
#include <cstdio>
using namespace JTPower;
int main() {
Table t{};
t.protocol = Protocol::SmartAudio2;
t.count = 4;
t.levels[0] = {0,25};
t.levels[1] = {1,500};
t.levels[2] = {2,1000};
t.levels[3] = {3,2500};
uint16_t wire = 0;
assert(validate(t));
assert(encode(t,0,wire) && wire == 0);
assert(encode(t,3,wire) && wire == 3);
assert(find_code(t,3) == 3);
assert(find_code(t,99) == -1);
assert(!encode(t,4,wire));
puts("PASS: SmartAudio2 codes and zero code");
t.levels[3].code = 2;
assert(!validate(t));
puts("PASS: duplicate code rejected");
t.count = 1;
t.protocol = Protocol::SmartAudio1;
t.levels[0].code = 40;
assert(encode(t,0,wire) && wire == 40);
puts("PASS: SmartAudio1 DAC");
t.protocol = Protocol::SmartAudio21;
t.levels[0].code = 30;
assert(encode(t,0,wire) && wire == 158);
t.levels[0].code = 128;
assert(!validate(t));
puts("PASS: SmartAudio21 encoding");
t.protocol = Protocol::Tramp;
t.levels[0].code = 2500;
assert(encode(t,0,wire) && wire == 2500);
puts("PASS: Tramp uint16 value");
t.count = 0;
assert(!validate(t));
t.count = 11;
assert(!validate(t));
puts("PASS: table boundaries");
puts("ALL POWER CODE TESTS PASSED");
}
