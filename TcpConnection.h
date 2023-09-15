#pragma once

#include <memory>
#include <string>
#include <atomic>

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Callbacks.h"

namespace muduo {
namespace net {
    class EventLoop;
    class Socket;
    class Channel;

    /**
     * Constructs a TcpConnection with a connected sockfd(not listen fd)
    */
    class TcpConnection : NonCopyable, 
                    public std::enable_shared_from_this<TcpConnection>{
    public:
        TcpConnection(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);

        ~TcpConnection();
        
        EventLoop* getLoop() const { return loop_; }
        const std::string& name() const { return name_; }
        const InetAddress& localAddress() const { return localAddr_; }
        const InetAddress& peerAddress() const { return peerAddr_; }
        bool connected() const { return state_ == StateE::kConnected; }
        bool disconnected() const { return state_ == StateE::kDisconnected; }
        void send(const void* message, int len);
        void send(Buffer* message);  
        void shutdown(); 
        bool isReading() const { return reading_; }
        void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }

        void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }

        void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }

        void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

        /// Advanced interface
        Buffer* inputBuffer() { return &inputBuffer_; }

        Buffer* outputBuffer() { return &outputBuffer_; }

        /// Internal use only.
        void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

        // called when TcpServer accepts a new connection
        void connectEstablished();   // should be called only once
        // called when TcpServer has removed me from its map
        void connectDestroyed();  // should be called only once

    private:
        enum class StateE {
            kDisconnected, 
            kConnecting, 
            kConnected, 
            kDisconnecting
        };
        void handleRead(Timestamp receiveTime);
        void handleWrite();
        void handleClose();
        void handleError();
        void sendInLoop(const void* message, size_t len);
        void shutdownInLoop();
        void setState(StateE s) { state_ = s; }
        EventLoop* loop_;
        const std::string name_;
        StateE state_;  // FIXME: use atomic variable
        std::atomic_bool reading_;
        std::unique_ptr<Socket> socket_;
        std::unique_ptr<Channel> channel_;
        const InetAddress localAddr_;
        const InetAddress peerAddr_;
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        HighWaterMarkCallback highWaterMarkCallback_;
        CloseCallback closeCallback_;
        size_t highWaterMark_;  // 水位线，默认64M
        Buffer inputBuffer_;    // 用户读缓冲区，暂存从Tcp内核的读缓冲区读过来的数据
        Buffer outputBuffer_;   // 用户写缓冲区,暂存用户往TCP内核的写缓冲区发送的数据
    };
    // for other class use
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
}
}