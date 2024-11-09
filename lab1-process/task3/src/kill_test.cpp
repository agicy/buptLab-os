#include <csignal>
#include <format>
#include <iostream>
#include <unistd.h>

auto sigterm_handler(int signal) -> void {
    std::cout
        << std::format("I received SIGTERM({})?", signal)
        << std::endl;
    std::cout
        << std::format("I won't be terminated!")
        << std::endl;
    return;
}

int main() {
    auto pid = getpid();
    std::cout
        << std::format("My pid is {}.", pid)
        << std::endl;

    signal(SIGTERM, sigterm_handler);
    while (true)
        sleep(1);

    return 0;
}
