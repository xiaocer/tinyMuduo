#include "Poller.h"
#include "EpollPoller.h"
#include "PollPoller.h"

#include <stdlib.h>

using namespace muduo::net;

Poller* Poller::newDefaultPoller(EventLoop* loop) {
    // 如果设置了环境变量MUDUO_USE_POLL，则IO复用使用poll
    if (::getenv("MUDUO_USE_POLL")) {
        return new PollPoller(loop);
    } else {
        return new EpollPoller(loop);
    }
}