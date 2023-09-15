#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

using namespace muduo;
using namespace muduo::net;

// 默认用于创建subLoop的线程数量为0个，除非客户显式通过setThreadNum方法指定
EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg)
    : baseLoop_(baseLoop),
    name_(nameArg),
    started_(false),
    numThreads_(0),
    next_(0) {
        
    }

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
    baseLoop_->assertInLoopThread();

    started_ = true;

    for (int i = 0; i < numThreads_; ++i) {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
        EventLoopThread* t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        // 创建新线程，一个线程一个subLoop
        loops_.push_back(t->startLoop());

    }

    // 客户不通过setThreadNum方法指定线程数量，则只会有一个主线程所在的mainLoop在工作
    if (numThreads_ == 0 && cb) {
        cb(baseLoop_);
    }

}

// 轮询算法的实现
EventLoop* EventLoopThreadPool::getNextLoop() {
    baseLoop_->assertInLoopThread();
    EventLoop* loop = baseLoop_;

    // 存在subLoop
    if (!loops_.empty()) {
        // 轮询算法核心
        loop = loops_[next_++];
        if (static_cast<size_t>(next_) >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    baseLoop_->assertInLoopThread();

    if (loops_.empty()) {
        return std::vector<EventLoop*>(1, baseLoop_);
    }   else {
        return loops_;
    }
}