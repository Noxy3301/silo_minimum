#include <vector>
#include <thread>
#include <atomic>

#include <string.h>

#include <iostream> // NOTE: 確認で使っているだけだよ
using std::cout;
using std::endl;

#include "../App/include/main.h"
#include "enclave.h"
#include "../Include/consts.h"

#include "include/logger.h"
#include "include/notifier.h"

#include "include/atomic_tool.h"
#include "include/atomic_wrapper.h"
#include "include/silo_op_element.h"
#include "include/transaction.h"
#include "include/tsc.h"
#include "include/util.h"
#include "include/zipf.h"

#include "OCH.cpp"
// #include "include/tpcc.h"
#include "include/ycsb.h"

#if INDEX_PATTERN == 0
OptCuckoo<Tuple*> Table(TUPLE_NUM*2);
#elif INDEX_PATTERN == 1
LinearIndex<Tuple*> Table;
#else
// Masstree Table
#endif
std::vector<uint64_t> ThLocalEpoch(THREAD_NUM);
std::vector<uint64_t> CTIDW(THREAD_NUM);
std::vector<uint64_t> ThLocalDurableEpoch(LOGGER_NUM);
uint64_t DurableEpoch;
uint64_t GlobalEpoch = 1;
std::vector<Result> results(THREAD_NUM);
std::atomic<Logger *> logs[LOGGER_NUM];
Notifier notifier;
std::vector<int> readys(THREAD_NUM);

bool start = false;
bool quit = false;

// MARK: enclave function

// NOTE: enclave内で__atomic系の処理できたっけ？できるなら直でそのまま呼びたい
// というかquitは管理しなくてもいいかも

void ecall_waitForReady() {
    while (true) {
        bool failed = false;
        for (const auto &ready : readys) {
            if (!__atomic_load_n(&ready, __ATOMIC_ACQUIRE)) {
                failed = true;
                break;
            }
        }
        if (!failed) break;
    }
}

void ecall_sendStart() {
    __atomic_store_n(&start, true, __ATOMIC_RELEASE);
}

void ecall_sendQuit() {
    __atomic_store_n(&quit, true, __ATOMIC_RELEASE);
}

unsigned get_rand() {
    // 乱数生成器（引数にシードを指定可能）, [0, (2^32)-1] の一様分布整数を生成
    static std::mt19937 mt32(0);
    return mt32();
}

void ecall_worker_th(int thid, int gid) {
    Result &myres = std::ref(results[thid]);
    TxExecutor trans(thid, (Result *) &myres, std::ref(quit));

    // assign logger thread
    Logger *logger;
    std::atomic<Logger*> *logp = &(logs[gid]);  // loggerのthreadIDを指定したいからgidを使う
    for (;;) {
        logger = logp->load();
        if (logger != 0) break;
        waitTime_ns(100);
    }
    logger->add_tx_executor(trans);

#if BENCHMARK == 0  // TPC-C-NP benchmark
    TPCCWorkload<Tuple,void> workload;
#elif BENCHMARK == 1    // YCSB benchmark
    YcsbWorkload workload;

    // Wait for other thread's ready
    storeRelease(readys[thid], 1);
    while (!loadAcquire(start)) continue;

    // Execute transaction while quit == false
    if (thid == 0) trans.epoch_timer_start = rdtscp();
    while (!loadAcquire(quit)) {
        workload.run<TxExecutor,TransactionStatus>(trans);
    }
#endif
    // terminate logger
    trans.log_buffer_pool_.terminate();
    logger->worker_end(thid);
    return;
}

void ecall_logger_th(int thid) {
    Logger logger(thid, std::ref(notifier));
    notifier.add_logger(&logger);
    std::atomic<Logger*> *logp = &(logs[thid]);
    logp->store(&logger);
    logger.worker();
    return;
}

// [datatype]
// 0: local_abort_counts
// 1: local_commit_counts
// 2: local_abort_by_validation1
// 3: local_abort_by_validation2
// 4: local_abort_by_validation3
// 5: local_abort_by_null_buffer
uint64_t ecall_getResult(int thid, int datatype) {
    switch (datatype) {
    case 0: // local_abort_counts
        return results[thid].local_abort_counts_;
    case 1: // local_commit_counts
        return results[thid].local_commit_counts_;
#if ADD_ANALYSIS
    case 2: // local_abort_by_validation1
        return results[thid].local_abort_by_validation1_;
    case 3: // local_abort_by_validation2
        return results[thid].local_abort_by_validation2_;
    case 4: // local_abort_by_validation3
        return results[thid].local_abort_by_validation3_;
    case 5: // local_abort_by_null_buffer
        return results[thid].local_abort_by_null_buffer_;
#endif
    default:
        break;
    }
}

uint64_t ecall_showDurableEpoch() {
    uint64_t min_dl = __atomic_load_n(&(ThLocalDurableEpoch[0]), __ATOMIC_ACQUIRE);
    for (unsigned int i=1; i < LOGGER_NUM; ++i) {
        uint64_t dl = __atomic_load_n(&(ThLocalDurableEpoch[i]), __ATOMIC_ACQUIRE);
        if (dl < min_dl) {
            min_dl = dl;
        }
    }
    return min_dl;
}