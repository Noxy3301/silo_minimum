#pragma once

#include "../Include/classes.h"

extern void ecall_initDB();

extern void ecall_sendStart();
extern void ecall_sendQuit();

extern void ecall_worker_th(int thid);
extern void ecall_logger_th();

extern uint64_t ecall_getAbortResult(int thid);
extern uint64_t ecall_getCommitResult(int thid);