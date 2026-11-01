CC ?= gcc
CXX ?= g++
CFLAGS ?= -std=gnu11 -Wall -Wextra -O2
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2

BUILD_DIR := build
RESULTS_DIR := results
TEST_SOURCES := $(wildcard test_*.cpp)
TEST_BINS := $(patsubst test_%.cpp,$(BUILD_DIR)/test_%,$(TEST_SOURCES))
LOGIC_OBJECT := $(BUILD_DIR)/logic.o

.PHONY: all test clean

all: $(TEST_BINS)

test: $(TEST_BINS) make_gtkw.py
	mkdir -p $(RESULTS_DIR)
	@set -e; for binary in $(TEST_BINS); do \
		name=$$(basename "$$binary"); \
		trace="$(RESULTS_DIR)/$$name.vcd"; \
		"$$binary" "$$trace"; \
		python3 make_gtkw.py "$$trace" "$(RESULTS_DIR)/$$name.gtkw"; \
	done

$(BUILD_DIR):
	mkdir -p $@

$(LOGIC_OBJECT): ../src/logic.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Imock -I../src -c $< -o $@

$(BUILD_DIR)/test_%: test_%.cpp runtime.cpp test_runtime.hpp $(LOGIC_OBJECT) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -Imock -I../src runtime.cpp $< $(LOGIC_OBJECT) -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(RESULTS_DIR)/test_*.vcd $(RESULTS_DIR)/test_*.gtkw
