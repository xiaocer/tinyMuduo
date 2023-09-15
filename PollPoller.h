#pragma once

#include "Poller.h"

namespace muduo {
namespace net {
    class EventLoop;

    /**
     * IO复用之poll，TODO
    */
    class PollPoller : public Poller{
    public:
        PollPoller(EventLoop* loop);

        virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);

        virtual void updateChannel(Channel* channel);

        virtual void removeChannel(Channel* channel);
    };
}
}