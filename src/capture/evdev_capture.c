#include "evdev_capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/epoll.h>
#include <linux/input.h>

#define INPUT_DIR "/dev/input"
#define EVENT_PREFIX "event"

static bool is_keyboard(int fd)
{
    unsigned long bits[KEY_MAX / (sizeof(unsigned long) * 8) + 1];
    memset(bits, 0, sizeof(bits));

    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bits)), bits) < 0)
        return false;

    /* Check for letter keys A-Z (keycodes 30-46, etc.) */
    bool has_letters = false;
    for (int key = KEY_Q; key <= KEY_P; key++) {
        int idx = key / (sizeof(unsigned long) * 8);
        int bit = key % (sizeof(unsigned long) * 8);
        if (bits[idx] & (1UL << bit)) {
            has_letters = true;
            break;
        }
    }

    if (!has_letters) return false;

    /* Should not be a mouse/touchpad (has relative axes) */
    unsigned long rel_bits = 0;
    ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel_bits)), &rel_bits);
    if (rel_bits != 0) return false;

    return true;
}

int capture_init(capture_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->count = 0;

    ctx->epoll_fd = epoll_create1(0);
    if (ctx->epoll_fd < 0) {
        perror("epoll_create1");
        return -1;
    }

    DIR *dir = opendir(INPUT_DIR);
    if (!dir) {
        perror("opendir");
        close(ctx->epoll_fd);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (ctx->count >= MAX_KEYBOARDS) break;

        if (strncmp(entry->d_name, EVENT_PREFIX, strlen(EVENT_PREFIX)) != 0)
            continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", INPUT_DIR, entry->d_name);

        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        if (!is_keyboard(fd)) {
            close(fd);
            continue;
        }

        /* Get device name */
        char name[256] = "Unknown";
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        if (strcmp(name, "keyboard-quack") == 0) {
            close(fd);
            continue;
        }

        kbd_device_t *kbd = &ctx->keyboards[ctx->count];
        kbd->fd = fd;
        kbd->grabbed = false;
        strncpy(kbd->path, path, sizeof(kbd->path) - 1);
        strncpy(kbd->name, name, sizeof(kbd->name) - 1);

        /* Add to epoll */
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, fd, &ev);

        fprintf(stderr, "[capture] Found keyboard: %s (%s)\n", name, path);
        ctx->count++;
    }

    closedir(dir);

    if (ctx->count == 0) {
        fprintf(stderr, "[capture] No keyboards found\n");
        close(ctx->epoll_fd);
        return -1;
    }

    fprintf(stderr, "[capture] Found %d keyboard(s)\n", ctx->count);
    return 0;
}

int capture_grab(capture_ctx_t *ctx)
{
    int grabbed = 0;
    for (int i = 0; i < ctx->count; i++) {
        if (ioctl(ctx->keyboards[i].fd, EVIOCGRAB, 1) == 0) {
            ctx->keyboards[i].grabbed = true;
            grabbed++;
            fprintf(stderr, "[capture] Grabbed: %s\n", ctx->keyboards[i].name);
        } else {
            fprintf(stderr, "[capture] Failed to grab %s: %s\n",
                    ctx->keyboards[i].name, strerror(errno));
        }
    }
    return (grabbed > 0) ? 0 : -1;
}

int capture_ungrab(capture_ctx_t *ctx)
{
    for (int i = 0; i < ctx->count; i++) {
        if (ctx->keyboards[i].grabbed) {
            ioctl(ctx->keyboards[i].fd, EVIOCGRAB, 0);
            ctx->keyboards[i].grabbed = false;
            fprintf(stderr, "[capture] Ungrabbed: %s\n", ctx->keyboards[i].name);
        }
    }
    return 0;
}

int capture_read(capture_ctx_t *ctx, struct input_event *ev, int *dev_idx)
{
    struct epoll_event ep_ev;
    int nfds = epoll_wait(ctx->epoll_fd, &ep_ev, 1, -1);
    if (nfds < 0) {
        if (errno == EINTR) return -1;
        perror("epoll_wait");
        return -1;
    }

    if (nfds == 0) return -1;

    int fd = ep_ev.data.fd;
    ssize_t n = read(fd, ev, sizeof(*ev));
    if (n != sizeof(*ev)) {
        if (errno == EINTR || errno == EAGAIN) return -1;
        perror("read");
        return -1;
    }

    /* Find which keyboard */
    if (dev_idx) {
        *dev_idx = -1;
        for (int i = 0; i < ctx->count; i++) {
            if (ctx->keyboards[i].fd == fd) {
                *dev_idx = i;
                break;
            }
        }
    }

    return 0;
}

void capture_cleanup(capture_ctx_t *ctx)
{
    capture_ungrab(ctx);
    for (int i = 0; i < ctx->count; i++) {
        if (ctx->keyboards[i].fd >= 0) {
            close(ctx->keyboards[i].fd);
            ctx->keyboards[i].fd = -1;
        }
    }
    if (ctx->epoll_fd >= 0) {
        close(ctx->epoll_fd);
        ctx->epoll_fd = -1;
    }
    ctx->count = 0;
}

int capture_get_poll_fd(capture_ctx_t *ctx)
{
    return ctx->epoll_fd;
}
