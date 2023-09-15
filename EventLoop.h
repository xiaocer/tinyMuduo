#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

namespace muduo {
namespace net {

    class Channel;
    class Poller;
    /**
     * Reactor,one loop per thread
    */
    class EventLoop : NonCopyable{
    public:
        using Functor = std::function<void()>;

        EventLoop();
        ~EventLoop();

        void loop();
        void quit();

        void wakeup();
        Timestamp pollReturnTime() const {return pollReturnTime_;}
        void removeChannel(Channel* channel);
        void updateChannel(Channel* channel);
        bool hasChannel(Channel* channel);

        void assertInLoopThread() {
            if (!isInLoopThread()) {
                abortNotInLoopThread();
            }
        }
        bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
        bool eventHandling() const { return eventHandling_; }
        static EventLoop* getEventLoopOfCurrentThread();

        void runInLoop(Functor cb);
        // 将cb放入队列中，唤醒cb所在的线程执行cb
        void queueInLoop(Functor cb);
        size_t queueSize() const;
    private:
        void abortNotInLoopThread();
        void handleRead(); // 唤醒
        void doPendingFunctors();

        using ChannelList = std::vector<Channel*>;

        std::atomic_bool looping_;  // 控制事件循环是否进行
        std::atomic_bool quit_;     
        std::atomic_bool eventHandling_;
        std::atomic_bool callingPendingFunctors_;   // 标识当前事件循环是否有需要执行的回调操作

        const pid_t threadId_;
        Timestamp pollReturnTime_;                   // 记录从epoll_wait调用返回的时间
        std::unique_ptr<Poller> poller_;
        int wakeupFd_;                               // mainLoop唤醒subLoop使用的fd
        std::unique_ptr<Channel> wakeupChannel_;
        mutable std::mutex mutex_;

        ChannelList activeChannels_;
        Channel* currentActiveChannel_;

        std::vector<Functor> pendingFunctors_;
    };
}
}