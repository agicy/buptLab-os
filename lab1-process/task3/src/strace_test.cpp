#include <cstdlib>
#include <format>
#include <iostream>
#include <unistd.h>

int main() {
    auto pid = getpid();
    std::cout
        << std::format("I just got my pid, it is {}, is that right?", pid)
        << std::endl;
    return 0;
}
