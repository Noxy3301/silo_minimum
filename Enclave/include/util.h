#pragma once

#include <cstdint>

#include "atomic_tool.h"
#include "procedure.h"
#include "random.h"
#include "tsc.h"

inline static bool chkClkSpan(const uint64_t start, const uint64_t stop, const uint64_t threshold) {
    uint64_t diff = 0;
    diff = stop - start;
    if (diff > threshold) {
        return true;
    } else {
        return false;
    }
}

inline static void waitTime_ns(const uint64_t time) {
    uint64_t start = rdtscp();
    uint64_t end = 0;
    for (;;) {
        end = rdtscp();
        if (end - start > time * CLOCKS_PER_US) break;   // ns換算にしたいけど除算はコストが高そうなので1000倍して調整
    }
}

template<typename Int>
Int byteswap(Int in) {
  switch (sizeof(Int)) {
    case 1:
      return in;
    case 2:
      return __builtin_bswap16(in);
    case 4:
      return __builtin_bswap32(in);
    case 8:
      return __builtin_bswap64(in);
    default:
      assert(false);
  }
}

template<typename Int>
void assign_as_bigendian(Int value, char *out) {
  Int tmp = byteswap(value);
  ::memcpy(out, &tmp, sizeof(tmp));
}

template<typename Int>
void parse_bigendian(const char *in, Int &out) {
  Int tmp;
  ::memcpy(&tmp, in, sizeof(tmp));
  out = byteswap(tmp);
}

extern bool chkEpochLoaded();
extern void siloLeaderWork(uint64_t &epoch_timer_start, uint64_t &epoch_timer_stop);
extern void ecall_initDB();
extern void makeProcedure(std::vector<Procedure> &pro, Xoroshiro128Plus &rnd);