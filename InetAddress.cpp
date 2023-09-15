#include "InetAddress.h"

#include <arpa/inet.h>
#include <string.h>
#include <iostream>

using namespace muduo;
using namespace muduo::net;

// 默认参数时不能在声明和定义同时出现
InetAddress::InetAddress(uint16_t port, std::string ip) {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    inet_aton(ip.c_str(), &addr_.sin_addr);
}
            
// 将网络字节序的IP转化为主机字节序的
std::string InetAddress::toIp() const {
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr.s_addr, buf, sizeof(buf));
    return buf;
}

// 将网络字节序的IP和端口转化为主机字节序的 ip:port
std::string InetAddress::toIpPort() const {
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr.s_addr, buf, sizeof(buf));
    int end = strlen(buf);
    sprintf(buf + end, ":%u", port());
    return buf;
}

// 将网络字节序的端口转化为主机字节序的
uint16_t InetAddress::port() const {
    return ntohs(addr_.sin_port);
}


// test
// int main() {
//     InetAddress addr(6666, "192.168.39.5");
//     std::cout << addr.toIpPort() << std::endl;
//     return 0;
// }