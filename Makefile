CFLAGS  := -std=gnu99 -Iinclude -Ibuild/protocols/include -Werror -lEGL \
  -lm -lwayland-egl -lwayland-client -lxkbcommon -lGL -DGL_GLEXT_PROTOTYPES
HEADERS := $(wildcard include/*.h) build/protocols/include/xdg-shell.h build/protocols/include/cursor-shape.h
SOURCES := $(wildcard src/*.c)
OBJECTS := $(addprefix build/, $(notdir $(SOURCES:.c=.o)))

TEST_HEADERS := $(HEADERS) $(wildcard test/include/*.h)
TEST_SOURCES := $(wildcard test/src/*.c)
TEST_OBJECTS := $(filter-out %/main.o, $(OBJECTS)) $(addprefix build/test/, $(notdir $(TEST_SOURCES:.c=.o)))

ifdef LOG_LEVEL
	CFLAGS += -DLOG_LEVEL=$(LOG_LEVEL)
endif
ifeq (${DEBUG_SYM}, 1)
	CFLAGS += -g
endif
ifeq (${GF_DEBUG_PLAYER_INPUT}, 1)
	CFLAGS += -DGF_DEBUG_PLAYER_INPUT
endif
ifeq (${GF_DEBUG_DRAW}, 1)
	CFLAGS += -DGF_DEBUG_DRAW
endif

TEST_CFLAGS := $(CFLAGS) -Itest/include

build/:
	mkdir -p build
	mkdir -p build/protocols
	mkdir -p build/protocols/src
	mkdir -p build/protocols/include

# Compile Object Files
build/%.o: src/%.c $(HEADERS) | build/
	gcc $< -c $(CFLAGS) -o $@

build/protocols/include/xdg-shell.h: /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml | build/
	wayland-scanner client-header $< $@

build/protocols/src/xdg-shell.c: /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml | build/
	wayland-scanner private-code $< $@

build/protocols/xdg-shell.o: build/protocols/src/xdg-shell.c
	gcc $< -c $(CFLAGS) -o $@

build/protocols/include/cursor-shape.h: /usr/share/wayland-protocols/staging/cursor-shape/cursor-shape-v1.xml | build/
	wayland-scanner client-header $< $@

build/protocols/src/cursor-shape.c: /usr/share/wayland-protocols/staging/cursor-shape/cursor-shape-v1.xml | build/
	wayland-scanner private-code $< $@

build/protocols/cursor-shape.o: build/protocols/src/cursor-shape.c
	gcc $< -c $(CFLAGS) -o $@

# For some reason cursor-shape depends on tablet-tool
build/protocols/include/tablet.h: /usr/share/wayland-protocols/stable/tablet/tablet-v2.xml | build/
	wayland-scanner client-header $< $@

build/protocols/src/tablet.c: /usr/share/wayland-protocols/stable/tablet/tablet-v2.xml | build/
	wayland-scanner private-code $< $@

build/protocols/tablet.o: build/protocols/src/tablet.c
	gcc $< -c $(CFLAGS) -o $@

build/factoriclone: $(OBJECTS) build/protocols/xdg-shell.o build/protocols/cursor-shape.o build/protocols/tablet.o
	gcc $^ $(CFLAGS) -o $@

# Test
build/test/test: $(TEST_OBJECTS) build/protocols/xdg-shell.o build/protocols/cursor-shape.o build/protocols/tablet.o
	gcc $^ $(TEST_CFLAGS) -o build/test/test

# Compile Object Files
build/test/%.o: test/src/%.c $(TEST_HEADERS) | build/ build/test/
	gcc $< -c $(TEST_CFLAGS) -o $@

build/test/:
	mkdir -p build/test

# noise example.
build/noise: build/noise.o | build/
	gcc examples/noise.c build/noise.o $(CFLAGS) -o build/noise

.PHONY: factoriclone run test run-test fmt
factoriclone: build/factoriclone
run: build/factoriclone
	exec $<
test: build/test/test
run-test: build/test/test
	exec $<
fmt:
	clang-format -i ./include/* ./src/* ./test/src/* ./test/include/*
