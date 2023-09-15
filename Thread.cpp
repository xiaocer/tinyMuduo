#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>
#include <strings.h>

using namespace muduo;

// 静态变量的初始化
std::atomic_int Thread::numCreated_(0);

thread_local int muduo::CurrentThread::t_cachedTid = 0;

Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false),
    tid_(0),
    func_(std::move(func)),
    name_(name) {
    setDefaultName();
}
Thread::~Thread() {
    if (started_) {
        threadPtr_->detach();
    }
}

void Thread::start() {
    started_ = true;

    sem_t sem;
    ::sem_init(&sem, false, 0);
    
    // 开启线程
    threadPtr_ = std::shared_ptr<std::thread>(new std::thread([&]() {
        tid_ = CurrentThread::tid();
        sem_post(&sem);
        // 执行线程函数
        func_();
    }));

    // 等待新创建的线程获取到tid_值
    sem_wait(&sem);
}

void Thread::setDefaultName() {
    int num = numCreated_++;
    if (name_.empty()) {
        char buf[32];
        bzero(buf, sizeof(buf));
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}