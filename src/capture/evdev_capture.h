#ifndef EVDEV_CAPTURE_H
#define EVDEV_CAPTURE_H

#include <stdint.h>
#include <stdbool.h>
#include <linux/input.h>

#define MAX_KEYBOARDS 8

typedef struct {
    int  fd;
    char name[256];
    char path[256];
    bool grabbed;
} kbd_device_t;

typedef struct {
    kbd_device_t keyboards[MAX_KEYBOARDS];
    int          count;
    int          epoll_fd;
} capture_ctx_t;

/*
 * Find and open all keyboard devices.
 * Returns 0 on success, -1 on error.
 */
int capture_init(capture_ctx_t *ctx);

/*
 * Grab/ungrab all keyboard devices.
 * When grabbed, original events are suppressed.
 */
int capture_grab(capture_ctx_t *ctx);
int capture_ungrab(capture_ctx_t *ctx);

/*
 * Read next input event from any keyboard.
 * Blocks until an event is available.
 * Returns 0 on success, -1 on error/EOF.
 */
int capture_read(capture_ctx_t *ctx, struct input_event *ev, int *dev_idx);

/*
 * Cleanup and close all devices.
 */
void capture_cleanup(capture_ctx_t *ctx);

/*
 * Get keyboard file descriptors for polling.
 */
int capture_get_poll_fd(capture_ctx_t *ctx);

#endif /* EVDEV_CAPTURE_H */
