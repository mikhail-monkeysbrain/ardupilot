#include "AP_VTXPowerTable.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main(){char out[5];
assert(JTPower::decode_label(387U,out)); assert(strcmp(out,"25")==0);
assert(JTPower::decode_label(4166U,out)); assert(strcmp(out,"500")==0);
assert(JTPower::decode_label(266306U,out)); assert(strcmp(out,"1000")==0);
assert(JTPower::decode_label(266627U,out)); assert(strcmp(out,"2500")==0);
assert(JTPower::decode_label(136790U,out)); assert(strcmp(out,"LOW")==0);
assert(JTPower::decode_label(4789458U,out)); assert(strcmp(out,"HIGH")==0);
assert(JTPower::decode_label(139991U,out)); assert(strcmp(out,"MAX")==0);
assert(JTPower::decode_label(124122U,out)); assert(strcmp(out,"PIT")==0);
assert(JTPower::decode_label(2881829U,out)); assert(strcmp(out,"aZ-9")==0);
assert(!JTPower::decode_label(0,out));
assert(!JTPower::decode_label(0x1000000U,out));
assert(!JTPower::decode_label(4097U,out));
puts("PASS: all power label decoding tests");
}
