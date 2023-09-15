#include <netinet/in.h>

namespace muduo {
namespace net {
    namespace sockets {
        struct sockaddr_in getLocalAddr(int sockfd);

        ssize_t write(int sockfd, const void* buf, size_t count);

        ssize_t read(int sockfd, const void* buf, size_t count);

        int createNonblockingOrDie(sa_family_t family);
        
        void bindOrDie(int sockfd, const struct sockaddr* addr);
        
        void listenOrDie(int sockfd);

        int  accept(int sockfd, struct sockaddr_in* addr);

        void close(int sockfd);

        ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);
    
        void shutdownWrite(int sockfd);

        int getSocketError(int sockfd);
    }
}
}