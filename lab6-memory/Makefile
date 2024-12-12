C := gcc
CXX := g++

build:
	mkdir -p build
	$(C) src/main.c -std=c17 -O0 \
		-Wall -Wextra -Werror \
		-fno-stack-protector -no-pie \
		-g \
		-o build/main
	$(CXX) src/util.cpp -std=c++20 -O2 \
		-Wall -Wextra -Werror \
		-o build/util

clean:
	rm -rf build

.PHONY: build clean
