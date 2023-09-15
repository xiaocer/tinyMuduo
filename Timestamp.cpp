#include "Timestamp.h"

#include <time.h>

using namespace muduo;

// 获取当前时间
Timestamp Timestamp::now() {
    return Timestamp(time(NULL));
}
// 时间的格式化表示
std::string Timestamp::toString() const {
    char buf[128] = {0};
    tm* t = localtime(&microSecondsSinceEpoch_);
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d", 
        t->tm_year + 1900,
        t->tm_mon + 1,
        t->tm_mday,
        t->tm_hour,
        t->tm_min,
        t->tm_sec);
    return buf;
}


// test
// int main() {
//     std::cout << Timestamp::now().toString() << std::endl;
// }