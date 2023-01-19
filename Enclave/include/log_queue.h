#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <map>
#include <thread>
#include <vector>

#include "../../Include/consts.h"
#include "log_buffer.h" // enqのxが不完全なクラスになるから入れないといけないっぽい

class LogBuffer;

class LogQueue {
    private:
        std::atomic<unsigned int> my_mutex_;
        std::mutex mutex_;
        std::condition_variable cv_deq_;
        std::map<uint64_t, std::vector<LogBuffer*>> queue_;
        std::size_t capacity_ = 1000;
        std::atomic<bool> quit_;
        std::chrono::microseconds timeout_;

        void my_lock() {
            for (;;) {
                unsigned int lock = 0;  // expectedとして使う
                if (my_mutex_.compare_exchange_strong(lock, 1)) return;
                waitTime_ns(30);
            }
        }

        void my_unlock() {
            my_mutex_.store(0);
        }
    
    public:
        LogQueue() {    // constructor
            my_mutex_.store(0);
            quit_.store(false);
            timeout_ = std::chrono::microseconds((int)(EPOCH_TIME*1000));
        }

        void enq(LogBuffer* x) {    //入れる方
            {
                std::lock_guard<std::mutex> lock(mutex_);   // mutex_を用いてLock, destructor時にlock解放できる
                auto &v = queue_[x->min_epoch_];
                v.emplace_back(x);
            }   // ここでmutex_のlockが解放される
            cv_deq_.notify_one();
        }

        bool wait_deq() {
            if (quit_.load() || !queue_.empty()) return true;
            std::unique_lock<std::mutex> lock(mutex_);
            return cv_deq_.wait_for(lock, timeout_, [this]{return quit_.load() || !queue_.empty();});
        }

        bool quit() {
            return quit_.load() && queue_.empty();
        }
        
        std::vector<LogBuffer*> deq() {
            std::lock_guard<std::mutex> lock(mutex_);
            size_t n_logbuf = 0;
            for (auto itr = queue_.begin(); itr != queue_.end(); itr++) {
                auto q = itr->second;
                n_logbuf += q.size();
            }
            std::vector<LogBuffer*> ret;
            ret.reserve(n_logbuf);
            size_t byte_size = 0;
            while (!queue_.empty()) {
                auto itr = queue_.cbegin();
                auto &v = itr->second;
                std::copy(v.begin(), v.end(), std::back_inserter(ret));
                queue_.erase(itr);
            }
            return ret;
        }

        bool empty() {
            return queue_.empty();
        }

        uint64_t min_epoch() {
            if (empty()) {
                return ~(uint64_t)0;
            } else {
                return queue_.cbegin()->first;
            }
        }

        void terminate() {
            quit_.store(true);
            cv_deq_.notify_all();
        }
};