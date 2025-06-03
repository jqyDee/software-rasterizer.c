# Project settings
PROJECT_NAME := rasterizer

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

RAYLIB_DIR := libs/raylib/src
RAYLIB_LIB := $(RAYLIB_DIR)/libraylib.a
RAYLIB_INCLUDE := $(RAYLIB_DIR)

# Compiler settings
CC := clang
CFLAGS := -I$(RAYLIB_INCLUDE) -I$(RAYLIB_DIR)/external -Wall -g -fsanitize=address
LDFLAGS := -L$(RAYLIB_DIR) -lraylib -g -lm -ldl -lpthread -framework Cocoa -framework IOKit -framework CoreVideo -fsanitize=address

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run

# Targets
all: $(RAYLIB_LIB) $(BIN_DIR)/$(PROJECT_NAME)

$(RAYLIB_LIB):
	$(MAKE) -C $(RAYLIB_DIR) PLATFORM=PLATFORM_DESKTOP

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/$(PROJECT_NAME): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

run: all
	./bin/rasterizer

clean:
	$(MAKE) -C $(RAYLIB_DIR) clean
	rm -rf $(BUILD_DIR) $(BIN_DIR)

