#pragma once

#include <netinet/in.h>
#include <string>

#include "Copyable.h"

namespace muduo {
    namespace net {
        class InetAddress : public muduo::Copyable{
        private:
            struct sockaddr_in addr_; 
        public:
            InetAddress() {}
            explicit InetAddress(const struct sockaddr_in& addr) : addr_(addr) {}

            explicit InetAddress(uint16_t port, std::string ip = "127.0.0.1");
            
            std::string toIp() const;
            
            std::string toIpPort() const;
            
            uint16_t port() const;

            sa_family_t family() const {
                return addr_.sin_family;
            }

            const struct sockaddr* getSockAddr() const { return static_cast<const struct sockaddr*>(((void*)&addr_)); }
            void setSockAddrInet(const struct sockaddr_in& addr)  {
                this->addr_ = addr;
            }
        };
    }
}