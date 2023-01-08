#pragma once

#include <vector>

#include "silo_op_element.h"
#include "procedure.h"

enum class TransactionStatus : uint8_t {
    InFlight,
    Committed,
    Aborted,
};

class TxExecutor {
    public:
        std::vector<ReadElement> read_set_;
        std::vector<WriteElement> write_set_;
        std::vector<Procedure> pro_set_;

        //TODO: log?
        TransactionStatus status_;
        unsigned int thid_;
        TIDword mrctid_;
        TIDword max_rset_, max_wset_;

        uint64_t write_val_;
        uint64_t return_val_;

        TxExecutor(int thid);

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

};