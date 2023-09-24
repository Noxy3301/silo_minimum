# include "app_util.h"

/**
 * @brief タイムスタンプの追加
 * 
 * 現在のシステム時刻をタイムスタンプとしてベクターに追加する
 */
void DisplayResults::addTimestamp() {
    timestamps.emplace_back(std::chrono::system_clock::now());
}

/**
 * @brief 各種のシステムパラメータを表示する。
 * 
 * @param CLOCKS_PER_US ターゲットハードウェアにおけるマイクロ秒ごとのクロック数
 * @param EPOCH_TIME    エポックの期間（ミリ秒単位）
 * @param EXTIME        実行時間（秒単位）
 * @param MAX_OPE       操作の最大数
 * @param RRAITO        読み取り操作の比率（パーセンテージ）
 * @param THREAD_NUM    使用されるスレッドの総数
 * @param TUPLE_NUM     タプルの総数
 * @param YCSB          YCSB (Yahoo! Cloud Serving Benchmark) を使用するかどうかを示すフラグ
 * @param ZIPF_SKEW     Zipf分布の歪度（0は一様分布を示す）
 * @param LOGGER_NUM    ロガースレッドの数
 * @param BUFFER_NUM    バッファの数
 * @param BUFFER_SIZE   各バッファのサイズ（バイト単位）
 */
void DisplayResults::displayParameter() {
    std::cout << "#clocks_per_us:\t"<< CLOCKS_PER_US<< std::endl;
    std::cout << "#epoch_time:\t"   << EPOCH_TIME   << std::endl;
    std::cout << "#extime:\t"       << EXTIME       << std::endl;
    std::cout << "#max_ope:\t"      << MAX_OPE      << std::endl;
    std::cout << "#rratio:\t"       << RRAITO       << std::endl;
    std::cout << "#thread_num:\t"   << THREAD_NUM   << std::endl;
    std::cout << "#tuple_num:\t"    << TUPLE_NUM    << std::endl;
    std::cout << "#ycsb:\t\t"       << YCSB         << std::endl;
    std::cout << "#zipf_skew:\t"    << ZIPF_SKEW    << std::endl;
    std::cout << "#logger_num:\t"   << LOGGER_NUM   << std::endl;
    std::cout << "#buffer_num:\t"   << BUFFER_NUM   << std::endl;
    std::cout << "#buffer_size:\t"  << BUFFER_SIZE  << std::endl;
}

/**
 * @brief 複数のスレッドによる実行結果を集計し、結果を表示する
 * 
 * 各スレッドによるコミット数、アボート数、各アボートの理由に応じたアボート数を集計する
 * SHOW_DETAILS が有効な場合、詳細な結果も表示される
 * 
 * @param SiloResult 各スレッドの実行結果を保持する配列
 * @param THREAD_NUM スレッドの総数
 * @param total_commit_counts_ 各スレッドのコミット数の合計
 * @param total_abort_counts_ 各スレッドのアボート数の合計
 * @param total_abort_res_counts_ 各アボートの理由に応じたアボート数の合計
 * @param EXTIME 実行時間（秒単位）
 */
void DisplayResults::displayResult() {
    // ====================
    // logger result
    // ====================
    double cps = CLOCKS_PER_US*1e6;
    for (size_t i = 0; i < LOGGER_NUM; i++) {
    #if SHOW_DETAILS
        std::cout << ("Logger#" + std::to_string(i))
            << " byte_count[B]="    << loggerResults[i].byte_count_
            << " write_latency[s]=" << loggerResults[i].write_latency_/cps
            << " throughput[B/s]="  << loggerResults[i].byte_count_/(loggerResults[i].write_latency_/cps)
            << " wait_latency[s]="  << loggerResults[i].wait_latency_/cps
        << std::endl;
    #endif
    }
    // ====================
    // worker result
    // ====================
    // TODO: tablePrinterみたいなやつを作る
    std::cout << std::left
        << std::setw(8) << "Thread#"
        << "| " << std::setw(12) << "Commit"
        << "| " << std::setw(12) << "Abort"
        << "| " << std::setw(8) << "VP1"
        << "| " << std::setw(8) << "VP2"
        << "| " << std::setw(8) << "VP3"
        << "| " << std::setw(8) << "NullB"
    << std::endl;
    std::cout << std::string(76, '-') << std::endl; // 区切り線

    for (size_t i = 0; i < workerResults.size(); i++) {
    #if SHOW_DETAILS
        // 各ワーカースレッドのcommit/abort数と、abort理由(vp1/vp2/vp3/nullBuffer)を表示する
        std::cout << std::left 
            << std::setw(8) << ("#" + std::to_string(i)) 
            << "| " << std::setw(12) << workerResults[i].local_commit_count_ 
            << "| " << std::setw(12) << workerResults[i].local_abort_count_ 
            << "| " << std::setw(8)  << workerResults[i].local_abort_vp1_count_ 
            << "| " << std::setw(8)  << workerResults[i].local_abort_vp2_count_ 
            << "| " << std::setw(8)  << workerResults[i].local_abort_vp3_count_ 
            << "| " << std::setw(8)  << workerResults[i].local_abort_nullBuffer_count_ 
        << std::endl;
    #endif
        // 各ワーカースレッドの数値の総和を計算する
        total_commit_count_           += workerResults[i].local_commit_count_;
        total_abort_count_            += workerResults[i].local_abort_count_;
        total_abort_vp1_count_        += workerResults[i].local_abort_vp1_count_;
        total_abort_vp2_count_        += workerResults[i].local_abort_vp2_count_;
        total_abort_vp3_count_        += workerResults[i].local_abort_vp3_count_;
        total_abort_nullBuffer_count_ += workerResults[i].local_abort_nullBuffer_count_;
    }
    #if SHOW_DETAILS
    // 算出した総和の数を元にresultを表示する
    double abort_rate = (double)total_abort_count_ / (double)(total_commit_count_ + total_abort_count_);
    uint64_t throughput = total_commit_count_ / EXTIME;
    long double latency = powl(10.0, 9.0) / throughput * THREAD_NUM;
    std::cout << "===== Transaction Protocol Performance ====="                     << std::endl;
    std::cout << "[info]\tcommit_counts_:\t"       << total_commit_count_           << std::endl;
    std::cout << "[info]\tabort_counts_:\t"        << total_abort_count_            << std::endl;
    std::cout << "[info]\t├─ abort_validation1:\t" << total_abort_vp1_count_        << std::endl;
    std::cout << "[info]\t├─ abort_validation2:\t" << total_abort_vp2_count_        << std::endl;
    std::cout << "[info]\t├─ abort_validation3:\t" << total_abort_vp3_count_        << std::endl;
    std::cout << "[info]\t└─ abort_NULLbuffer:\t"  << total_abort_nullBuffer_count_ << std::endl;
    std::cout << "[info]\tabort_rate:\t"           << throughput                    << std::endl;
    std::cout << "[info]\tlatency[ns]:\t"          << latency                       << std::endl;
    std::cout << "[info]\tthroughput[tps]:\t"      << throughput                    << std::endl;

    assert(timestamps.size() == 5);
    double makedb_time    = calculateDurationTime_ms(0, 1);
    double createth_time  = calculateDurationTime_ms(1, 2);
    double ex_time        = calculateDurationTime_ms(2, 3);
    double destroyth_time = calculateDurationTime_ms(3, 4);

    std::cout << "===== Enclave Performance ====="                             << std::endl;
    std::cout << "[info]\tmakeDB:\t"            << makedb_time/1000    << "s." << std::endl;
    std::cout << "[info]\tcreateThread:\t"      << createth_time/1000  << "s." << std::endl;
    std::cout << "[info]\texecutionTime:\t"     << ex_time/1000        << "s." << std::endl;
    std::cout << "[info]\tdestroyThread:\t"     << destroyth_time/1000 << "s." << std::endl;
    // std::cout << "[info]\tcall_count(write):\t" << ocall_count.load()          << std::endl;
    // std::cout << "[info]\tdurableEpoch:\t"      << ret_durableEpoch            << std::endl;
    #endif
    std::cout << "=== for copy&paste ===" << std::endl;
    std::cout << total_commit_count_      << std::endl; // commit count
    std::cout << total_abort_count_       << std::endl; // abort count
    std::cout << abort_rate               << std::endl; // abort rate
    std::cout << latency                  << std::endl; // latency
    // std::cout << ret_durableEpoch         << std::endl; // durable epoch
    std::cout << throughput               << std::endl; // throughput
}

/**
 * @brief 指定された開始時間と終了時間のインデックスに基づいて、経過時間をミリ秒単位で計算する
 * 
 * @param startTimeIndex 開始時間のインデックス
 * @param endTimeIndex 終了時間のインデックス
 * @return 経過時間（ミリ秒単位）
 * 
 * @note 関数は、内部で保持されているtimestampsベクトルを使用して時間の差を計算する
 *       また、指定されたインデックスが適切な範囲にあることを確認するためのアサーションも含まれている
 */
double DisplayResults::calculateDurationTime_ms(size_t startTimeIndex, size_t endTimeIndex) {
    assert(startTimeIndex < endTimeIndex);
    assert(endTimeIndex <= timestamps.size());
    return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(timestamps[endTimeIndex] - timestamps[startTimeIndex]).count() / 1000.0);
}

void DisplayResults::getWorkerResult() {
    for (size_t i = 0; i < THREAD_NUM; i++) {
        workerResults[i].local_commit_count_           = ecall_getCommitCount(i);
        workerResults[i].local_abort_count_            = ecall_getAbortCount(i);
        workerResults[i].local_abort_vp1_count_        = ecall_getSpecificAbortCount(i, ValidationPhase1);
        workerResults[i].local_abort_vp2_count_        = ecall_getSpecificAbortCount(i, ValidationPhase2);
        workerResults[i].local_abort_vp3_count_        = ecall_getSpecificAbortCount(i, ValidationPhase3);
        workerResults[i].local_abort_nullBuffer_count_ = ecall_getSpecificAbortCount(i, NullCurrentBuffer);
    }
}

void DisplayResults::getLoggerResult() {
    for (size_t i = 0; i < LOGGER_NUM; i++) {
        uint64_t byte_count    = ecall_getLoggerCount(i, LoggerResultType::ByteCount);
        uint64_t write_latency = ecall_getLoggerCount(i, LoggerResultType::WriteLatency);
        uint64_t wait_latency  = ecall_getLoggerCount(i, LoggerResultType::WaitLatency);
        loggerResults[i].byte_count_    = byte_count;
        loggerResults[i].write_latency_ = write_latency;
        loggerResults[i].wait_latency_  = wait_latency;
    }
}