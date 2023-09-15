#pragma once

namespace muduo {
    class Copyable {
    protected:
        Copyable() = default;
        ~Copyable() = default;
    };
}