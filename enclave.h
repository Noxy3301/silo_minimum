#pragma once

#include "classes.h"

extern void ecall_initDB();

extern void ecall_sendStart();
extern void ecall_sendQuit();

extern void ecall_worker_th(int thid, Result &res);
extern void ecall_logger_th();