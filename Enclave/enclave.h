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

class KeyValue {
public:
    std::string_view key_;
    Tuple *value_;

    KeyValue() {}   // 空のコンストラクタがないと動かない？
    KeyValue(std::string_view key, Tuple *value) : key_(key), value_(value) {};
};

#if INDEX_PATTERN == 0
extern OptCuckoo<Tuple*> Table;
#elif INDEX_PATTERN == 1
extern std::vector<KeyValue> Table;
#else
// Masstree Table
#endif