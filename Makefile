# Project settings
PROJECT_NAME := rasterizer
PLUGIN_NAME := rasterizer.so

SRC_DIR := src
HOST_DIR := $(SRC_DIR)/host
PLUGIN_DIR := $(SRC_DIR)/rasterizer_module

BUILD_DIR := build
BIN_DIR := bin
PLUGIN_BUILD_DIR := $(BUILD_DIR)/plugin

RAYLIB_DIR := libs/raylib/src
RAYLIB_LIB := $(RAYLIB_DIR)/libraylib.a
RAYLIB_INCLUDE := $(RAYLIB_DIR)


# Compiler settings
CFLAGS := -Wall -Wextra -g -DDEBUG
CFLAGS_HOST := $(CFLAGS) -I$(HOST_DIR)
CFLAGS_PLUGIN := $(CFLAGS) -fPIC -I$(PLUGIN_DIR) -I$(RAYLIB_INCLUDE) -I$(RAYLIB_DIR)/external -I$(PLUGIN_DIR)

LDFLAGS := -g -ldl
LDFLAGS_PLUGIN := -shared -lraylib -g -lm -ldl -lpthread -framework Cocoa -framework IOKit -framework CoreVideo

# Source files
HOST_SRCS := $(wildcard $(HOST_DIR)/*.c)
HOST_OBJS := $(patsubst $(HOST_DIR)/%.c,$(BUILD_DIR)/host_%.o,$(HOST_SRCS))

PLUGIN_SRCS := $(wildcard $(PLUGIN_DIR)/*.c)
PLUGIN_OBJS := $(patsubst $(PLUGIN_DIR)/%.c,$(PLUGIN_BUILD_DIR)/%.o,$(PLUGIN_SRCS))

.PHONY: all clean run

# Targets
all: $(RAYLIB_LIB) $(BIN_DIR)/$(PROJECT_NAME) $(BIN_DIR)/$(PLUGIN_NAME)

# Build raylib
$(RAYLIB_LIB):
	$(MAKE) -C $(RAYLIB_DIR) PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=SHARED

# Build host object files
$(BUILD_DIR)/host_%.o: $(HOST_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS_HOST) -c $< -o $@

# Build plugin object files
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
	LD_LIBRARY_PATH=$(BIN_DIR) ./bin/$(PROJECT_NAME)

clean:
	# $(MAKE) -C $(RAYLIB_DIR) clean
	rm -rf $(BUILD_DIR) $(BIN_DIR)

