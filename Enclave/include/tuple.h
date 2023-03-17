#pragma once

#include <cstdint>

#include "cache_line_size.h"

struct Tidword {
    union {
        uint64_t obj_;
        struct {
            bool lock : 1;
            bool latest : 1;
            bool absent : 1;
            uint64_t TID : 29;
            uint64_t epoch : 32;
        };
    };
    Tidword() : epoch(0), TID(0), absent(false), latest(true), lock(false) {};

    bool operator == (const Tidword &right) const { return obj_ == right.obj_; }
    bool operator != (const Tidword &right) const { return !operator == (right); }
    bool operator < (const Tidword &right) const { return this->obj_ < right.obj_; }
};

class Tuple {
    public:
        alignas(CACHE_LINE_SIZE) Tidword tidword_;
        uint64_t key_;
        uint32_t val_;
};