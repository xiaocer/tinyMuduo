#pragma once

#include <unistd.h>
#include <vector>
#include <string>
#include <algorithm>
#include <assert.h>

#include "Copyable.h"

namespace muduo {
namespace net {
    /**
     * 用户缓冲区的设计
    */
    class Buffer : public Copyable{
    public:
        // 8字节预留空间
        static const size_t kCheapPrepend = 8;
        // 存放用户数据的空间
        static const size_t kInitialSize = 1024;
        
        explicit Buffer(size_t initialSize = kInitialSize)
            : buffer_(kCheapPrepend + initialSize),
            readerIndex_(kCheapPrepend),
            writerIndex_(kCheapPrepend) {
            assert(readableBytes() == 0);
            assert(writableBytes() == initialSize);
            assert(prependableBytes() == kCheapPrepend);
        }
        // 可读的缓冲区的长度
        size_t readableBytes() const { return writerIndex_ - readerIndex_; }
        // 可写的缓冲区的长度
        size_t writableBytes() const { return buffer_.size() - writerIndex_; }

        size_t prependableBytes() const { return readerIndex_; }

        // 返回缓冲区中可读数据的起始地址
        const char* peek() const { return begin() + readerIndex_; }

        // 从用户缓冲区读走len个字节的数据
        void retrieve(size_t len) {
            assert(len <= readableBytes());
            // 读取部分
            if (len < readableBytes()) {
                readerIndex_ += len;
            }
            // 全部读完
            else {
                retrieveAll();
            }
        }

        void retrieveAll() {
            readerIndex_ = kCheapPrepend;
            writerIndex_ = kCheapPrepend;
        }
        // 将onMessage函数向上传递的数据转成string类型的
        std::string retrieveAllAsString() {
            return retrieveAsString(readableBytes());
        }

        std::string retrieveAsString(size_t len) {
            assert(len <= readableBytes());
            std::string result(peek(), len);
            // 更新readerIndex
            retrieve(len);
            return result;
        }

        void ensureWritableBytes(size_t len) {
            // 扩容
            if (writableBytes() < len) {
                makeSpace(len);
            }
            assert(writableBytes() >= len);
        }

        void append(const char* /*restrict*/ data, size_t len)
        {
            ensureWritableBytes(len);
            std::copy(data, data+len, beginWrite());
            hasWritten(len);
        }

        void append(const void* /*restrict*/ data, size_t len) {
            append(static_cast<const char*>(data), len);
        }

        char* beginWrite() { return begin() + writerIndex_; }

        const char* beginWrite() const { return begin() + writerIndex_; }
        
        void hasWritten(size_t len) {
            assert(len <= writableBytes());
            writerIndex_ += len;
        }

        ssize_t readFd(int fd, int* savedErrno);
        ssize_t writefd();
    private:
        char* begin() { return &*buffer_.begin(); }

        const char* begin() const { return &*buffer_.begin(); }

        void makeSpace(size_t len) {
            // prependableBytes为读索引的位置
            if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
                buffer_.resize(writerIndex_+len);
            }
            // 将读缓冲区中的数据往前挪动
            else {
                // move readable data to the front, make space inside buffer
                assert(kCheapPrepend < readerIndex_);
                size_t readable = readableBytes();
                std::copy(begin()+readerIndex_,
                            begin()+writerIndex_,
                            begin()+kCheapPrepend);
                readerIndex_ = kCheapPrepend;
                writerIndex_ = readerIndex_ + readable;
                assert(readable == readableBytes());
            }
        
        }
        
    private:
        std::vector<char> buffer_;
        size_t readerIndex_;
        size_t writerIndex_;
    };
}
}