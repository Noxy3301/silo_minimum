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

#include "OCH.cpp"

#if INDEX_PATTERN == INDEX_USE_MASSTREE
// do define masstree table
#elif INDEX_PATTERN == INDEX_USE_OCH
OptCuckoo<Tuple*> Table(TUPLE_NUM*2);
#endif

uint64_t GlobalEpoch = 1;                       // Global Epoch
std::vector<uint64_t> ThLocalEpoch(THREAD_NUM); // 各ワーカースレッドのLocal epoch, Global epochを参照せず、epoch更新時に更新されるLocal epochを参照してtxを処理する
std::vector<uint64_t> CTIDW(THREAD_NUM);        // 各ワーカースレッドのCommit Timestamp ID, TID算出時にWorkerが発行したTIDとして用いられたものが保存される(TxExecutorのmrctid_と同じ値を持っているはず...)

uint64_t DurableEpoch;                                  // Durable Epoch, 永続化された全てのデータのエポックの最大値を表す(epoch <= DのtxはCommit通知ができる)
std::vector<uint64_t> ThLocalDurableEpoch(LOGGER_NUM);  // 各ロガースレッドのLocal durable epoch, Global durable epcohの算出に使う

std::vector<WorkerResult> workerResults(THREAD_NUM);    // ワーカースレッドの実行結果を格納するベクター
std::vector<LoggerResult> loggerResults(LOGGER_NUM);    // ロガースレッドの実行結果を格納するベクター

std::atomic<Logger *> logs[LOGGER_NUM]; // ロガーオブジェクトのアトミックなポインタを保持する配列, 複数のスレッドからの安全なアクセスが可能
Notifier notifier;                      // 通知オブジェクト, スレッド間でのイベント通知を管理する

std::vector<int> readys(THREAD_NUM);    // 各ワーカースレッドの準備状態を表すベクター, 全てのスレッドが準備完了するのを待つ
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
    TxExecutor trans(thid);
    WorkerResult &myres = std::ref(workerResults[thid]);
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
        waitTime_ns(100);
    }
    logger->add_tx_executor(trans);

    __atomic_store_n(&readys[thid], 1, __ATOMIC_RELEASE);

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
                assert(false);
            }
        }
   
        if (trans.validationPhase()) {
            trans.writePhase();
            storeRelease(myres.local_commit_count_, loadAcquire(myres.local_commit_count_) + 1);
        } else {
            trans.abort();
            // TODO: ここ汚いからtransの方でいい感じに処理してenumでごねごねする
            assert(trans.abort_res_ != 0);
            switch (trans.abort_res_) {
                case 1:
                    myres.local_abort_vp1_count_++;
                    break;
                case 2:
                    myres.local_abort_vp2_count_++;
                    break;
                case 3:
                    myres.local_abort_vp3_count_++;
                    break;
                case 4:
                    myres.local_abort_nullBuffer_count_++;
                    break;
                default:
                    assert(false);
                    break;
            }
            myres.local_abort_count_++;
            goto RETRY;
        }
    }

    trans.log_buffer_pool_.terminate();
    logger->worker_end(thid);

    return;
}

void ecall_logger_th(int thid) {
    Logger logger(thid, std::ref(notifier), std::ref(loggerResults[thid]));
    notifier.add_logger(&logger);
    std::atomic<Logger*> *logp = &(logs[thid]);
    logp->store(&logger);
    logger.worker();
    return;
}

uint64_t ecall_getAbortCount(int thid) {
    return workerResults[thid].local_abort_count_;
}

uint64_t ecall_getCommitCount(int thid) {
    return workerResults[thid].local_commit_count_;
}

uint64_t ecall_getSpecificAbortCount(int thid, int reason) {
    switch (reason) {
        case AbortReason::ValidationPhase1:
            return workerResults[thid].local_abort_vp1_count_;
        case AbortReason::ValidationPhase2:
            return workerResults[thid].local_abort_vp2_count_;
        case AbortReason::ValidationPhase3:
            return workerResults[thid].local_abort_vp3_count_;
        case AbortReason::NullCurrentBuffer:
            return workerResults[thid].local_abort_nullBuffer_count_;
        default:
            assert(false);  // ここに来てはいけない
            return 0;
    }
}

uint64_t ecall_getLoggerCount(int thid, int type) {
	switch (type) {
		case LoggerResultType::ByteCount:
			return loggerResults[thid].byte_count_;
		case LoggerResultType::WriteLatency:
			return loggerResults[thid].write_latency_;
        case LoggerResultType::WaitLatency:
            return loggerResults[thid].wait_latency_;
		default: 
			assert(false);  // ここに来てはいけない
            return 0;
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