#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#include "utility/app_util.h"
#include "logger_affinity/logger_affinity.h"
#include "../Include/consts.h"
#include "../Enclave/enclave.h"
#include "../Include/result.h"


void worker_th(int thid, int gid) {
    ecall_worker_th(thid, gid);
}

void logger_th(int thid) {
    ecall_logger_th(thid);
}

std::atomic<int> ocall_count(0);

int main() {
    DisplayResults results; // 結果を保持・表示するオブジェクト
    LoggerAffinity affin;   // ロガーとワーカースレッドのアフィニティを管理するオブジェクト
    std::vector<std::thread> logger_threads;    // ロガースレッドを保持するベクター
    std::vector<std::thread> worker_threads;    // ワーカースレッドを保持するベクター

#if SHOW_DETAILS
    results.displayParameter();
#endif
    // DB作成開始のタイムスタンプを追加する
    results.addTimestamp();

    // DBを初期化する
    ecall_initDB();

    // スレッドのアフィニティオブジェクトを初期化する
    affin.init(THREAD_NUM, LOGGER_NUM);

    // スレッド作成開始のタイムスタンプを追加する
    results.addTimestamp();

    // CPUアフィニティに基づいてロガーとワーカースレッドを作成する
    int i = 0, j = 0;
    for (auto itr = affin.nodes_.begin(); itr != affin.nodes_.end(); itr++, j++) {
        logger_threads.emplace_back(logger_th, j);    // TODO: add some arguments
        for (auto wcpu = itr->worker_cpu_.begin(); wcpu != itr->worker_cpu_.end(); wcpu++, i++) {
            worker_threads.emplace_back(worker_th, i, j);  // TODO: add some arguments
        }
    }

    // すべてのスレッドが準備完了するのを待つ
    ecall_waitForReady();

    // スレッドの実行開始のタイムスタンプを追加する
    results.addTimestamp();

    // スレッドの実行を開始し、所定の時間待つ
    ecall_sendStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * EXTIME));
    ecall_sendQuit();

    // スレッドの破壊開始のタイムスタンプを追加する
    results.addTimestamp();

    // すべてのロガーとワーカースレッドが完了するのを待つ
    for (auto &th : logger_threads) th.join();
    for (auto &th : worker_threads) th.join();

    // スレッドの破壊終了のタイムスタンプを追加する
    results.addTimestamp();
    
    // ワーカーとロガースレッドからの結果を収集し、表示する
    results.getWorkerResult();
    results.getLoggerResult();
    results.displayResult();
    
    uint64_t ret_durableEpoch = ecall_showDurableEpoch();



    return 0;
}