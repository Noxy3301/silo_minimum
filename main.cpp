#include "./include/random.h"
#include "./include/zipf.h"

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>

#include "consts.h"

#include "enclave.h"
#include "classes.h"
#include <mutex>

// MARK: class(LoggerAffinity)

class LoggerNode {
    public:
        int logger_cpu_;
        std::vector<int> worker_cpu_;

        LoggerNode() {}
};

class LoggerAffinity {
    public:
        std::vector<LoggerNode> nodes_;
        unsigned worker_num_ = 0;
        unsigned logger_num_ = 0;
        void init(unsigned worker_num, unsigned logger_num);
};

void LoggerAffinity::init(unsigned worker_num, unsigned logger_num) {
    unsigned num_cpus = std::thread::hardware_concurrency();
    if (logger_num > num_cpus || worker_num > num_cpus) {
        std::cout << "too many threads" << std::endl;
    }
    // LoggerAffinityのworker_numとlogger_numにコピー
    worker_num_ = worker_num;
    logger_num_ = logger_num;
    for (unsigned i = 0; i < logger_num; i++) {
        nodes_.emplace_back();
    }
    unsigned thread_num = logger_num + worker_num;
    if (thread_num > num_cpus) {
        for (unsigned i = 0; i < worker_num; i++) {
            nodes_[i * logger_num/worker_num].worker_cpu_.emplace_back(i);
        }
        for (unsigned i = 0; i < logger_num; i++) {
            nodes_[i].logger_cpu_ = nodes_[i].worker_cpu_.back();
        }
    } else {
        for (unsigned i = 0; i < thread_num; i++) {
            nodes_[i * logger_num/thread_num].worker_cpu_.emplace_back(i);
        }
        for (unsigned i = 0; i < logger_num; i++) {
            nodes_[i].logger_cpu_ = nodes_[i].worker_cpu_.back();
            nodes_[i].worker_cpu_.pop_back();
        }
    }
}

// MARK: global variables

bool threadReady = false;

// MARK: siloResult

std::vector<Result> SiloResult(THREAD_NUM);

// MARK: thread function

void worker_th(int thid, int &ready) {
    __atomic_store_n(&ready, 1, __ATOMIC_RELEASE);
    while (true) {
        if (__atomic_load_n(&threadReady, __ATOMIC_ACQUIRE)) break;
    }

    Result res;
    ecall_worker_th(thid, std::ref(res));  // thread.emplace_backで直接渡せる気がしないし、こっちで受け取ってResultの下処理をしたい

    // TODO: get return and collect them
    SiloResult[thid].local_commit_counts_ = res.local_commit_counts_ ;
    SiloResult[thid].local_abort_counts_ = res.local_abort_counts_;
    
    // std::cout << "worker_end" << std::endl;
}

void logger_th(int thid) {

}

// MARK: utilities

void waitForReady(const std::vector<int> &readys) {
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

Xoroshiro128Plus rnd;
FastZipf zipf(&rnd, ZIPF_SKEW, TUPLE_NUM);  //関数内で宣言すると割り算処理繰り返すからクソ重いぞ！

void ocall_makeProcedure(uint64_t pro[MAX_OPE][2]) {
    rnd.init();
    for (int i = 0; i < MAX_OPE; i++) {
        uint64_t tmpkey;
        // keyの決定
        if (YCSB) {
            tmpkey = zipf() % TUPLE_NUM;
        } else {
            tmpkey = rnd.next() % TUPLE_NUM;
        }
        pro[i][1] = tmpkey;

        // Operation typeの決定
        if ((rnd.next() % 100) < RRAITO) {
            // pro.emplace_back(Ope::READ, tmpkey);
            pro[i][0] = 0;
        } else {
            // pro.emplace_back(Ope::WRITE, tmpkey);
            pro[i][0] = 1;
        }
    }
}

void displayParameter() {
    cout << "#clocks_per_us:\t" << CLOCKS_PER_US << endl;
    cout << "#epoch_time:\t" << EPOCH_TIME << endl;
    cout << "#extime:\t" << EXTIME << endl;
    cout << "#max_ope:\t" << MAX_OPE << endl;
    // cout << "#rmw:\t\t" << RMW << endl;
    cout << "#rratio:\t" << RRAITO << endl;
    cout << "#thread_num:\t" << THREAD_NUM << endl;
    cout << "#tuple_num:\t" << TUPLE_NUM << endl;
    cout << "#ycsb:\t\t" << YCSB << endl;
    cout << "#zipf_skew:\t" << ZIPF_SKEW << endl;
    cout << "#logger_num:\t" << LOGGER_NUM << endl;
    // cout << "#buffer_num:\t" << BUFFER_NUM << endl;
    // cout << "#buffer_size:\t" << BUFFER_SIZE << endl;
}

void displayResult() {
    uint64_t total_commit_counts_ = 0;
    uint64_t total_abort_counts_ = 0;

    for (int i = 0; i < THREAD_NUM; i++) {
        total_commit_counts_ += SiloResult[i].local_commit_counts_;
        total_abort_counts_ += SiloResult[i].local_abort_counts_;
    }

    cout << "commit_counts_:\t" << total_commit_counts_ << endl;
    cout << "abort_counts_:\t" << total_abort_counts_ << endl;

    uint64_t result = total_commit_counts_ / EXTIME;
    cout << "latency[ns]:\t" << powl(10.0, 9.0) / result * THREAD_NUM << endl;
    cout << "throughput[tps]:\t" << result << endl;
}


// MARK: main function

int main() {
    displayParameter();
    ecall_initDB();
    LoggerAffinity affin;
    affin.init(THREAD_NUM, LOGGER_NUM); // logger/worker実行threadの決定

    std::vector<int> readys(THREAD_NUM);
    std::vector<std::thread> lthv;
    std::vector<std::thread> wthv;

    int i = 0, j = 0;
    for (auto itr = affin.nodes_.begin(); itr != affin.nodes_.end(); itr++, j++) {
        lthv.emplace_back(logger_th, j);    // TODO: add some arguments
        for (auto wcpu = itr->worker_cpu_.begin(); wcpu != itr->worker_cpu_.end(); wcpu++, i++) {
            wthv.emplace_back(worker_th, i, std::ref(readys[i]));  // TODO: add some arguments
        }
    }

    waitForReady(readys);
    __atomic_store_n(&threadReady, true, __ATOMIC_RELEASE);
    ecall_sendStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * EXTIME));
    ecall_sendQuit();

    for (auto &th : lthv) th.join();
    for (auto &th : wthv) th.join();

    displayResult();

    return 0;
}