#pragma once

#include <iostream>
#include <string>

namespace muduo {

    class Timestamp {
    private:
        int64_t microSecondsSinceEpoch_;
    public:
        Timestamp() : microSecondsSinceEpoch_(0) {}
        explicit Timestamp(int64_t microSecondsSinceEpoch) : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

        // 获取当前时间
        static Timestamp now();
        // 时间的格式化表示
        std::string toString() const;
    };
}