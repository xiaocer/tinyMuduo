#pragma once

#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace muduo {
namespace net {

    class EventLoop;
    class EventLoopThread;
    class EventLoopThreadPool : NonCopyable{
    public:
        using ThreadInitCallback = std::function<void(EventLoop*)>;

        EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
        ~EventLoopThreadPool();

        void setThreadNum(int numThreads) { numThreads_ = numThreads; }
        void start(const ThreadInitCallback& cb = ThreadInitCallback());

        // 轮询算法的实现
        EventLoop* getNextLoop();

        std::vector<EventLoop*> getAllLoops();

        bool started() const { return started_; }
        const std::string& name() const { return name_; }
    private:
        EventLoop* baseLoop_;   // mainLoop
        std::string name_;
        bool started_;
        int numThreads_;
        int next_;  // 轮询算法使用的下标
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop*> loops_; // 存储每个线程创建的subLoop
    };   
}    
}