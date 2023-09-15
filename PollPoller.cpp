#include "PollPoller.h"

using namespace muduo;
using namespace muduo::net;

PollPoller::PollPoller(EventLoop* loop) : Poller(loop) {

}

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    return Timestamp();
}

void PollPoller::updateChannel(Channel* channel) {

}

void PollPoller::removeChannel(Channel* channel) {

}