CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -O2

build:
	mkdir -p build
	$(CXX) $(CXXFLAGS) -o build/main src/main.cpp

run: build
	sleep 5
	./build/main

clean:
	rm -rf build

.PHONY: build run clean
