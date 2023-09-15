#pragma once

#include <functional>
#include <memory>
#include <cstring>

#include "Timestamp.h"
#include "noncopyable.h"
#include "EventLoop.h"

namespace muduo {
namespace net {

    /**
     * Channel封装了fd
    */
    class Channel : NonCopyable {
    public:
        using EventCallback = std::function<void()>;
        using ReadEventCallback = std::function<void(Timestamp)>;

        Channel(EventLoop* loop, int fd);
        ~Channel() = default;

        void handleEvent(Timestamp receiveTime);

        void setReadCallback(ReadEventCallback cb) {
            this->readEventCallback_ = std::move(cb);            
        }
        void setWriteCallback(EventCallback cb)  {
            this->writeCallback_ = std::move(cb);
        }
        void setCloseCallback(EventCallback cb) {
            this->closeCallback_ = std::move(cb);
        }
        void setErrorCallback(EventCallback cb) {
            this->errorCallback_ = std::move(cb);
        }

        void tie(const std::shared_ptr<void>&);

        int fd() const { return this->fd_; }
        int events() const { return this->events_ ; }
        void set_revents(int revt)  { this->revents_ = revt; }
        // Channel上没有需要监听的事件
        bool isNoneEvent() const { return this->events_ == KNoneEvent; }

        void enableReading() {
            events_ |= KReadEvent;
            update();
        }
        void disableReading() {
            events_ &= ~KReadEvent;
            update();
        }
        void enableWriting() {
            events_ |= KWriteEvent;
            update();
        }
        void disableWriting() {
            events_ &= ~KWriteEvent;
            update();
        }
        void disableAll() {
            events_ = KNoneEvent;
            update();
        }
        bool isWriting() const {
            return this->events_ & KWriteEvent;
        }
        bool isReading() const {
            return this->events_ & KReadEvent;
        }

        int index() const { return this->index_; }
        void set_index(int idx) { this->index_ = idx; }

        void remove();
        EventLoop* ownerLoop() { return this->loop_; }

        std::string reventsToString() const {
            return eventsToString(fd_, revents_);
        }

        std::string eventsToString() const {
            return eventsToString(fd_, events_);
        }
    private:
        void update();
        void handleEventWithGuard(Timestamp receiveTime);
        std::string eventsToString(int fd, int event) const;

        static const int KNoneEvent;
        static const int KReadEvent;
        static const int KWriteEvent;

        EventLoop* loop_;   // 当前channel所属的loop，有可能是mainLoop，也可能是subLoop
        const int fd_;
        int events_;
        int revents_;
        int index_; // 取值为-1，1，2，标识当前channel在poller中的状态

        std::weak_ptr<void> tie_;
        bool tied_;
        bool eventHandling_;
        bool addedToLoop_;
        ReadEventCallback readEventCallback_;
        EventCallback writeCallback_;
        EventCallback closeCallback_;
        EventCallback errorCallback_;

    };
}
}