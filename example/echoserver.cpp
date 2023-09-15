#include <tinyMuduo/TcpServer.h>
#include <string>
#include <functional>

using namespace muduo;
using namespace muduo::net;

class EchoServer {
public:
    EchoServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const std::string& nameArg,
            TcpServer::Option option = TcpServer::Option::KReusePort)
        : mainLoop_(loop),
        server_(loop, listenAddr, nameArg, option) {
            server_.setThreadNum(5);    // 设置ioLoop个数
            server_.setMessageCallback(
                std::bind(&EchoServer::onMessage,
                    this, std::placeholders::_1,std::placeholders::_2,
                    std::placeholders::_3)
            );
        }
    void start() {
        server_.start();
    }
private:
    EventLoop* mainLoop_;    
    TcpServer server_;

    void onMessage(const TcpConnectionPtr& conn,
                Buffer* buffer,
                Timestamp receiveTime) {
        std::string message = buffer->retrieveAllAsString();
        conn->send((const void*)message.c_str(), message.size());
        conn->shutdown();
    }
};

int main() {
    EventLoop g_loop;
    InetAddress address(8080);
    EchoServer server(&g_loop, address, "ECHO-SERVER");
    
    server.start();
    
    g_loop.loop();

    return 0;
}