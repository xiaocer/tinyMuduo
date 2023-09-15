#include "TcpServer.h"

#include "logging.h"
#include "SocketsOps.h"

using namespace muduo;
using namespace muduo::net;

TcpServer::TcpServer(EventLoop* mainLoop, 
            const InetAddress& listenAddr,
            const std::string& nameArg,
            Option option)
    : mainLoop_(mainLoop),
    ipPort_(listenAddr.toIpPort()),
    name_(nameArg),
    acceptor_(new Acceptor(mainLoop, listenAddr, static_cast<bool>(option))),
    threadPool_(new EventLoopThreadPool(mainLoop, nameArg)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),
    started_(0),
    nextConnId_(1)  {
    // 设置一个回调：当有新用户的连接请求时调用
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}
TcpServer::~TcpServer() {
    mainLoop_->assertInLoopThread();

    LOG_INFO("TcpServer:[%s] destructing", name_.c_str());

    for (auto& item : connections_) {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads) {
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() {
    // 防止一个TcpServer对象被start多次
    if (started_.load() == 0) {
        // 客户指定了numThreads_，则创建子线程，一个线程对应一个subLoop，否则还是一个mainLoop
        threadPool_->start(threadInitCallback_);
    }
    mainLoop_->runInLoop(
        std::bind(&Acceptor::listen, acceptor_.get()));    
}


// 每当有一个新的客户端连接请求，都会调用这个回调
// Channel::handleEvent =》Acceptor::handleRead =》 newConnection
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
    mainLoop_->assertInLoopThread();
    // 轮询算法选定的一个ioLoop
    EventLoop* subLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("new connection %s from %s", connName.c_str(), peerAddr.toIpPort().c_str());

    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(subLoop, 
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));
    connections_[connName] = conn;
    // 下面的回调的调用路径
    // 用户=》TcpServer=》TcpConnection=》Channel=》Poller
    // =>notify =>Channel调用回调

    // 设置业务层面上，连接建立时的回调。例如TcpConnection的connectEstablished方法内调用
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe
    subLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));

}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    mainLoop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    mainLoop_->assertInLoopThread();
    LOG_INFO("TcpServer::removeConnectionInLoop [%s]- connection[%s] ",
        name_.c_str(),
        conn->name().c_str()
        );
    size_t n = connections_.erase(conn->name());
    // 防止编译器警告变量n未使用
    (void)n;    
    // 删除一个键值对
    assert(n == 1);
    // 获取连接所在的loop
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}
