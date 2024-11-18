CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -O2

build_release:
	mkdir -p build
	$(CXX) $(CXXFLAGS) -o build/release src/main.cpp

run: build_release
	./build/release

build_debug:
	mkdir -p build
	$(CXX) $(CXXFLAGS) -DFRAME_CHECK -o build/debug src/main.cpp

test: build_debug
	./build/debug

clean:
	rm -rf build

.PHONY: build_debug test build_release run clean
