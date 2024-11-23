CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -O2

build:
	mkdir -p build
	$(CXX) $(CXXFLAGS) -o build/main src/main.cpp

run: build
	./build/main

loop: build
	i=0; \
	while true; do \
		i=$$(($$i + 1)); \
		echo "Run $$i..."; \
		timeout -s 9 60 ./build/main >/dev/null 2>&1 || (echo "Error: runs longer than 60 seconds" && exit 1); \
	done

clean:
	rm -rf build

.PHONY: build run loop clean
