#pragma once    // 防止头文件被重复包含

namespace muduo {
    /**
     * 继承自NonCopyable的类，其定义的对象不能拷贝构造以及拷贝赋值
    */
    class NonCopyable {
    public:
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    protected:  // 权限修饰符为protected，只允许子类访问
        NonCopyable() = default;
        ~NonCopyable() = default;
    };
}