#include "Buffer.h"
#include "SocketsOps.h"

#include <sys/uio.h>

using namespace muduo;
using namespace muduo::net;

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

/**
 * 从fd上读取数据
 * 开启一块栈上空间暂时存放TCP读缓冲区的数据
 * 这样确保用户的缓冲区刚好容纳TCP读缓冲区大小的数据
*/
ssize_t Buffer::readFd(int fd, int* savedErrno) {
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536]; // 64K
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;

    // readv可以将数据读到非连续的多个缓冲区
    const ssize_t n = sockets::readv(fd, vec, iovcnt);
    if (n < 0) {
        *savedErrno = errno;
    }
    else if (static_cast<size_t>(n) <= writable) {
        writerIndex_ += n;
    }
    // extrabuf也写入了数据
    else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    // if (n == writable + sizeof extrabuf)
    // {
    //   goto line_19;
    // }
    return n;
}

ssize_t Buffer::writefd() {
    return 0;
}
