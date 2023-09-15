#pragma once

#include <memory>
#include <functional>

namespace muduo {
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    class Timestamp;
namespace net {
    class TcpConnection;
    class Buffer;
    

    using TcpConnectionPtr =  std::shared_ptr<TcpConnection>;
    using ConnectionCallback = std::function<void (const TcpConnectionPtr&)>;
    using CloseCallback = std::function<void (const TcpConnectionPtr&)>;
    using WriteCompleteCallback = std::function<void (const TcpConnectionPtr&)>;
    using HighWaterMarkCallback = std::function<void (const TcpConnectionPtr&, size_t)>;

    // the data has been read to (buf, len)
    using MessageCallback = std::function<void (const TcpConnectionPtr&,
                                Buffer*,
                                Timestamp)>;

    void defaultConnectionCallback(const TcpConnectionPtr& conn);
    void defaultMessageCallback(const TcpConnectionPtr& conn,
                                Buffer* buffer,
                                Timestamp receiveTime);
}
}