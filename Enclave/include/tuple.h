#pragma once

#include <cstdint>

#include "cache_line_size.h"
// #include "tuple_body.hh"

struct Tidword {
    union {
        uint64_t obj_;
        struct {
            bool lock : 1;
            bool latest : 1;
            bool absent : 1;
            uint64_t tid : 29;
            uint64_t epoch : 32;
        };
    };
    Tidword() : epoch(0), tid(0), absent(false), latest(true), lock(false) {};

    bool operator == (const Tidword &right) const { return obj_ == right.obj_; }
    bool operator != (const Tidword &right) const { return !operator == (right); }
    bool operator < (const Tidword &right) const { return this->obj_ < right.obj_; }
};

// valueをvoid*型にして任意の型を格納できるようにしているので、どの型に変換するかを一緒に書いておく
enum class ValueType : uint32_t {
    integer,
    string,
    TPCC_item,
};

/**
 * @param key std::string
 * @param value void*
 * @param valueType ValueType::~
 */
class Tuple {
public:
    alignas(CACHE_LINE_SIZE) 
    Tidword tidword_;
    std::string key_;
    void* value_;
    ValueType valueType_;

    Tuple(std::string key, void* value, ValueType valueType) : key_(key), value_(value), valueType_(valueType) {}

    void init() {
        tidword_.epoch = 1;
        tidword_.latest = true;
        tidword_.absent = false;
        tidword_.lock = false;
    }

    // void init([[maybe_unused]] size_t thid, TupleBody&& body, [[maybe_unused]] void* p) {
    //     // for initializer
    //     tidword_.epoch = 1;
    //     tidword_.latest = true;
    //     tidword_.absent = false;
    //     tidword_.lock = false;
    //     body_ = std::move(body);
    // }

    // void init(TupleBody&& body) {
    //     tidword_.absent = true;
    //     tidword_.lock = true;
    //     body_ = std::move(body);
    // }
};