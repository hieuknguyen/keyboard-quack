#ifndef EVDEV_CAPTURE_H
#define EVDEV_CAPTURE_H

#include <stdint.h>
#include <stdbool.h>
#include <linux/input.h>

#define MAX_DEVICES 32

typedef struct {
    int  fd;
    char name[256];
    char path[256];
    bool grabbed;
    bool is_mouse;
} capture_device_t;

typedef struct {
    capture_device_t devices[MAX_DEVICES];
    int              count;
    int              epoll_fd;
} capture_ctx_t;

/*
 * Find and open all keyboard and pointer devices.
 * Returns 0 on success, -1 on error.
 */
int capture_init(capture_ctx_t *ctx);

/*
 * Grab/ungrab all keyboard devices (pointers are not grabbed).
 * When grabbed, original events are suppressed.
 */
int capture_grab(capture_ctx_t *ctx);
int capture_ungrab(capture_ctx_t *ctx);

/*
 * Read next input event from any device.
 * Blocks until an event is available.
 * Returns 0 on success, -1 on error/EOF.
 */
int capture_read(capture_ctx_t *ctx, struct input_event *ev, int *dev_idx);

/*
 * Cleanup and close all devices.
 */
void capture_cleanup(capture_ctx_t *ctx);

/*
 * Get file descriptor for polling.
 */
int capture_get_poll_fd(capture_ctx_t *ctx);

/*
 * Check if a device index is a pointer/mouse device.
 */
static inline bool capture_is_mouse(const capture_ctx_t *ctx, int dev_idx)
{
    if (dev_idx >= 0 && dev_idx < ctx->count) {
        return ctx->devices[dev_idx].is_mouse;
    }
    return false;
}

#endif /* EVDEV_CAPTURE_H */
