#pragma once

#include "noncopyable.h"
#include "TcpConnection.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "Acceptor.h"
#include "Buffer.h"


#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <atomic>

namespace muduo {
namespace net {
    /**
     * 客户使用，支持单线程/多线程
    */
    class TcpServer : NonCopyable{
    public:
        using ThreadInitCallback = std::function<void(EventLoop*)>;
        enum class Option {
            KNoReusePort,
            KReusePort,
        };

        TcpServer(EventLoop* mainLoop, 
            const InetAddress& listenAddr,
            const std::string& nameArg,
            Option option = Option::KNoReusePort);
        ~TcpServer();

        const std::string& ipPort() const { return ipPort_ ; }
        const std::string& name() const { return name_; }
        EventLoop* getMainLoop() const { return mainLoop_; }

        // 设置subLoop的个数，这个方法需要在start方法前调用
        void setThreadNum(int numThreads);
        void setThreadInitCallback(const ThreadInitCallback& cb) {
            threadInitCallback_ = cb;
        }
        std::shared_ptr<EventLoopThreadPool> threadPool() const {
            return threadPool_;
        }

        void start();

        void setConnectionCallback(const ConnectionCallback& cb) {
            connectionCallback_ = cb;
        }
        void setMessageCallback(const MessageCallback& cb) {
            messageCallback_ = cb;
        }
        void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
            writeCompleteCallback_ = cb;
        }

    private:

        void newConnection(int sockfd, const InetAddress& peerAddr);
        void removeConnection(const TcpConnectionPtr& conn);
        void removeConnectionInLoop(const TcpConnectionPtr& conn);
        
        EventLoop* mainLoop_ ;   // mainLoop
        const std::string ipPort_;
        const std::string name_;
        std::unique_ptr<Acceptor> acceptor_;
        std::shared_ptr<EventLoopThreadPool> threadPool_;
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        ThreadInitCallback threadInitCallback_;
        std::atomic_int started_;
        int nextConnId_;

        using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
        ConnectionMap connections_;
    };
}
}