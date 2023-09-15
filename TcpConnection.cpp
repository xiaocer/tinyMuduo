#include "Timestamp.h"
#include "TcpConnection.h"
#include "Socket.h"
#include "Channel.h"
#include "logging.h"
#include "SocketsOps.h"

#include <functional>

using namespace muduo;
using namespace muduo::net;

TcpConnection::TcpConnection(EventLoop* loop,
                const std::string& nameArg,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr) 
    : loop_(loop), // CHECK_NOTNULL(loop)
    name_(nameArg),
    state_(StateE::kConnecting),
    reading_(true),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64*1024*1024) {
        channel_->setReadCallback(
            std::bind(&TcpConnection::handleRead, this, _1)
        );
        channel_->setWriteCallback(
            std::bind(&TcpConnection::handleWrite, this)
        );
        channel_->setCloseCallback(
            std::bind(&TcpConnection::handleClose, this));
        channel_->setErrorCallback(
            std::bind(&TcpConnection::handleError, this));
        LOG_DEBUG("Tcpconnection::ctor[%s]at fd=%d", name_.c_str(), sockfd);
        socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG_DEBUG("TcpConnection::dtor[%s]at fd=%d", name_.c_str(), channel_->fd());
}

void TcpConnection::send(const void* data, int len) {
    // 已建立连接状态
    if (state_ == StateE::kConnected) {
        // 判断当前loop是否在创建loop所在的线程
        if (loop_->isInLoopThread()) {
            sendInLoop(data, len);
        } else {
            void (TcpConnection::*fp)(const void* data, size_t len) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(fp,
                            this,     // FIXME
                            data,
                            len));
                            //std::forward<string>(message)));
        }
    }
}

/**
 *发送数据 
*/

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;     // 已经发送数据的大小
    size_t remaining = len; // 待发送数据的大小
    bool faultError = false;
    if (state_ == StateE::kDisconnected) {
        LOG_ERROR("disconnected, give up writing");
        return;
    }
    // if no thing in output queue, try writing directly
    // 表示channel上的写事件就绪，而且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = sockets::write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                // 一次性完成发送数据，就不用再给channel设置EPOLLOUT事件
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else {
            // nwrote < 0
            nwrote = 0;
            // 真正发生错误
            if (errno != EWOULDBLOCK) {
                LOG_FATAL("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
            // 错误码为EWOULDBLOCK则表示发不出去了
        }
    }

    assert(remaining <= len);
    // 前面的write调用没有将应用上的数据全部发送出去，剩余的数据需要
    // 保存到缓冲区中，然后给Channel注册EPOLLOUT事件
    // 当poller发现tcp的发送缓冲区还有数据，会通知相应的Chanel，
    // 调用writeCallback方法（即本类中的handleWrite）
    if (!faultError && remaining > 0) {
        // 目前发送缓冲区中剩余的待发送数据的长度
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_) {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
        if (!channel_->isWriting()) {
            // 注册Chanel的写事件
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown() {
    // FIXME: use compare and swap
    if (state_ == StateE::kConnected) {
        setState(StateE::kDisconnecting);
        // FIXME: shared_from_this()?
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    // channel上没有可写事件，则表示缓冲区的数据已经全部发送完毕
    if (!channel_->isWriting()) {
        // we are not writing，关闭写端
        socket_->shutdownWrite();
    }
}

// 连接建立
void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == StateE::kConnecting);
    setState(StateE::kConnected);
    // channel的tie_作为TcpConnection对象的观测者
    // 防止channel在调用上层（TcpConnection）传递的回调方法时TcpConnection
    // 对象已经被销毁
    channel_->tie(shared_from_this());
    // 向poller注册channel的epollin事件
    channel_->enableReading();

    // 执行新连接建立时的回调，这个回调在TcpServer::TcpServer中指定。
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if (state_ == StateE::kConnected) {
        setState(StateE::kDisconnected);
        // 将channel所有感兴趣的事件，从poller中删除
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    // 将channel从Poller中删除,Poller中维护了一个map
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime) {
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    // 已建立连接的用户，有可读事件发生，调用用户传递的回调操作
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0) {
        handleClose();
    } else {
        // 出错
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {
        // 推荐：封装一个writeFd方法
        ssize_t n = sockets::write(channel_->fd(),
                                outputBuffer_.peek(),
                                outputBuffer_.readableBytes());
        if (n > 0)
        {
            // 这里为了维护用户缓冲区，实际数据已经发送
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {   
                    // 唤醒loop对应的线程，执行回调
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                // 正在关闭
                if (state_ == StateE::kDisconnecting)   {
                    // 内部还会判断channel是否存在感兴趣的epoolout事件
                    shutdownInLoop();
                }
            }
        }
        else {
            LOG_ERROR("TcpConnection::handleWrite");
            // if (state_ == kDisconnecting)
            // {
            //   shutdownInLoop();
            // }
        }
    }
    else {
        LOG_DEBUG("Connection fd = %d is down, no more writing", channel_->fd());
    }
}

// poller=>Channel::closeCallback=>TcpConnection::handleClose
void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    LOG_INFO("fd = %d，state = kDisconnected", channel_->fd());
    assert(state_ == StateE::kConnected || state_ == StateE::kDisconnecting);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    setState(StateE::kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);   // 执行连接关闭的回调
    // must be the last line
    closeCallback_(guardThis);        // 关闭连接的回调
}

void TcpConnection::handleError() {
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR("TcpConnection::handleError [%s]- SO_ERROR = %d,%s",
        name_.c_str(),
        err,
        muduo::strerror_tl(err));
}

void muduo::net::defaultConnectionCallback(const TcpConnectionPtr& conn) {
    LOG_DEBUG("%s -> %s,is %s",
        conn->localAddress().toIpPort().c_str(),
        conn->peerAddress().toIpPort().c_str(),
        (conn->connected() ? "UP" : "DOWN"));
}

void muduo::net::defaultMessageCallback(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp receiveTime) {
    
    buffer->retrieveAll();
}