#include "EventLoopThread.h"
#include "EventLoop.h"

#include <functional>

using namespace muduo;
using namespace muduo::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                    const std::string& name) 
    : loop_(nullptr), 
    exiting_(false),
    thread_(std::bind(&EventLoopThread::threadFunc, this), name),
    callback_(cb) {

}
EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}   
EventLoop* EventLoopThread::startLoop() {
    thread_.start();    // 创建新线程，线程中执行EventLoop中指定的threadFunc
    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr) {
            condition_.wait(lock);  // 等待每个线程初始化好EventLoop
        }
        loop = loop_;
    }
    return loop;
}

// 每个新创建的线程都会执行下面这个函数
void EventLoopThread::threadFunc() {
    EventLoop loop;
    // callback_的传递：客户=》TcpServer=》EventLoopThreadPool =》EventLoopThread
    if (callback_) {
        callback_(&loop);
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        condition_.notify_one();
    }
    loop.loop();
    std::lock_guard<std::mutex> lock(mutex_);
    // 防止野指针
    loop_ = NULL;
}