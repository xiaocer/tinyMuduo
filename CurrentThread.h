#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace muduo {
namespace CurrentThread {

    extern thread_local int t_cachedTid;

    // 获取当前线程所在的线程号
    inline int tid() {
        if (__builtin_expect(t_cachedTid == 0, 0)) {
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
        return t_cachedTid;
    }
}
}