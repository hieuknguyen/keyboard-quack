CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -Isrc
CXXFLAGS = -Wall -Wextra -O2 -std=c++17 -Isrc
LDFLAGS = -lm -ldl

# Sources
ENGINE_SRC = src/engine/telex.c
CAPTURE_SRC = src/capture/evdev_capture.c
INJECT_SRC = src/inject/uinput_inject.c
CONFIG_SRC = src/config/config.c
MAIN_SRC = src/main.c

# Objects
ENGINE_OBJ = $(ENGINE_SRC:.c=.o)
CAPTURE_OBJ = $(CAPTURE_SRC:.c=.o)
INJECT_OBJ = $(INJECT_SRC:.c=.o)
CONFIG_OBJ = $(CONFIG_SRC:.c=.o)
MAIN_OBJ = $(MAIN_SRC:.c=.o)

OBJS = $(ENGINE_OBJ) $(CAPTURE_OBJ) $(INJECT_OBJ) $(CONFIG_OBJ) $(MAIN_OBJ)

# Target
TARGET = quack

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/
	install -m 644 config/default.toml /etc/keyboard-quack/config.toml

# Build quack-config (Qt GUI) if Qt is available
quack-config: src/ui/main_window.cpp src/config/config.c
	$(CXX) $(CXXFLAGS) -o $@ $^ -lQt6Widgets -lQt5Widgets 2>/dev/null || \
	echo "Qt not available, skipping quack-config"
