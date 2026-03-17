#include "UInputEmitter.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/uinput.h>

UInputEmitter::~UInputEmitter() {
    destroy();
}

bool UInputEmitter::create(const std::string& name, int vendor, int product) {
    m_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (m_fd < 0) {
        fprintf(stderr, "[uinput] Cannot open /dev/uinput: %s\n", strerror(errno));
        return false;
    }

    // Enable event types
    ioctl(m_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(m_fd, UI_SET_EVBIT, EV_REP);
    ioctl(m_fd, UI_SET_EVBIT, EV_LED);

    // Enable all key codes
    for (int k = 0; k < KEY_MAX; k++)
        ioctl(m_fd, UI_SET_KEYBIT, k);

    // Enable LED codes (for NumLock LED sync)
    ioctl(m_fd, UI_SET_LEDBIT, LED_NUML);
    ioctl(m_fd, UI_SET_LEDBIT, LED_CAPSL);
    ioctl(m_fd, UI_SET_LEDBIT, LED_SCROLLL);

    struct uinput_setup setup = {};
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "%s", name.c_str());
    setup.id.bustype = BUS_VIRTUAL;
    setup.id.vendor = vendor;
    setup.id.product = product;
    setup.id.version = 1;

    if (ioctl(m_fd, UI_DEV_SETUP, &setup) < 0) {
        fprintf(stderr, "[uinput] UI_DEV_SETUP failed: %s\n", strerror(errno));
        close(m_fd);
        m_fd = -1;
        return false;
    }

    if (ioctl(m_fd, UI_DEV_CREATE) < 0) {
        fprintf(stderr, "[uinput] UI_DEV_CREATE failed: %s\n", strerror(errno));
        close(m_fd);
        m_fd = -1;
        return false;
    }

    // Wait for device to be registered
    usleep(100000);
    fprintf(stderr, "[uinput] Created virtual device: %s\n", name.c_str());
    return true;
}

void UInputEmitter::destroy() {
    if (m_fd >= 0) {
        ioctl(m_fd, UI_DEV_DESTROY);
        close(m_fd);
        m_fd = -1;
    }
}

void UInputEmitter::emitKey(int code, int value) {
    if (m_fd < 0) return;
    struct input_event ev = {};
    ev.type = EV_KEY;
    ev.code = code;
    ev.value = value;
    write(m_fd, &ev, sizeof(ev));
}

void UInputEmitter::emitSyn() {
    if (m_fd < 0) return;
    struct input_event ev = {};
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    write(m_fd, &ev, sizeof(ev));
}
