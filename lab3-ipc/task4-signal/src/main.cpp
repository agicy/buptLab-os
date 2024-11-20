#include <csignal>
#include <format>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

constexpr int signals[] = {16, 17};
std::vector<int> pids;

static inline auto handler(int signal) -> void {
    std::cout
        << std::format("[Process pid = {}] received signal = {}, will exit",
                       getpid(), signal)
        << std::endl;

    exit(0);
}

static inline auto global_handler(int signal) -> void {
    std::cout
        << std::format("[Process pid = {}] received signal = {}, will signal and wait",
                       getpid(), signal)
        << std::endl;

    for (size_t i = 0; i < pids.size(); ++i)
        kill(pids[i], signals[i]);

    for (size_t i = 0; i < pids.size(); ++i)
        wait(nullptr);

    exit(0);
}

static inline auto wait_for_signal(void) -> void {
    std::cout
        << std::format("[Process pid = {}] waiting for signal...",
                       getpid())
        << std::endl;

    while (true)
        sleep(60);
}

static inline auto empty_handler([[maybe_unused]] int _) -> void {
}

int main() {
    signal(SIGTSTP, global_handler);

    for (int sig : signals) {
        auto pid = fork();

        if (!pid) {
            signal(sig, handler);
            signal(SIGTSTP, empty_handler);
            wait_for_signal();
        } else
            pids.push_back(pid);
    }

    wait_for_signal();
    return 0;
}
