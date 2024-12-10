#!/bin/sh

CXX=g++

build() {
    echo "Compiling..."
    mkdir -p build

    eval "$CXX $CXXFLAGS -o build/serial src/serial.cpp"
    eval "$CXX $CXXFLAGS -o build/parallel_2 src/parallel_2.cpp"
    eval "$CXX $CXXFLAGS -o build/parallel_3_mutex src/parallel_3_mutex.cpp"
    eval "$CXX $CXXFLAGS -o build/parallel_3 src/parallel_3.cpp"

    for size in 16 24 32 48 64 80 96 112 128 192 256 384; do
        eval "$CXX $CXXFLAGS "-DPADDING_SIZE=$size" -o build/parallel_3_cache_${size} src/parallel_3_cache_padding.cpp"
    done

    for size in 16 24 32 48 64 80 96 112 128 192 256 384; do
        eval "$CXX $CXXFLAGS "-DPADDING_SIZE=$size" -o build/parallel_3_mutex_cache_${size} src/parallel_3_mutex_cache_padding.cpp"
    done

    eval "$CXX $CXXFLAGS -o build/parallel_2_affinity src/parallel_2_affinity.cpp"
    eval "$CXX $CXXFLAGS -o build/parallel_3_affinity src/parallel_3_affinity.cpp"

    echo "Compiled."
}

build_release() {
    CXXFLAGS="-std=c++20 -Wall -Wextra -Werror -O0"
    build
}

build_debug() {
    CXXFLAGS="-std=c++20 -Wall -Wextra -Werror -O0 -DTEST -fsanitize=address -fsanitize=undefined"
    build
}

clean() {
    rm -rf build
}

case "$1" in
build_release)
    build_release
    ;;
build_debug)
    build_debug
    ;;
clean)
    clean
    ;;
*)
    echo "Usage: $0 {build_release|build_debug|clean}"
    exit 1
    ;;
esac
