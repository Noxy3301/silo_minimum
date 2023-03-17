#pragma once

#include <vector>

#include "silo_op_element.h"
#include "procedure.h"
#include "log_buffer.h"
#include "notifier.h"

#include "../../Include/result.h"

enum class TransactionStatus : uint8_t {
    invalid,
    inFlight,
    committed,
    aborted,
};

class TxExecutor {
    public:
        // operation sets
        std::vector<ReadElement> read_set_;
        std::vector<WriteElement> write_set_;
        std::vector<Procedure> pro_set_;
        
        // for logging
        std::vector<LogRecord> log_set_;
        LogHeader latest_log_header_;
        
        // transaction status 
        TransactionStatus status_;
        unsigned int thid_;
        Result *result_;
        uint64_t epoch_timer_start, epoch_timer_stop;
        const bool& quit_; // for thread termination control
        
        // for calcurate TID
        Tidword mrctid_;
        Tidword max_rset_, max_wset_;

        
        uint64_t write_val_;
        uint64_t return_val_;

        // for logging
        LogBufferPool log_buffer_pool_;
        NotificationId nid_;
        std::uint32_t nid_counter_ = 0; // Notification ID
        int logger_thid_;

        // Abort reason status
        // 0 : Commit
        // 1 : Aborted by failed validation phase 1 (write-write conflict)
        // 2 : Aborted by failed validation phase 2 (TID of read-set already changed)
        // 3 : Aborted by failed validation phase 3 (tuple is locked but it isn't included by its write-set)
        // 4 : Aborted by failure to set current buffer 
        int abort_res_ = 0;

        TxExecutor(int thid, Result *res, const bool &quit) : thid_(thid), result_(res), quit_(quit) {
            read_set_.reserve(MAX_OPE);
            write_set_.reserve(MAX_OPE);
            pro_set_.reserve(MAX_OPE);

            max_rset_.obj_ = 0;
            max_wset_.obj_ = 0;
            epoch_timer_start = rdtscp();
            epoch_timer_stop = 0;
        }

        void begin();
        void read(uint64_t key);
        void write(uint64_t key, uint64_t val = 0);
        ReadElement *searchReadSet(uint64_t key);
        WriteElement *searchWriteSet(uint64_t key);
        void unlockWriteSet();
        void unlockWriteSet(std::vector<WriteElement>::iterator end);
        void lockWriteSet();
        bool validationPhase();
        // TODO void wal(std::uint64_t ctid);
        void writePhase();
        void abort();
    
        void wal(std::uint64_t ctid);
        bool pauseCondition();
        void epochWork(uint64_t &epoch_timer_start, uint64_t &epoch_timer_stop);
        void durableEpochWork(uint64_t &epoch_timer_start, uint64_t &epoch_timer_stop, const bool &quit);

};