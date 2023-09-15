#include "EpollPoller.h"
#include "logging.h"
#include "Channel.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <strings.h>

using namespace muduo;
using namespace muduo::net;

const int KNew = -1;    // channel未添加到poller维护的ChannelMap中
const int KAdded = 1;   // channel已经添加到poller维护的ChannelMap中
const int KDeleted = 2; // channel从poller维护的ChannelMap中删除

// EPOLL_CLOEXEC表示创建的文件描述符包含close-on-exec标志
EpollPoller::EpollPoller(EventLoop* loop) : Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize) {
    // 创建epoll实例失败
    if (epollfd_ < 0) {
        LOG_FATAL("epoll_create1 error:%d", epollfd_);
    }
}

EpollPoller::~EpollPoller() {
    ::close(epollfd_);    
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    LOG_INFO("fd total count:%d", static_cast<int>(channels_.size()));
    int numEvents = ::epoll_wait(epollfd_, 
                                &*events_.begin(),
                                static_cast<int>(events_.size()),
                                timeoutMs);
    
    int savedErrno = errno;
    Timestamp now(Timestamp::now());

    // 存在就绪的文件描述符
    if (numEvents > 0) {
        LOG_INFO("events happened，number:%d", numEvents);
        // activeChannels是一个传入传出参数，EventLoop::loop中获取存在就绪文件描述符的channel
        fillActiveChannels(numEvents, activeChannels);
        if (static_cast<int>(numEvents) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_DEBUG("nothing happened");
    } else {
        // 真的发生错误
        if (savedErrno != EINTR) {
            errno = savedErrno;
            LOG_ERROR("errno:%d", errno);
        }
    }
    return now;
}

void EpollPoller::updateChannel(Channel* channel)  {
    Poller::assertInLoopThread();
    const int index = channel->index();  // 获取channel在Poller中的状态
    int fd = channel->fd();

    if (index == KNew || index == KDeleted) {
        
        if (index == KNew) {
            channels_[fd] = channel;
        } else {

        }
        channel->set_index(KAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(KDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }

    }
}

// 从Poller中维护的ChannelMap删除channel
void EpollPoller::removeChannel(Channel* channel)  {
    Poller::assertInLoopThread();
    int fd = channel->fd();
    int index = channel->index();

    channels_.erase(fd);

    if (index == KAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(KNew);
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) {
    for (int i = 0; i < numEvents; ++i) {
        // 取出包含就绪事件的channel
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EpollPoller::update(int operation, Channel* channel) {
    struct epoll_event event;
    bzero(&event, sizeof(event));

    event.events = channel->events(); // 委托内核需要监听的事件
    event.data.ptr = channel;       // 额外带的数据
    int fd = channel->fd();

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR("operation:EPOLL_CTL_DEL failed");
        } else {
            // EPOLL_CTL_ADD 以及EPOLL_CTL_MOD操作失败则终止程序
            LOG_FATAL("operation:EPOLL_CTL_ADD,EPOLL_CTL_MOD failed");
        }
    }
}