# Privilege Tetris - build file (GNU Make, g++ with C++17)
#   make          build ./privilege_tetris
#   make run      build and start the interactive menu
#   make test     build and run the automated tests
#   make asan     rebuild everything with AddressSanitizer + UBSan and run the tests
#   make clean    remove build output

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
SRC_DIR  := src
BUILD    := build

LIB_SRCS := $(filter-out $(SRC_DIR)/main.cpp,$(wildcard $(SRC_DIR)/*.cpp))
LIB_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD)/%.o,$(LIB_SRCS))

all: privilege_tetris

privilege_tetris: $(LIB_OBJS) $(BUILD)/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

tests/run_tests: $(LIB_OBJS) tests/test_main.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -o $@ tests/test_main.cpp $(LIB_OBJS)

$(BUILD)/%.o: $(SRC_DIR)/%.cpp $(wildcard $(SRC_DIR)/*.h) | $(BUILD)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

run: privilege_tetris
	./privilege_tetris

test: tests/run_tests
	./tests/run_tests

asan:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="-std=c++17 -Wall -Wextra -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer" test
	$(MAKE) CXXFLAGS="-std=c++17 -Wall -Wextra -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer" privilege_tetris

clean:
	rm -rf $(BUILD) privilege_tetris tests/run_tests

.PHONY: all run test asan clean
