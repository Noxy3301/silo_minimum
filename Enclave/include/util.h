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

extern bool chkEpochLoaded();
extern void leaderWork(uint64_t &epoch_timer_start, uint64_t &epoch_timer_stop);
extern void ecall_initDB();
extern void makeProcedure(std::vector<Procedure> &pro, Xoroshiro128Plus &rnd);