CC = clang # gcc for WSL desktop, clang for macbook setup
CXX = clang++ # C++ compiler for wrapper
CFLAGS = -Wall -Wextra -O2 $(shell sdl2-config --cflags)
CXXFLAGS = -Wall -Wextra -O2 -std=c++11 $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lm

# Python bindings configuration
PYTHON_INCLUDES = $(shell python3 -m pybind11 --includes)
PYTHON_EXT_SUFFIX = $(shell python3-config --extension-suffix)
# On macOS, use -undefined dynamic_lookup instead of linking Python directly
PYTHON_LDFLAGS = -undefined dynamic_lookup
PYBIND_CXXFLAGS = $(CXXFLAGS) $(PYTHON_INCLUDES) -fPIC
PYBIND_MODULE = sim_bindings$(PYTHON_EXT_SUFFIX)

SRC_DIR = src
BUILD_DIR = build
TARGET = engine
TARGET_SIM = sim
TARGET_WRAPPER = test_wrapper

# Core source files (shared by both targets)
CORE_SRC = $(filter-out $(SRC_DIR)/main.c $(SRC_DIR)/main_sim.c, $(wildcard $(SRC_DIR)/*.c))
CORE_OBJ = $(CORE_SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Main executables
MAIN_OBJ = $(BUILD_DIR)/main.o
MAIN_SIM_OBJ = $(BUILD_DIR)/main_sim.o

# Build both targets by default
all: $(TARGET) $(TARGET_SIM)

# C++ wrapper test target
$(TARGET_WRAPPER): $(CORE_OBJ) $(BUILD_DIR)/env_wrapper.o $(BUILD_DIR)/test_wrapper.o
	$(CXX) $^ -o $@ $(LDFLAGS)

$(TARGET): $(CORE_OBJ) $(MAIN_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_SIM): $(CORE_OBJ) $(MAIN_SIM_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# C++ compilation rule
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

run-sim: $(TARGET_SIM)
	./$(TARGET_SIM)

# Test C++ wrapper
test-wrapper: $(TARGET_WRAPPER)
	./$(TARGET_WRAPPER)

# Python bindings target
$(PYBIND_MODULE): $(CORE_OBJ) $(BUILD_DIR)/env_wrapper.o $(BUILD_DIR)/bindings.o
	$(CXX) -shared $^ -o $@ $(LDFLAGS) $(PYTHON_LDFLAGS)

# Special compilation rule for bindings.cpp with pybind11 flags
$(BUILD_DIR)/bindings.o: $(SRC_DIR)/bindings.cpp | $(BUILD_DIR)
	$(CXX) $(PYBIND_CXXFLAGS) -c $< -o $@

# Build Python bindings
bindings: $(PYBIND_MODULE)

# Test Python bindings
test-bindings: $(PYBIND_MODULE)
	python3 -c "import sim_bindings; print('✓ sim_bindings imported successfully')"

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TARGET_SIM) $(TARGET_WRAPPER) sim_bindings*.so

.PHONY: all run run-sim test-wrapper bindings test-bindings clean
