#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <set>

#include "random.h"

#include "tpcc/tpcc_query.hh"
#include "tpcc/tpcc_tables.hh"
#include "tpcc/tpcc_tx_neworder.hh"
#include "tpcc/tpcc_tx_payment.hh"

template <typename Tuple, typename Param>
class TPCCWorkload {
public:
    Param* param_;
    Xoroshiro128Plus rnd_;
    // HistoryKeyGenerator hkg_;  // CHECK: 何に使うの？
    uint16_t w_id;  // home warehouse

    TPCCWorkload() {
        rnd_.init();
    }

    // CHECK: これ使ってないかもしれない
    class TxArgs {
    public:
        void generate(TPCCWorkload *w, TxType type) {
            // case TxType::xxx:
            // break;
            default:
                // TODO: Error handling
                break;
        }
    };

    void prepare(TxExecutor &tx, Param *p) {
        hkg_.init(tx.thid_, true);
        w_id = (tx.thid_ % TPCC_NUM_WH) + 1;    // CHECK: つまりprepareで自分がどのwarehouseを担当するかを決めるってこと？
    }

    template <typename TxExecutor, typename TransactionStatus>
    void run(TxExecutor &tx) {
        Query query;
        TPCCQuery::Option option;   // query typeのthresholdを決めている

RETRY:
        query.generate(w_id, option);

        if (tx.isLeader()) {
            tx.leaderWork();
        }

        if (loadAcquire(tx.quit_)) return;

        tx.begin();

        switch (query.type) {
            case TxType::NewOrder:
                if (!run_new_order<TxExecutor, TransactionStatus>(tx, &query.new_order)) {
                    tx.status_ = TransactionStatus::aborted;
                }
                break;
            case TxType::Payment:
                if (!run_payment<TxExecutor, TransactionStatus, Tuple>(tx, &query.payment, &hkg_)) {
                    tx.status_ = TransactionStatus::aborted;
                }
                break;
            default:
                // TODO: Error handling
                break;
        }

        if (tx.status_ == TransactionStatus::aborted) {
            tx.abort();
            tx.result_->local_abort_counts_++;
            goto RETRY;
        }

        if (!tx.commit()) {
            tx.abort();
            if (tx.status_ == TransactionStatus::invalid) return;
            tx.result_->local_abort_counts_++;
            // TODO: add local_abort_counts_per_tx_++
        }

        if (loadAcquire(tx.quit_)) return;
        tx.result_->local_commit_counts_;
        // TODO: add local_commit_counts_per_tx++

        return;
    }

    // 関数にstaticをつけるとファイル内だけのローカル関数になるらしい, というかこれいつ使う？
    static uint32_t getTableNum() {
        return (uint32_t)Storage::size;
    }

    static void makeDB([[maybe_unused]] Param *param) {
        Xoroshiro128Plus rand;
        rand.init();
        TPCCInitializer<Tuple, Param>::load(param);
    }

    // TODO: display parameterとdisplay workloadresultをここに実装するべきなのか？


};