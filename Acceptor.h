#pragma once

#include <functional>

#include "Socket.h"
#include "Channel.h"
#include "noncopyable.h"

namespace muduo {
namespace net {
    class EventLoop;
    class InetAddress;
    class Acceptor : NonCopyable{
    public:
        using NewConnectionCallback = std::function<void(int sockfd, const InetAddress& )>;
        
        Acceptor(EventLoop* mainLoop, const InetAddress& listenAddr, bool reusePort, NewConnectionCallback&& callback = NewConnectionCallback());
        ~Acceptor();

        // TcpServer::TcpServer=》setNewConnectionCallback
        void setNewConnectionCallback(const NewConnectionCallback& cb) {
            newConnectionCallback_ = cb;
        }

        void listen();
        bool listening() const { return listening_; }
        
    private:
        void handleRead();

        EventLoop* mainLoop_;   // 用户传入 =》TcpServer=》Acceptor
        Socket acceptSocket_;   // 对侦听套接字的封装
        Channel acceptChannel_;
        NewConnectionCallback newConnectionCallback_;
        bool listening_;
        int idleFd_;
    };
}
}