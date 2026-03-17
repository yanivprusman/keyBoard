#pragma once
#include <string>
#include <linux/input.h>

class UInputEmitter {
public:
    UInputEmitter() = default;
    ~UInputEmitter();

    // Move semantics (prevent double-close of uinput fd)
    UInputEmitter(UInputEmitter&& o) noexcept : m_fd(o.m_fd) { o.m_fd = -1; }
    UInputEmitter& operator=(UInputEmitter&& o) noexcept { destroy(); m_fd = o.m_fd; o.m_fd = -1; return *this; }
    UInputEmitter(const UInputEmitter&) = delete;
    UInputEmitter& operator=(const UInputEmitter&) = delete;

    // Create a virtual keyboard device
    bool create(const std::string& name, int vendor = 0, int product = 0);

    // Destroy the virtual device
    void destroy();

    // Emit a key event (value: 1=press, 0=release, 2=repeat)
    void emitKey(int code, int value);

    // Emit SYN_REPORT
    void emitSyn();

    int fd() const { return m_fd; }
    bool isOpen() const { return m_fd >= 0; }

private:
    int m_fd = -1;
};
