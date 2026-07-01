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
    CFLAGS    := -Wall -Wextra -g -O2 -DBIN_DIR=\"bin/release\"
endif

PLUGIN_BUILD_DIR := $(BUILD_DIR)/plugin

# -MMD -MP: auto-generate header dependency files alongside .o files
CFLAGS_HOST   := $(CFLAGS) -MMD -MP -I$(HOST_DIR)
CFLAGS_PLUGIN := $(CFLAGS) -MMD -MP -fPIC $(OPENMP_CFLAGS) -I$(PLUGIN_DIR) -I$(RAYLIB_INCLUDE) -I$(RAYLIB_DIR)/external -Ilibs/raygui

# Source and object files
HOST_SRCS  := $(wildcard $(HOST_DIR)/*.c)
HOST_OBJS  := $(patsubst $(HOST_DIR)/%.c,$(BUILD_DIR)/host_%.o,$(HOST_SRCS))
HOST_DEPS  := $(HOST_OBJS:.o=.d)

PLUGIN_SRCS := $(wildcard $(PLUGIN_DIR)/*.c)
PLUGIN_OBJS := $(patsubst $(PLUGIN_DIR)/%.c,$(PLUGIN_BUILD_DIR)/%.o,$(PLUGIN_SRCS))
PLUGIN_DEPS := $(PLUGIN_OBJS:.o=.d)

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

.PHONY: all debug clean run run-debug

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
	@mkdir -p $(PLUGIN_BUILD_DIR)
	$(CC) $(CFLAGS_PLUGIN) -c $< -o $@

# Link host executable
$(BIN_DIR)/$(PROJECT_NAME): $(HOST_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Link plugin shared object
$(BIN_DIR)/$(PLUGIN_NAME): $(PLUGIN_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS_PLUGIN)

run: all
	$(LIB_PATH_VAR)=$(BIN_DIR):$(RAYLIB_DIR) ./$(BIN_DIR)/$(PROJECT_NAME)

run-debug:
	$(MAKE) DEBUG=1 all
	$(LIB_PATH_VAR)=$(BIN_DIR):$(RAYLIB_DIR) ./$(BIN_DIR)/$(PROJECT_NAME)

clean:
	$(MAKE) -C $(RAYLIB_DIR) clean
	rm -f $(RAYLIB_DIR)/libraylib*.dylib $(RAYLIB_DIR)/libraylib*.dylib.dSYM
	rm -rf build bin
