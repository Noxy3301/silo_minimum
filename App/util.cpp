#include "include/util.h"

#include <iostream>

using namespace std;

// defineで定義しているパラメータを表示する
void displayParameter() {
    cout << "#clocks_per_us:\t" << CLOCKS_PER_US << endl;
    cout << "#epoch_time:\t" << EPOCH_TIME << endl;
    cout << "#extime:\t" << EXTIME << endl;
    cout << "#max_ope:\t" << MAX_OPE << endl;
    // cout << "#rmw:\t\t" << RMW << endl;
    cout << "#rratio:\t" << RRAITO << endl;
    cout << "#thread_num:\t" << THREAD_NUM << endl;
    cout << "#tuple_num:\t" << TUPLE_NUM << endl;
    // cout << "#ycsb:\t\t" << YCSB << endl;
    cout << "#zipf_skew:\t" << ZIPF_SKEW << endl;
    cout << "#logger_num:\t" << LOGGER_NUM << endl;
    cout << "#buffer_num:\t" << BUFFER_NUM << endl;
    cout << "#buffer_size:\t" << BUFFER_SIZE << endl;
}


void displayResult() {
    for (int i = 0; i < THREAD_NUM; i++) {
#if SHOW_DETAILS
        cout << 
        "thread#" << i << 
        "\tcommit: " << SiloResult[i].local_commit_counts_ << 
        "\tabort:" << SiloResult[i].local_abort_counts_ << 
        "\tabort_VP1: " << SiloResult[i].local_abort_res_counts_[0] <<  // aborted by validation phase 1
        "\tabort_VP2: " << SiloResult[i].local_abort_res_counts_[1] <<  // aborted by validation phase 2
        "\tabort_VP3: " << SiloResult[i].local_abort_res_counts_[2] <<  // aborted by validation phase 3
        "\tabort_bNULL: " << SiloResult[i].local_abort_res_counts_[3] <<// aborted by NULL current buffer    
        endl;
#endif
        // total_commit_counts_ += SiloResult[i].local_commit_counts_;
        // total_abort_counts_ += SiloResult[i].local_abort_counts_;
        // tocal_abort_res_counts_[0] += SiloResult[i].local_abort_res_counts_[0];
        // tocal_abort_res_counts_[1] += SiloResult[i].local_abort_res_counts_[1];
        // tocal_abort_res_counts_[2] += SiloResult[i].local_abort_res_counts_[2];
        // tocal_abort_res_counts_[3] += SiloResult[i].local_abort_res_counts_[3];

    }
    
#if SHOW_DETAILS
    cout << "[info]\tcommit_counts_:\t" << total_commit_counts_ << endl;
    cout << "[info]\tabort_counts_:\t" << total_abort_counts_ << endl;
    cout << "[info]\t-abort_validation1:\t" << tocal_abort_res_counts_[0] << endl;
    cout << "[info]\t-abort_validation2:\t" << tocal_abort_res_counts_[1] << endl;
    cout << "[info]\t-abort_validation3:\t" << tocal_abort_res_counts_[2] << endl;
    cout << "[info]\t-abort_NULLbuffer:\t" << tocal_abort_res_counts_[3] << endl;

    cout << "[info]\tabort_rate:\t" << (double)total_abort_counts_ / (double)(total_commit_counts_ + total_abort_counts_) << endl;

    uint64_t result = total_commit_counts_ / EXTIME;
    cout << "[info]\tlatency[ns]:\t" << powl(10.0, 9.0) / result * THREAD_NUM << endl;
    cout << "[info]\tthroughput[tps]:\t" << result << endl;
#endif
}