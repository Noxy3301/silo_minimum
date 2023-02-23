#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <cmath>

#include "../Include/consts.h"

#include "../Enclave/enclave.h"
#include "../Include/result.h"
#include <mutex>

using namespace std;

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

// MARK: siloResult

std::vector<Result> SiloResult(THREAD_NUM);

// MARK: thread function

void worker_th(int thid, int gid) {
    returnResult ret;
    ecall_worker_th(thid, gid);  // thread.emplace_backで直接渡せる気がしないし、こっちで受け取ってResultの下処理をしたい
    ret.local_abort_counts_ = ecall_getAbortResult(thid);
    ret.local_commit_counts_ = ecall_getCommitResult(thid);

    SiloResult[thid].local_commit_counts_ = ret.local_commit_counts_ ;
    SiloResult[thid].local_abort_counts_ = ret.local_abort_counts_;
}

void logger_th(int thid) {
    ecall_logger_th(thid);
}

// MARK: utilities

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
    cout << "#buffer_num:\t" << BUFFER_NUM << endl;
    cout << "#buffer_size:\t" << BUFFER_SIZE << endl;
}

void displayResult() {
    uint64_t total_commit_counts_ = 0;
    uint64_t total_abort_counts_ = 0;

    for (int i = 0; i < THREAD_NUM; i++) {
        cout << "thread#" << i << "\tcommit: " << SiloResult[i].local_commit_counts_ << "\tabort:" << SiloResult[i].local_abort_counts_ << endl;
        total_commit_counts_ += SiloResult[i].local_commit_counts_;
        total_abort_counts_ += SiloResult[i].local_abort_counts_;
    }

    cout << "[info]\tcommit_counts_:\t" << total_commit_counts_ << endl;
    cout << "[info]\tabort_counts_:\t" << total_abort_counts_ << endl;
    cout << "[info]\tabort_rate:\t" << (double)total_abort_counts_ / (double)(total_commit_counts_ + total_abort_counts_) << endl;

    uint64_t result = total_commit_counts_ / EXTIME;
    cout << "[info]\tlatency[ns]:\t" << powl(10.0, 9.0) / result * THREAD_NUM << endl;
    cout << "[info]\tthroughput[tps]:\t" << result << endl;
}

std::atomic<int> ocall_count(0);

// MARK: main function

int main() {

    chrono::system_clock::time_point p1, p2, p3, p4, p5;

    displayParameter();

    p1 = chrono::system_clock::now();
    
    ecall_initDB();
    LoggerAffinity affin;
    affin.init(THREAD_NUM, LOGGER_NUM); // logger/worker実行threadの決定

    std::vector<std::thread> lthv;
    std::vector<std::thread> wthv;

    p2 = chrono::system_clock::now();

    int i = 0, j = 0;
    for (auto itr = affin.nodes_.begin(); itr != affin.nodes_.end(); itr++, j++) {
        lthv.emplace_back(logger_th, j);    // TODO: add some arguments
        for (auto wcpu = itr->worker_cpu_.begin(); wcpu != itr->worker_cpu_.end(); wcpu++, i++) {
            wthv.emplace_back(worker_th, i, j);  // TODO: add some arguments
        }
    }

    ecall_waitForReady();
    p3 = chrono::system_clock::now();
    ecall_sendStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * EXTIME));
    ecall_sendQuit();

    p4 = chrono::system_clock::now();

    for (auto &th : lthv) th.join();
    for (auto &th : wthv) th.join();

    p5 = chrono::system_clock::now();

    double duration1 = static_cast<double>(chrono::duration_cast<chrono::microseconds>(p2 - p1).count() / 1000.0);
    double duration2 = static_cast<double>(chrono::duration_cast<chrono::microseconds>(p3 - p2).count() / 1000.0);
    double duration3 = static_cast<double>(chrono::duration_cast<chrono::microseconds>(p4 - p3).count() / 1000.0);
    double duration4 = static_cast<double>(chrono::duration_cast<chrono::microseconds>(p5 - p4).count() / 1000.0);

    displayResult();

    std::cout << "[info]\tmakeDB:\t" << duration1/1000 << "s.\n";
    std::cout << "[info]\tcreateThread:\t" << duration2/1000 << "s.\n";
    std::cout << "[info]\texecutionTime:\t" << duration3/1000 << "s.\n";
    std::cout << "[info]\tdestroyThread:\t" << duration3/1000 << "s.\n";
    std::cout << "[info]\tcall_count(write):\t" << ocall_count.load() << std::endl;
    uint64_t ret_durableEpoch = ecall_showDurableEpoch();
    std::cout << "[info]\tdurableEpoch:\t" << ret_durableEpoch << std::endl;

    return 0;
}