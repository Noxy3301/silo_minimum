#include <iostream>
#include <vector>

#include "../Include/consts.h"
#include "../Include/result.h"

using namespace std;

void Result::displayAbortCounts() {
    cout << "abort_counts_:\t" << total_abort_counts_ << endl;
}

void Result::displayCommitCounts() {
    cout << "commit_counts_:\t" << total_commit_counts_ << endl;
}

void Result::displayAllResult() {
    displayCommitCounts();
    displayAbortCounts();
}



void Result::addLocalAbortCounts(const uint64_t count) {
    total_abort_counts_ += count;
}

void Result::addLocalCommitCounts(const uint64_t count) {
    total_commit_counts_ += count;
}

void Result::addLocalAllResult(const Result &other) {
    addLocalAbortCounts(other.local_abort_counts_);
    addLocalCommitCounts(other.local_commit_counts_);
}