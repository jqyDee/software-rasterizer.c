# Project settings
PROJECT_NAME := rasterizer

SRC_DIR    := src
HOST_DIR   := $(SRC_DIR)/host
PLUGIN_DIR := $(SRC_DIR)/rasterizer_module

RAYLIB_DIR     := libs/raylib/src
RAYLIB_INCLUDE := $(RAYLIB_DIR)

# Detect OS
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    PLUGIN_EXT     := dylib
    RAYLIB_LIB     := $(RAYLIB_DIR)/libraylib.5.5.0.dylib
    LDFLAGS        := -g -ldl
    LIBOMP         := $(shell brew --prefix libomp 2>/dev/null)
    OPENMP_CFLAGS  := -Xpreprocessor -fopenmp -I$(LIBOMP)/include
    OPENMP_LFLAGS  := -L$(LIBOMP)/lib -lomp
    LDFLAGS_PLUGIN := -shared -L$(RAYLIB_DIR) -lraylib -g -lm -ldl -lpthread $(OPENMP_LFLAGS) \
                      -framework Cocoa -framework IOKit -framework CoreVideo
    LIB_PATH_VAR   := DYLD_LIBRARY_PATH
else ifeq ($(UNAME_S),Linux)
    PLUGIN_EXT     := so
    RAYLIB_LIB     := $(RAYLIB_DIR)/libraylib.so
    LDFLAGS        := -g -ldl
    OPENMP_CFLAGS  := -fopenmp
    OPENMP_LFLAGS  := -fopenmp
    LDFLAGS_PLUGIN := -shared -L$(RAYLIB_DIR) -lraylib -g -lm -ldl -lpthread $(OPENMP_LFLAGS)
    LIB_PATH_VAR   := LD_LIBRARY_PATH
else
    $(error Unsupported OS: $(UNAME_S))
endif

PLUGIN_NAME := $(PROJECT_NAME).$(PLUGIN_EXT)

# Build mode — separate bin and build dirs per mode
ifeq ($(DEBUG),1)
    BUILD_DIR := build/debug
    BIN_DIR   := bin/debug
    CFLAGS    := -Wall -Wextra -g -O0 -DDEBUG -DBIN_DIR=\"bin/debug\"
else
    BUILD_DIR := build/release
    BIN_DIR   := bin/release
    CFLAGS    := -Wall -Wextra -g -O3 -march=native -DBIN_DIR=\"bin/release\"
endif

PLUGIN_BUILD_DIR := $(BUILD_DIR)/plugin

# -MMD -MP: auto-generate header dependency files alongside .o files
CFLAGS_HOST   := $(CFLAGS) -MMD -MP -I$(HOST_DIR)
PLUGIN_INCDIRS := $(shell find $(PLUGIN_DIR) -type d)
CFLAGS_PLUGIN := $(CFLAGS) -MMD -MP -fPIC $(OPENMP_CFLAGS) \
                 $(addprefix -I,$(PLUGIN_INCDIRS)) -I$(RAYLIB_INCLUDE) -Ilibs/raygui

# Source and object files
HOST_SRCS  := $(wildcard $(HOST_DIR)/*.c)
HOST_OBJS  := $(patsubst $(HOST_DIR)/%.c,$(BUILD_DIR)/host_%.o,$(HOST_SRCS))
HOST_DEPS  := $(HOST_OBJS:.o=.d)

PLUGIN_SRCS := $(shell find $(PLUGIN_DIR) -name '*.c' -not -path '*/tests/*')
PLUGIN_OBJS := $(patsubst $(PLUGIN_DIR)/%.c,$(PLUGIN_BUILD_DIR)/%.o,$(PLUGIN_SRCS))
PLUGIN_DEPS := $(PLUGIN_OBJS:.o=.d)

# Static release (self-contained binary with embedded assets)
STATIC_BIN_DIR    := bin/static
STATIC_BUILD_DIR  := build/static
ifeq ($(UNAME_S),Darwin)
    RAYLIB_LIB_STATIC  := $(RAYLIB_DIR)/libraylib.a
    LDFLAGS_STATIC     := $(RAYLIB_LIB_STATIC) -lm -ldl -lpthread $(OPENMP_LFLAGS) \
                          -framework Cocoa -framework IOKit -framework CoreVideo \
                          -framework OpenGL
else ifeq ($(UNAME_S),Linux)
    RAYLIB_LIB_STATIC  := $(RAYLIB_DIR)/libraylib.a
    LDFLAGS_STATIC     := $(RAYLIB_LIB_STATIC) -lGL -lX11 -lXrandr -lXinerama \
                          -lXi -lXcursor -lm -ldl -lpthread $(OPENMP_LFLAGS)
endif

CFLAGS_STATIC := -Wall -Wextra -g -O3 -march=native -DSTATIC_RELEASE \
                 -DBIN_DIR=\"$(STATIC_BIN_DIR)\" -MMD -MP \
                 $(OPENMP_CFLAGS) $(addprefix -I,$(PLUGIN_INCDIRS)) -I$(HOST_DIR) \
                 -I$(RAYLIB_INCLUDE) -Ilibs/raygui

STATIC_HOST_OBJS   := $(patsubst $(HOST_DIR)/%.c,$(STATIC_BUILD_DIR)/host_%.o,$(HOST_SRCS))
STATIC_PLUGIN_OBJS := $(patsubst $(PLUGIN_DIR)/%.c,$(STATIC_BUILD_DIR)/plugin/%.o,$(PLUGIN_SRCS))
STATIC_EMBED_OBJ   := $(STATIC_BUILD_DIR)/embedded_assets.o
STATIC_ALL_OBJS    := $(STATIC_HOST_OBJS) $(STATIC_PLUGIN_OBJS) $(STATIC_EMBED_OBJ)

# Sentinel: force recompile if CFLAGS changed
FLAGS_SENTINEL := $(BUILD_DIR)/.flags
FLAGS_CURRENT  := $(CFLAGS)
FLAGS_STORED   := $(shell cat $(FLAGS_SENTINEL) 2>/dev/null)

ifneq ($(FLAGS_CURRENT),$(FLAGS_STORED))
    $(shell rm -f $(HOST_OBJS) $(PLUGIN_OBJS) && mkdir -p $(BUILD_DIR) $(PLUGIN_BUILD_DIR) && printf '%s' '$(FLAGS_CURRENT)' > $(FLAGS_SENTINEL))
endif

.DEFAULT_GOAL := all

-include $(HOST_DEPS)
-include $(PLUGIN_DEPS)

.PHONY: all debug clean run run-debug run-static static test test-coverage

all: $(RAYLIB_LIB) $(BIN_DIR)/$(PROJECT_NAME) $(BIN_DIR)/$(PLUGIN_NAME)

debug:
	$(MAKE) DEBUG=1 all

# Build raylib (with local macOS viewport fix applied)
$(RAYLIB_LIB):
	cd libs/raylib && git apply --check ../../raylib-macos-viewport.patch 2>/dev/null && git apply ../../raylib-macos-viewport.patch || true
	$(MAKE) -C $(RAYLIB_DIR) PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=SHARED

# Compile host objects
$(BUILD_DIR)/host_%.o: $(HOST_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS_HOST) -c $< -o $@

# Compile plugin objects
$(PLUGIN_BUILD_DIR)/%.o: $(PLUGIN_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_PLUGIN) -c $< -o $@

# Link host executable
$(BIN_DIR)/$(PROJECT_NAME): $(HOST_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Link plugin shared object
$(BIN_DIR)/$(PLUGIN_NAME): $(PLUGIN_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS_PLUGIN)

run-static: static
	./$(STATIC_BIN_DIR)/$(PROJECT_NAME)

run:
	$(MAKE) all
	$(LIB_PATH_VAR)=bin/release:$(RAYLIB_DIR) ./bin/release/$(PROJECT_NAME)

run-debug:
	$(MAKE) DEBUG=1 all
	$(LIB_PATH_VAR)=bin/debug:$(RAYLIB_DIR) ./bin/debug/$(PROJECT_NAME)

static: $(RAYLIB_LIB_STATIC) $(STATIC_ALL_OBJS)
	@mkdir -p $(STATIC_BIN_DIR)
	$(CC) $(STATIC_ALL_OBJS) -o $(STATIC_BIN_DIR)/$(PROJECT_NAME) $(LDFLAGS_STATIC)

$(RAYLIB_LIB_STATIC):
	cd libs/raylib && git apply --check ../../raylib-macos-viewport.patch 2>/dev/null && git apply ../../raylib-macos-viewport.patch || true
	$(MAKE) -C $(RAYLIB_DIR) PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=STATIC

$(STATIC_BUILD_DIR)/embedded_assets.c: $(wildcard assets/obj/*.obj) $(wildcard assets/textures/*)
	@mkdir -p $(STATIC_BUILD_DIR)
	python3 tools/embed_assets.py assets/obj assets/textures \
	    $(STATIC_BUILD_DIR)/embedded_assets.c \
	    $(PLUGIN_DIR)/embedded_assets.h

$(STATIC_BUILD_DIR)/host_%.o: $(HOST_DIR)/%.c
	@mkdir -p $(STATIC_BUILD_DIR)
	$(CC) $(CFLAGS_STATIC) -c $< -o $@

$(STATIC_BUILD_DIR)/plugin/%.o: $(PLUGIN_DIR)/%.c $(STATIC_BUILD_DIR)/embedded_assets.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_STATIC) -c $< -o $@

$(STATIC_BUILD_DIR)/embedded_assets.o: $(STATIC_BUILD_DIR)/embedded_assets.c
	@mkdir -p $(STATIC_BUILD_DIR)
	$(CC) $(CFLAGS_STATIC) -c $< -o $@

# ---- Tests ---------------------------------------------------------------
TEST_MODULE_DIR  := $(PLUGIN_DIR)/tests
TEST_BUILD_DIR   := build/test

# All plugin sources except those inside the test dir itself
TEST_PLUGIN_SRCS := $(filter-out $(shell find $(TEST_MODULE_DIR) -name '*.c'),$(PLUGIN_SRCS))
TEST_PLUGIN_OBJS := $(patsubst $(PLUGIN_DIR)/%.c,$(TEST_BUILD_DIR)/plugin/%.o,$(TEST_PLUGIN_SRCS))

TEST_SRCS := $(shell find $(TEST_MODULE_DIR) -name 'test_*.c')
TEST_BINS := $(patsubst $(TEST_MODULE_DIR)/%.c,$(TEST_BUILD_DIR)/%,$(TEST_SRCS))

CFLAGS_TEST  := -Wall -Wextra -g -O0 -DUNIT_TEST $(OPENMP_CFLAGS) \
                $(addprefix -I,$(PLUGIN_INCDIRS)) -I$(RAYLIB_INCLUDE) -Ilibs/raygui
LDFLAGS_TEST := -L$(RAYLIB_DIR) -lraylib -lm $(OPENMP_LFLAGS)
ifeq ($(UNAME_S),Darwin)
    LDFLAGS_TEST += -framework Cocoa -framework IOKit -framework CoreVideo
endif

$(TEST_BUILD_DIR)/unity.o: $(TEST_MODULE_DIR)/unity.c
	@mkdir -p $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS_TEST) -c $< -o $@

$(TEST_BUILD_DIR)/plugin/%.o: $(PLUGIN_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_TEST) -c $< -o $@

$(TEST_BUILD_DIR)/%: $(TEST_MODULE_DIR)/%.c $(TEST_PLUGIN_OBJS) $(TEST_BUILD_DIR)/unity.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_TEST) $^ -o $@ $(LDFLAGS_TEST)

test: $(RAYLIB_LIB) $(TEST_BINS)
	@export $(LIB_PATH_VAR)=$(RAYLIB_DIR) && for t in $(TEST_BINS); do echo "--- $$t ---"; $$t; done

# ---- Coverage (lcov + genhtml) -------------------------------------------
COV_BUILD_DIR   := build/coverage
CFLAGS_COV      := $(CFLAGS_TEST) --coverage

COV_PLUGIN_OBJS := $(patsubst $(PLUGIN_DIR)/%.c,$(COV_BUILD_DIR)/plugin/%.o,$(TEST_PLUGIN_SRCS))
COV_UNITY_OBJ   := $(COV_BUILD_DIR)/unity.o
COV_TEST_BINS   := $(patsubst $(TEST_MODULE_DIR)/%.c,$(COV_BUILD_DIR)/%,$(TEST_SRCS))

$(COV_BUILD_DIR)/unity.o: $(TEST_MODULE_DIR)/unity.c
	@mkdir -p $(COV_BUILD_DIR)
	$(CC) $(CFLAGS_COV) -c $< -o $@

$(COV_BUILD_DIR)/plugin/%.o: $(PLUGIN_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_COV) -c $< -o $@

$(COV_BUILD_DIR)/%: $(TEST_MODULE_DIR)/%.c $(COV_PLUGIN_OBJS) $(COV_UNITY_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_COV) $^ -o $@ $(LDFLAGS_TEST)

test-coverage: $(COV_TEST_BINS)
	@export $(LIB_PATH_VAR)=$(RAYLIB_DIR) && for t in $(COV_TEST_BINS); do $$t; done
	lcov --capture --directory $(COV_BUILD_DIR) \
	     --include "$(CURDIR)/$(PLUGIN_DIR)/*" \
	     --output-file $(COV_BUILD_DIR)/lcov.info
	genhtml $(COV_BUILD_DIR)/lcov.info \
	        --output-directory $(COV_BUILD_DIR)/html
	@echo "Report: $(COV_BUILD_DIR)/html/index.html"

# ---- Clean ---------------------------------------------------------------
clean:
	$(MAKE) -C $(RAYLIB_DIR) clean
	rm -f $(RAYLIB_DIR)/libraylib*.dylib $(RAYLIB_DIR)/libraylib*.dylib.dSYM
	rm -rf build bin
