#pragma once

#include "Poller.h"

struct epoll_event; // 必须的

namespace muduo {
    namespace net {
        class EventLoop;
        
        class EpollPoller : public Poller{
        public:
            EpollPoller(EventLoop* loop);
            ~EpollPoller() override;

            virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;

            virtual void updateChannel(Channel* channel) override;

            virtual void removeChannel(Channel* channel) override;
        private:
            int epollfd_;

            void fillActiveChannels(int numEvents, ChannelList* activeChannels);

            void update(int operation, Channel* channel);

            using EventList = std::vector<struct epoll_event>;
            static const int kInitEventListSize = 16;
            EventList events_;  // 存储就绪的事件信息，包括fd以及channel

        };
    }
}