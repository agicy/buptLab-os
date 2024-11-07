#include "solution.hpp"
#include <cassert>
#include <csignal>
#include <format>
#include <iostream>
#include <random>

auto static sigterm_handler(int signal) -> void {
    std::cout
        << std::format(
               "[Student, pid={}, pgid={}]: Oh no! I will be terminated by signal {}! But I haven't done yet!",
               getpid(), getpgid(0), signal)
        << std::endl;
    exit(0);
}

int main(int argc, char *argv[]) {
    assert(argc == 3);

    std::string arg1 = argv[1];
    std::string arg2 = argv[2];

    uint32_t number1 = std::stoull(arg1);
    uint32_t number2 = std::stoull(arg2);
    uint64_t answer;

    task_t task(number1, number2, &answer);
    signal(SIGTERM, sigterm_handler);

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    size_t solution_id = std::random_device()() % 3;
    std::cout
        << std::format(
               "[Student, pid={}, pgid={}]: I am calculating! Method {} will be used.",
               getpid(), getpgid(0), solution_id)
        << std::endl;

    switch (solution_id) {
    case 0:
        solution_a(&task);
        break;
    case 1:
        solution_b(&task);
        break;
    case 2:
        solution_c(&task);
        break;

    default:
        abort();
    }

    clock_gettime(CLOCK_REALTIME, &end);
    auto seconds = end.tv_sec - start.tv_sec;
    auto nanoseconds = end.tv_nsec - start.tv_nsec;
    auto milliseconds = seconds * 1000.0 + nanoseconds / 1000000.0;

    std::cout
        << std::format(
               "[Student, pid={}, pgid={}]: I am done, the answer is {}! I used {} ms.",
               getpid(), getpgid(0), answer, milliseconds)
        << std::endl;
    return 0;
}
