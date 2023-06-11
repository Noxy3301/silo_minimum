#pragma once

#include "../Include/result.h"
#include "../Include/consts.h"
#include "include/tuple.h"

extern void ecall_initDB();

void ecall_waitForReady();
extern void ecall_sendStart();
extern void ecall_sendQuit();

extern void ecall_worker_th(int thid, int gid);
extern void ecall_logger_th(int thid);

extern uint64_t ecall_getResult(int thid, int datatype);

extern uint64_t ecall_showDurableEpoch();

template <class T>
class LinearIndex {
public:
    std::vector<Tuple*> table_;
    int table_size_;

    LinearIndex() {}

    void insert_value(T value) {
        table_.emplace_back(value);
    }

    T get(std::string key) {
        for (int i = 0; i < table_.size(); i++) {
            // print_String2Hex(table_[i]->key_, false);
            // cout << " ";
            // print_String2Hex(key);
            // cout << "|" << print_hexString(table_[i]->key_) << "|" << print_hexString(key)  << "|" << endl;
            if (table_[i]->key_ == key) {   // std::cout << "aru" << std::endl;
                return table_[i];
            }
        } // std::cout << "nai" << std::endl;
        return nullptr;
    }

    // void print_String2Hex(std::string str, bool isFlush = true) {
    //     // debug用、keyを8x8に戻す
    //     for (int i = 0; i < 8; i++) {
    //         std::cout << int(uint8_t(str[i])) << ",";
    //     }
    //     if (isFlush) std::cout << std::endl;
    // }
};

#if INDEX_PATTERN == 0
extern std::vector<OptCuckoo<Tuple*>> Table;
#elif INDEX_PATTERN == 1
extern LinearIndex<Tuple*> Table;
#else
// Masstree Table
#endif