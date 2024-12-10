#pragma once

#ifndef CONFIG_H
#define CONFIG_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <pthread.h>
#include <sched.h>
#include <vector>

constexpr size_t APPLE_MAX_VALUE = 1e5;
constexpr size_t ORANGE_MAX_VALUE = 0.35e5;
constexpr uint32_t apple_a_mul = 2022;
constexpr uint32_t apple_a_div = 2024;
constexpr uint32_t apple_b_mul = 2720;
constexpr uint32_t apple_b_div = 1363;
constexpr uint32_t orange_init_a = 2022212720;
constexpr uint32_t orange_init_b = 2022211363;
constexpr uint32_t orange_ka = 2022212720;
constexpr uint32_t orange_kb = 2022211363;

namespace operations {

constexpr size_t bit_width = 32;

const auto add = [](const uint32_t *a, const uint32_t *b) -> uint32_t {
    uint32_t carry = 0;
    uint32_t result = 0;
    for (size_t i = 0; i < bit_width; ++i) {
        uint32_t bit_a = (*a >> i) & 1;
        uint32_t bit_b = (*b >> i) & 1;
        uint32_t sum = bit_a ^ bit_b ^ carry;
        carry = (bit_a & bit_b) | (bit_a & carry) | (bit_b & carry);
        result |= sum << i;
    }
    assert(result == *a + *b);
    return result;
};

const auto sub = [](const uint32_t *a, const uint32_t *b) -> uint32_t {
    uint32_t tmp0 = ~*b;
    uint32_t tmp1 = 1;
    uint32_t tmp2 = add(&tmp0, &tmp1);
    uint32_t result = operations::add(a, &tmp2);
    assert(result == *a - *b);
    return result;
};

const auto add_and_assign = [](uint32_t *a, const uint32_t *b) -> uint32_t * {
    uint32_t tmp = add(a, b);
    *a = tmp;
    return a;
};

const auto sub_and_assign = [](uint32_t *a, const uint32_t *b) -> uint32_t * {
    uint32_t tmp = sub(a, b);
    *a = tmp;
    return a;
};

const auto mul = [](const uint32_t *a, const uint32_t *b) -> uint32_t {
    uint32_t result = 0;
    uint32_t tmp = *a;
    for (size_t i = 0; i < bit_width; ++i) {
        if ((*b >> i) & 1)
            add_and_assign(&result, &tmp);
        add_and_assign(&tmp, &tmp);
    }
    assert(result == *a * *b);
    return result;
};

const auto div = [](const uint32_t *a, const uint32_t *b) -> uint32_t {
    uint32_t dividend = *a;
    size_t shift = bit_width;
    for (size_t i = bit_width - 1; i < bit_width; --i)
        if ((*b >> i) & 1) {
            shift = i;
            break;
        }
    uint32_t quotient = 0;
    for (size_t i = bit_width - 1; i < bit_width && i >= shift; --i) {
        uint32_t tmp = *b << (i - shift);
        if (dividend >= tmp) {
            sub_and_assign(&dividend, &tmp);
            assert(dividend < tmp);
            quotient |= 1 << (i - shift);
        }
    }
    assert(quotient == *a / *b);
    return quotient;
};

} // namespace operations

static inline auto set_cpu(std::vector<int> cpu) -> void {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (auto &c : cpu)
        CPU_SET(c, &cpuset);
    int rc = sched_setaffinity(0, sizeof(cpuset), &cpuset);
    assert(rc == 0);
}

#endif
