CC = clang # gcc for WSL desktop, clang for macbook setup
CFLAGS = -Wall -Wextra -O2 $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lm

SRC_DIR = src
BUILD_DIR = build
TARGET = engine
TARGET_SIM = sim

# Core source files (shared by both targets)
CORE_SRC = $(filter-out $(SRC_DIR)/main.c $(SRC_DIR)/main_sim.c, $(wildcard $(SRC_DIR)/*.c))
CORE_OBJ = $(CORE_SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Main executables
MAIN_OBJ = $(BUILD_DIR)/main.o
MAIN_SIM_OBJ = $(BUILD_DIR)/main_sim.o

# Build both targets by default
all: $(TARGET) $(TARGET_SIM)

$(TARGET): $(CORE_OBJ) $(MAIN_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_SIM): $(CORE_OBJ) $(MAIN_SIM_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

run-sim: $(TARGET_SIM)
	./$(TARGET_SIM)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TARGET_SIM)

.PHONY: all run run-sim clean
