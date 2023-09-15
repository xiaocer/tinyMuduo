#include "Channel.h"
#include "logging.h"

#include <poll.h>
#include <sstream>

using namespace muduo;
using namespace muduo::net;


const int Channel::KNoneEvent = 0;
const int Channel::KReadEvent = POLLIN | POLLPRI;
const int Channel::KWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd) : 
    loop_(loop),
    fd_(fd),
    events_(0),
    revents_(0),
    index_(-1),
    tied_(false),
    eventHandling_(false),
    addedToLoop_(false) {
    
}

// 当poller监听的文件描述符就绪则，EventLoop::loop =》 handleEvent
void Channel::handleEvent(Timestamp receiveTime) {
    std::shared_ptr<void> guard;
    if (tied_) {
        guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
    }
    else {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime) {
    this->eventHandling_ = true;

    LOG_INFO("fd:%d，event:%s", fd_, this->reventsToString().c_str());
    // 对端关闭
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        if (closeCallback_) closeCallback_();
    }

    if (revents_ & POLLNVAL) {
        LOG_ERROR("fd = %d, Channel::handleEventWithGuard POLLNVAL", this->fd_);
    }
    
    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) errorCallback_();
    }

    if (revents_ & (POLLIN || POLLPRI || POLLRDHUP)) {
        if (readEventCallback_) readEventCallback_(receiveTime);
    }

    if (revents_ & POLLOUT) {
        if (writeCallback_) writeCallback_();
    }
    this->eventHandling_ = false;
}

void Channel::tie(const std::shared_ptr<void>& obj) {
    this->tie_ = obj;
    this->tied_ = true;
}

void Channel::remove() {
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

void Channel::update() {
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

// 
// 获取事件的名称
std::string Channel::eventsToString(int fd, int ev) const{
    std::ostringstream oss;

    oss << fd << ": ";
    if (ev & POLLIN)
        oss << "IN ";
    if (ev & POLLPRI)
        oss << "PRI ";
    if (ev & POLLOUT)
        oss << "OUT ";
    if (ev & POLLHUP)
        oss << "HUP ";
    if (ev & POLLRDHUP)
        oss << "RDHUP ";
    if (ev & POLLERR)
        oss << "ERR ";
    // POLLNVAL表示fd已经关闭，这个标志只有在返回就绪的文件描述符时使用
    if (ev & POLLNVAL)
        oss << "NVAL ";

    return oss.str();
}