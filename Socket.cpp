#include "Socket.h"
#include "SocketsOps.h"
#include "InetAddress.h"
#include "logging.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <strings.h>

using namespace muduo;
using namespace muduo::net;

int Socket::accept(InetAddress* peeraddr) {
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    int connfd = sockets::accept(sockfd_, &addr);
    if (connfd >= 0) {
        peeraddr->setSockAddrInet(addr);
    }
    return connfd;
}

void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
                &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                            &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on) {
        LOG_ERROR("SO_REUSEPORT failed.");
    }
#else
    if (on) {
        LOG_ERROR("SO_REUSEPORT is not supported.");
    }
#endif
}

void Socket::bindAddress(const InetAddress& localaddr) {
    sockets::bindOrDie(this->sockfd_, localaddr.getSockAddr());
}

void Socket::listen() {
    sockets::listenOrDie(sockfd_);
}

void Socket::setKeepAlive(bool on) {
    int optVal = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optVal, static_cast<socklen_t>(sizeof(optVal)));
}

void Socket::shutdownWrite() {
    sockets::shutdownWrite(sockfd_);
}