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

#include "include/logger_affinity.h"
#include "include/util.h"
#include "../Include/result.h"

std::vector <Result> SiloResult(THREAD_NUM);

void worker_th(int thid, int gid) {
    ecall_worker_th(thid, gid);  // thread.emplace_backで直接渡せる気がしないし、こっちで受け取ってResultの下処理をしたい

    // Enclaveからデータを持ってくる, enumで可読性を高めたかったけどedlに書けるとは思えないのでやむを得ず直書き
    SiloResult[thid].local_abort_counts_ = ecall_getResult(thid, 0);
    SiloResult[thid].local_commit_counts_ = ecall_getResult(thid, 1);
#if ADD_ANALYSIS
    SiloResult[thid].local_abort_by_validation1_ = ecall_getResult(thid, 2);
    SiloResult[thid].local_abort_by_validation2_ = ecall_getResult(thid, 3);
    SiloResult[thid].local_abort_by_validation3_ = ecall_getResult(thid, 4);
    SiloResult[thid].local_abort_by_null_buffer_ = ecall_getResult(thid, 5);
#endif
}

void logger_th(int thid) {
    ecall_logger_th(thid);
}

std::atomic<int> ocall_count(0);

// MARK: main function

int main() {
    chrono::system_clock::time_point p1, p2, p3, p4, p5;
#if SHOW_DETAILS
    displayParameter();
#endif
    p1 = chrono::system_clock::now();
    // SiloResult.resize(THREAD_NUM);
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

    uint64_t ret_durableEpoch = ecall_showDurableEpoch();
    // displayResult();

    for (int i = 0; i < THREAD_NUM; i++) {
        SiloResult[0].addLocalAllResult(SiloResult[i]);
    }
    SiloResult[0].displayAllResult();

    // std::cout << "[info]\tmakeDB:\t" << duration1/1000 << "s.\n";
    // std::cout << "[info]\tcreateThread:\t" << duration2/1000 << "s.\n";
    // std::cout << "[info]\texecutionTime:\t" << duration3/1000 << "s.\n";
    // std::cout << "[info]\tdestroyThread:\t" << duration4/1000 << "s.\n";
    // std::cout << "[info]\tcall_count(write):\t" << ocall_count.load() << std::endl;
    // std::cout << "[info]\tdurableEpoch:\t" << ret_durableEpoch << std::endl;

    std::cout << "=== for copy&paste ===" << std::endl;
    std::cout << SiloResult[0].total_commit_counts_ << std::endl;
    std::cout << SiloResult[0].total_abort_counts_ << std::endl;
    std::cout << (double)SiloResult[0].total_abort_counts_ / (double)(SiloResult[0].total_commit_counts_ + SiloResult[0].total_abort_counts_) << std::endl;
    uint64_t result = SiloResult[0].total_commit_counts_ / EXTIME;
    std::cout << powl(10.0, 9.0) / result * THREAD_NUM << std::endl;;  // latency
    std::cout << ret_durableEpoch << std::endl;
    std::cout << result << std::endl;; // throughput
    
    return 0;
}