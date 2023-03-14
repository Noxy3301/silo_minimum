#pragma once

#include "../Include/result.h"

extern void ecall_initDB();

void ecall_waitForReady();
extern void ecall_sendStart();
extern void ecall_sendQuit();

extern void ecall_worker_th(int thid, int gid);
extern void ecall_logger_th(int thid);

extern uint64_t ecall_getAbortResult(int thid);
extern uint64_t ecall_getCommitResult(int thid);
extern uint64_t ecall_getAbortResResult(int thid, int res);

extern uint64_t ecall_showDurableEpoch();