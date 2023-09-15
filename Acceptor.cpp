#include "Acceptor.h"
#include "SocketsOps.h"
#include "logging.h"
#include "InetAddress.h"

#include <fcntl.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(EventLoop* mainLoop, const InetAddress& listenAddr, bool reusePort, NewConnectionCallback&& callback) 
    : mainLoop_(mainLoop),
    acceptSocket_(sockets::createNonblockingOrDie(listenAddr.family())),
    acceptChannel_(mainLoop, acceptSocket_.fd()),
    listening_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
        acceptSocket_.setReuseAddr(true);
        acceptSocket_.setReusePort(reusePort);
        acceptSocket_.bindAddress(listenAddr);
        acceptChannel_.setReadCallback(
            std::bind(&Acceptor::handleRead, this)
        );       
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

void Acceptor::listen() {
    mainLoop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    // 检测侦听套接字的读事件，即监听是否有客户的连接请求
    acceptChannel_.enableReading();
}

/**
 * 侦听套接字侦听到客户端的连接请求，需要执行的回调
 * 主要受理客户端的连接请求
*/
void Acceptor::handleRead() {   
    mainLoop_->assertInLoopThread();
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else {
            sockets::close(connfd);
        }
    } else {
        LOG_ERROR("handleRead error");
        // 当前进程的最大打开文件描述符限制
        if (errno == EMFILE) {
            // 将提前拥有的idleFd_关掉
            ::close(idleFd_);
            // 受理客户端的连接请求
            idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
            // 防止一直触发listenfd上的可读事件
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }        
    }
}