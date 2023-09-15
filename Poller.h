#pragma once

#include "EventLoop.h"
#include "Timestamp.h"

#include <unordered_map>
#include <vector>

namespace muduo {
namespace net {
    class Channel;

    /**
     * IO复用的基类
    */
    class Poller {
    public:
        using ChannelList = std::vector<Channel*>;

        Poller(EventLoop* loop);
        virtual ~Poller();

        virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

        virtual void updateChannel(Channel* channel) = 0;

        virtual void removeChannel(Channel* channel) = 0;
    
        virtual bool hasChannel(Channel* channel) const;

        static Poller* newDefaultPoller(EventLoop* loop);  

        void assertInLoopThread() const {
            loop_->assertInLoopThread();
        }  
    protected:
        using ChannelMap = std::unordered_map<int, Channel*>;
        ChannelMap channels_;    // 键为fd，值为channel   
    private:
        EventLoop* loop_;   // 记录当前poller所属的EventLoop
    };
}
}