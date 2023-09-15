#include "EventLoop.h"
#include "logging.h"
#include "Poller.h"
#include "Channel.h"
#include "SocketsOps.h"

#include <thread>
#include <sys/eventfd.h>
#include <functional>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

namespace {
    thread_local EventLoop* t_loopInThisThread = nullptr;
    // 定义默认的IO复用接口超时时间
    const int kPollTimeMs = 10000;
    // 创建wakeupFd，用来唤醒subReactor用的
    int createEventfd() {
        int evfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evfd == -1) {
            LOG_FATAL("create eventfd error");
        }
        return evfd;
    }
}

EventLoop::EventLoop() 
    : looping_(false),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_( Poller::newDefaultPoller(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)),
    currentActiveChannel_(NULL) {
        LOG_INFO("event loop created in thread %d", threadId_);
        // 当前线程已经有一个EventLoop
        if (t_loopInThisThread) {
            LOG_FATAL("another EventLoop %p exists in this thread %d", t_loopInThisThread, threadId_);
        } else {
            t_loopInThisThread = this;
        }
        wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
        wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop：%p,start looping", this);

    while (!quit_) {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        eventHandling_ = true;
        for (Channel* channel : activeChannels_) {
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        // 清空
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;
        doPendingFunctors();

        LOG_INFO("EventLoop %p stop looping", this);
        looping_ = false;
    }
}

void EventLoop::quit() {
    quit_ = true;

    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        // 在非EventLoop对象所在的线程中执行cb
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

size_t EventLoop::queueSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pendingFunctors_.size();
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = sockets::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof one) {
        LOG_FATAL("wakeup error");
    }
}

// 当epoll上注册的wakeupfd感兴趣的读事件已经就绪，则需要判断是不是mainLoop向其写入的一个字节
void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = sockets::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof one) {
        LOG_FATAL("handleRead error");
    }
} 

void EventLoop::removeChannel(Channel* channel) {
    assertInLoopThread();
    if (eventHandling_) {

    }
    poller_->removeChannel(channel);
}

void EventLoop::updateChannel(Channel* channel) {
    assertInLoopThread();
    poller_->updateChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread() {
    LOG_FATAL("EventLoop::abortNotInLoopThread - EventLoop %p，was created in threadId_ %d，current thread id = %d", 
        this, threadId_, CurrentThread::tid());
}



void EventLoop::doPendingFunctors() {
    // 使用局部的functors将pendingFunctors_置换过来
    // 在执行pendingFunctors_中的回调时不影响mainLoop向pendingFunctors_添加回调
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for (const Functor& functor : functors) {
        functor();
    }
    callingPendingFunctors_ = false;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

