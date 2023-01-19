#include <vector>
#include <thread>
#include <atomic>

#include <string.h>

#include <iostream> // NOTE: 確認で使っているだけだよ
using std::cout;
using std::endl;

#include "../App/main.h"
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

std::vector<Tuple> Table(TUPLE_NUM);
std::vector<uint64_t> ThLocalEpoch(THREAD_NUM);
std::vector<uint64_t> CTIDW(THREAD_NUM);
std::vector<uint64_t> ThLocalDurableEpoch(LOGGER_NUM);
uint64_t DurableEpoch;
uint64_t GlobalEpoch = 1;
std::vector<returnResult> results(THREAD_NUM);
std::atomic<Logger *> logs[LOGGER_NUM];
Notifier notifier;

bool start = false;
bool quit = false;

// MARK: enclave function

// NOTE: enclave内で__atomic系の処理できたっけ？できるなら直でそのまま呼びたい
// というかquitは管理しなくてもいいかも
void ecall_sendStart() {
    __atomic_store_n(&start, true, __ATOMIC_RELEASE);
}

void ecall_sendQuit() {
    __atomic_store_n(&quit, true, __ATOMIC_RELEASE);
}

unsigned get_rand() {
    // 乱数生成器（引数にシードを指定可能）
    static std::mt19937 mt32(0);

    // [0, (2^32)-1] の一様分布整数を生成
    return mt32();
}

void ecall_worker_th(int thid, int gid) {
    TxExecutor trans(thid);
    returnResult &myres = std::ref(results[thid]);
    uint64_t epoch_timer_start, epoch_timer_stop;
    
    unsigned init_seed;
    init_seed = get_rand();
    // sgx_read_rand((unsigned char *) &init_seed, 4);
    Xoroshiro128Plus rnd(init_seed);

    Logger *logger;
    std::atomic<Logger*> *logp = &(logs[gid]);  // loggerのthreadIDを指定したいからgidを使う
    for (;;) {
        logger = logp->load();
        if (logger != 0) break;
        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
    }
    logger->add_tx_executor(trans);

    while (true) {
        if (__atomic_load_n(&start, __ATOMIC_ACQUIRE)) break;
    }

    if (thid == 0) epoch_timer_start = rdtscp();

    while (true) {
        if (__atomic_load_n(&quit, __ATOMIC_ACQUIRE)) break;
        
        makeProcedure(trans.pro_set_, rnd); // ocallで生成したprocedureをTxExecutorに移し替える

    RETRY:

        if (thid == 0) leaderWork(epoch_timer_start, epoch_timer_stop);
        trans.durableEpochWork(epoch_timer_start, epoch_timer_stop, quit);

        if (__atomic_load_n(&quit, __ATOMIC_ACQUIRE)) break;

        trans.begin();
        for (auto itr = trans.pro_set_.begin(); itr != trans.pro_set_.end(); itr++) {
            if ((*itr).ope_ == Ope::READ) {
                trans.read((*itr).key_);
            } else if ((*itr).ope_ == Ope::WRITE) {
                trans.write((*itr).key_);
            } else {
                // ERR;
                // DEBUG
                printf("おい！なんか変だぞ！\n");
                return;
            }
        }
   
        if (trans.validationPhase()) {
            trans.writePhase();
            storeRelease(myres.local_commit_counts_, loadAcquire(myres.local_commit_counts_) + 1);
        } else {
            trans.abort();
            myres.local_abort_counts_++;
            goto RETRY;
        }
    }

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

uint64_t ecall_getAbortResult(int thid) {
    return results[thid].local_abort_counts_;
}

uint64_t ecall_getCommitResult(int thid) {
    return results[thid].local_commit_counts_;
}