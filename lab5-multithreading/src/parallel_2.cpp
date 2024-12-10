#include "config.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <pthread.h>

struct apple_t {
    uint32_t a;
    uint32_t b;
    apple_t() : a(0), b(0) {}
    ~apple_t() {}
};

struct orange_t {
    uint32_t *a, *b;
    orange_t() {
        a = new uint32_t[ORANGE_MAX_VALUE];
        b = new uint32_t[ORANGE_MAX_VALUE];
        for (size_t i = 0; i < ORANGE_MAX_VALUE; ++i) {
            a[i] = orange_init_a;
            b[i] = orange_init_b;
        }
    }
    ~orange_t() {
        delete[] a;
        a = nullptr;
        delete[] b;
        b = nullptr;
    }
};

namespace apple_solution {

std::chrono::microseconds duration;

static inline auto part_a(void *arg) -> void * {
    apple_t *const apple = static_cast<apple_t *>(arg);

    for (size_t i = 0; i < APPLE_MAX_VALUE; ++i) {
        uint32_t val = i;
        uint32_t tmp0 = operations::mul(&val, &apple_a_mul);
        uint32_t tmp1 = operations::div(&tmp0, &apple_a_div);

        operations::add_and_assign(&apple->a, &tmp1);
    }

    return nullptr;
}

static inline auto part_b(void *arg) -> void * {
    apple_t *const apple = static_cast<apple_t *>(arg);

    for (size_t i = 0; i < APPLE_MAX_VALUE; ++i) {
        uint32_t val = i;
        uint32_t tmp0 = operations::mul(&val, &apple_b_mul);
        uint32_t tmp1 = operations::div(&tmp0, &apple_b_div);

        operations::add_and_assign(&apple->b, &tmp1);
    }

    return nullptr;
}

static inline auto solve(void *arg) -> void * {
    apple_t *const apple = static_cast<apple_t *>(arg);

    auto start = std::chrono::steady_clock::now();

    pthread_t thread_a, thread_b;

    pthread_create(&thread_a, nullptr, part_a, apple);
    pthread_join(thread_a, nullptr);

    pthread_create(&thread_b, nullptr, part_b, apple);
    pthread_join(thread_b, nullptr);

    auto end = std::chrono::steady_clock::now();

    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    return nullptr;
}

} // namespace apple_solution

namespace orange_solution {

std::chrono::microseconds duration;

static inline auto solve(void *arg) -> void * {
    std::pair<orange_t *, uint32_t> *const orange_problem = static_cast<std::pair<orange_t *, uint32_t> *>(arg);

    auto start = std::chrono::steady_clock::now();

    orange_problem->second = 0;
    for (size_t i = 0; i < ORANGE_MAX_VALUE; ++i) {
        uint32_t lef = operations::mul(&orange_problem->first->a[i], &orange_ka);
        uint32_t rig = operations::mul(&orange_problem->first->b[i], &orange_kb);
        uint32_t tmp = operations::add(&lef, &rig);

        operations::add_and_assign(&orange_problem->second, &tmp);
    }

    auto end = std::chrono::steady_clock::now();

    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    return nullptr;
}

} // namespace orange_solution

int main() {

    // init

    apple_t *apple = new apple_t;
    std::pair<orange_t *, uint32_t> orange = std::make_pair(new orange_t, 0);

    // start

    auto start = std::chrono::steady_clock::now();

    pthread_t apple_thread;
    pthread_t orange_thread;

    pthread_create(&apple_thread, nullptr, apple_solution::solve, apple);
    pthread_create(&orange_thread, nullptr, orange_solution::solve, &orange);

    pthread_join(apple_thread, nullptr);
    pthread_join(orange_thread, nullptr);

    // end

    auto end = std::chrono::steady_clock::now();

    std::chrono::microseconds total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // output

#ifdef TEST
    std::cout
        << std::format("apple=({}, {}), orange = {}",
                       apple->a, apple->b, orange.second)
        << std::endl;
#endif

    std::cout
        << std::format("apple_duration = {:>8} us, orange_duration = {:>8} us, total_duration = {:>8} us",
                       apple_solution::duration.count(), orange_solution::duration.count(), total_duration.count())
        << std::endl;

    // clean

    delete apple;
    delete orange.first;

    return 0;
}
