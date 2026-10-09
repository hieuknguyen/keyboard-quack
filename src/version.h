#ifndef QUACK_VERSION_H
#define QUACK_VERSION_H

/* Change this single value once when preparing a release. */
#define QUACK_VERSION 1.0.5

#define QUACK_STRINGIFY_INNER(value) #value
#define QUACK_STRINGIFY(value) QUACK_STRINGIFY_INNER(value)
#define QUACK_WIDEN_INNER(value) L##value
#define QUACK_WIDEN(value) QUACK_WIDEN_INNER(value)

#define QUACK_VERSION_STRING QUACK_STRINGIFY(QUACK_VERSION)
#define QUACK_VERSION_WIDE_STRING QUACK_WIDEN(QUACK_VERSION_STRING)

#endif
