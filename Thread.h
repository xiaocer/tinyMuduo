#pragma once

#include <functional>
#include <string>
#include <atomic>
#include <memory>
#include <thread>

#include "noncopyable.h"

namespace muduo {
    class Thread : NonCopyable{
    public:
        using ThreadFunc = std::function<void()>;

        explicit Thread(ThreadFunc, const std::string& name = std::string());
        ~Thread();

        void start();
        void join() {
            threadPtr_->join();
        }
        
        bool started() const { return started_; }
        const std::string& name() const {  return name_;  }
        pid_t tid() const { return tid_; }
    private:
        // internal use，set name for newed thread
        void setDefaultName();

        bool started_;
        std::shared_ptr<std::thread> threadPtr_; 
        pid_t tid_;
        ThreadFunc func_;      
        std::string name_;
        static std::atomic_int numCreated_;
    };
}