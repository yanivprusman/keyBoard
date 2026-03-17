#pragma once
#include <string>
#include <linux/input.h>

class UInputEmitter {
public:
    ~UInputEmitter();

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
