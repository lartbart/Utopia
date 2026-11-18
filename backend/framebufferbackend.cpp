#include "framebufferbackend.h"

void FramebufferBackend::init() {
    int fb = open("/dev/fb0", O_RDWR);
    if (fb < 0) {
        printf("Could not open framebuffer\n");
        return;
    }
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) < 0 ||
        ioctl(fb, FBIOGET_FSCREENINFO, &finfo) < 0) {
        printf("Error while gathering informations about the framebuffer\n");
        close(fb);
        return;
    }
    if (vinfo.bits_per_pixel != 32) {
        printf("32 bits per pixel is required.\n");
        close(fb);
        return;
    }
    size_t size = finfo.smem_len;
    fbmem = static_cast<uint8_t*>(mmap(nullptr, size,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED, fb, 0));
    if (fbmem == MAP_FAILED) {
        printf("Could not map framebuffer\n");
        close(fb);
        return;
    }
    mouse = open("/dev/input/event1", O_RDONLY);
    if (mouse < 0) {
        printf("Could not open mouse device\n");
        munmap(fbmem, size);
        close(fb);
        return;
    }
    keyboard = open("/dev/input/event0", O_RDONLY);
    if (keyboard < 0) {
        printf("Could not open keyboard device\n");
        close(mouse);
        munmap(fbmem, size);
        close(fb);
        return;
    }
}

void FramebufferBackend::drawCanvas() {
    if (!fbmem) {
        printf("Framebuffer is not mapped\n");
        return;
    }
    const size_t row_size = backend_canvas.width * sizeof(uint32_t);

    for (unsigned int y = 0; y < backend_canvas.height; y++) {
        size_t src_offset = y * backend_canvas.width;

        size_t dst_offset =
            (size_t)(y + vinfo.yoffset) * finfo.line_length +
            (size_t)vinfo.xoffset * sizeof(uint32_t);

        if (dst_offset + row_size <= finfo.smem_len) {
            memcpy(
                (uint8_t*)fbmem + dst_offset,
                backend_canvas.image + src_offset,
                row_size
            );
        }
    }
}

bool FramebufferBackend::getNextEvent() {
    drawCanvas();
    fd_set set;
    FD_ZERO(&set);

    int max_fd = -1;

    if (keyboard < 0) {
        printf("No keyboard device opened\n");
        return false;
    } 
    FD_SET(keyboard, &set);
    max_fd = std::max(max_fd, keyboard);

    if (mouse < 0) {
        printf("No mouse device opened\n");
        return false;
    } 
    FD_SET(mouse, &set);
    max_fd = std::max(max_fd, mouse);

    if (max_fd < 0) return false;

    while (true) {
        int result = select(
            max_fd + 1,
            &set,
            nullptr,
            nullptr,
            nullptr
        );

        if (result < 0) {
            if (errno == EINTR)
                continue;

            return false;
        }

        struct input_event ev;
        bool event_received = false;

        if (keyboard >= 0 && FD_ISSET(keyboard, &set)) {
            ssize_t n = read(keyboard, &ev, sizeof(ev));

            if (n == sizeof(ev) && ev.type == EV_KEY) {
                if (ev.code == KEY_A)
                    events_array[7] = ev.value;

                if (ev.code == KEY_B)
                    events_array[8] = ev.value;

                if (ev.code == KEY_ENTER)
                    events_array[9] = ev.value;

                event_received = true;
            }
        }

        if (mouse >= 0 && FD_ISSET(mouse, &set)) {
            ssize_t n = read(mouse, &ev, sizeof(ev));

            if (n == sizeof(ev)) {
                if (ev.type == EV_REL) {
                    if (ev.code == REL_X)
                        events_array[0] += ev.value;

                    if (ev.code == REL_Y)
                        events_array[1] += ev.value;

                    event_received = true;
                }

                if (ev.type == EV_KEY) {
                    if (ev.code == BTN_LEFT)
                        events_array[2] = ev.value;

                    if (ev.code == BTN_MIDDLE)
                        events_array[3] = ev.value;

                    if (ev.code == BTN_RIGHT)
                        events_array[4] = ev.value;

                    event_received = true;
                }
            }
        }

        if (event_received) {
            drawCanvas();
            return true;
        }

        FD_ZERO(&set);

        if (keyboard >= 0)
            FD_SET(keyboard, &set);

        if (mouse >= 0)
            FD_SET(mouse, &set);
    }
}

void FramebufferBackend::setCanvasImage(CanvasImage canvas) {
    backend_canvas = canvas;
}