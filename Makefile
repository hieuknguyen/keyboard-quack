CC ?= gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -Isrc
CXXFLAGS = -Wall -Wextra -O2 -std=c++17 -Isrc

PLATFORM_SRC = src/platform/platform.c
ENGINE_SRC = src/engine/telex.c
CONFIG_SRC = src/config/config.c
MAIN_SRC = src/main.c

ifeq ($(OS),Windows_NT)
    TARGET = quack.exe
    CAPTURE_SRC = src/capture/win32_capture.c src/capture/win32_uia.c src/windows/win_update.c
    INJECT_SRC = src/inject/win32_inject.c
    LDFLAGS = -mwindows -luser32 -lshell32 -ladvapi32 -luiautomationcore -lole32 -loleaut32 -luuid -lwinhttp -lbcrypt
    RM = del /Q /F
else
    TARGET = quack
    CAPTURE_SRC = src/capture/evdev_capture.c
    INJECT_SRC = src/inject/uinput_inject.c
    LDFLAGS = -lm -ldl
    RM = rm -f
endif

# Objects
PLATFORM_OBJ = $(PLATFORM_SRC:.c=.o)
ENGINE_OBJ = $(ENGINE_SRC:.c=.o)
CAPTURE_OBJ = $(CAPTURE_SRC:.c=.o)
INJECT_OBJ = $(INJECT_SRC:.c=.o)
CONFIG_OBJ = $(CONFIG_SRC:.c=.o)
MAIN_OBJ = $(MAIN_SRC:.c=.o)

OBJS = $(PLATFORM_OBJ) $(ENGINE_OBJ) $(CAPTURE_OBJ) $(INJECT_OBJ) $(CONFIG_OBJ) $(MAIN_OBJ)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
ifeq ($(OS),Windows_NT)
	-$(RM) $(subst /,\,$(OBJS)) $(TARGET)
else
	$(RM) $(OBJS) $(TARGET)
endif

install: $(TARGET)
ifeq ($(OS),Windows_NT)
	@echo "On Windows, copy quack.exe to your desired bin folder or PATH."
else
	install -m 755 $(TARGET) /usr/local/bin/
	mkdir -p /etc/keyboard-quack
	install -m 644 config/default.toml /etc/keyboard-quack/config.toml
endif

# Build quack-config (Qt GUI) if Qt is available
quack-config: src/ui/main_window.cpp src/config/config.c src/platform/platform.c
	$(CXX) $(CXXFLAGS) -o $@ $^ -lQt6Widgets -lQt5Widgets 2>/dev/null || \
	echo "Qt not available, skipping quack-config"
