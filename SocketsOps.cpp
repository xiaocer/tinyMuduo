#include "SocketsOps.h"
#include "logging.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/uio.h>

using namespace muduo;
using namespace muduo::net;

struct sockaddr_in sockets::getLocalAddr(int sockfd) {
    struct sockaddr_in localAddr;
    socklen_t size = static_cast<socklen_t>(sizeof(localAddr));
    if (::getsockname(sockfd, (sockaddr*)&localAddr, &size) < 0) {
        LOG_ERROR("getLocalAddr error");
    }
    return localAddr;
}

ssize_t sockets::write(int sockfd, const void* buf, size_t count) {
    return ::write(sockfd, const_cast<void*>(buf), count);   
}

ssize_t sockets::read(int sockfd, const void* buf, size_t count) {
    return ::read(sockfd, const_cast<void*>(buf), count);
}

int sockets::createNonblockingOrDie(sa_family_t family) {
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) {
        LOG_FATAL("create socket error");
    }
    return sockfd;
}

void sockets::bindOrDie(int sockfd, const struct sockaddr* addr) {
  int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr)));
  if (ret < 0) {
    LOG_FATAL("sockets::bindOrDie");
  }
}

void sockets::listenOrDie(int sockfd) {
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0) {
        LOG_FATAL("sockets::listenOrDie");
    }
}

int  sockets::accept(int sockfd, struct sockaddr_in* addr) {
    socklen_t addrLen = static_cast<socklen_t>(sizeof(*addr));
    int connfd = ::accept4(sockfd, static_cast<sockaddr*>(((void*)addr)), &addrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0) {
        LOG_FATAL("accept error");
    }
    return connfd;
}

void sockets::close(int sockfd) {
    if (::close(sockfd) < 0) {
        LOG_ERROR("close fd error");
    }
}

ssize_t sockets::readv(int sockfd, const struct iovec *iov, int iovcnt) {
   return ::readv(sockfd, iov, iovcnt);  
}

void sockets::shutdownWrite(int sockfd) {
    if (::shutdown(sockfd, SHUT_WR) == -1) {
        LOG_ERROR("shutdown error");
    }
}

// 获取套接字上发生错误的错误号
int sockets::getSocketError(int sockfd) {
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        return errno;
    } else {
        return optval;
    }
}