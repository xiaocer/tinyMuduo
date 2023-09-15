#pragma once

#include <functional>
#include <string>
#include <mutex>
#include <condition_variable>

#include "noncopyable.h"
#include "Thread.h"

namespace muduo {
namespace net {
    class EventLoop;
    class EventLoopThread : NonCopyable {
    public:
        using ThreadInitCallback = std::function<void(EventLoop*)>;

        EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                    const std::string& name = std::string());
        ~EventLoopThread();
        EventLoop* startLoop();

    private:
        void threadFunc();

        EventLoop* loop_;
        bool exiting_;
        Thread thread_;

        std::mutex mutex_;
        std::condition_variable condition_;

        ThreadInitCallback callback_;   // 线程初始化完后的回调
    };
}
}