#pragma once

#include <cstdint>

class Result {
    public:
        uint64_t local_abort_counts_ = 0;
        uint64_t local_commit_counts_ = 0;
        uint64_t total_abort_counts_ = 0;
        uint64_t total_commit_counts_ = 0;
};
