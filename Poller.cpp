#include "Poller.h"
#include "Channel.h"

using namespace muduo::net;

Poller::Poller(EventLoop* loop) : loop_(loop){

}
Poller::~Poller() = default;

bool Poller::hasChannel(Channel* channel) const {
    // 判断当前loop是否在创建loop的线程中
    assertInLoopThread();
    auto it = channels_ .find(channel->fd());
    return it != channels_.end() && it->second == channel;
}