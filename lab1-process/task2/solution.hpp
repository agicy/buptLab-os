#pragma once

#ifndef SOLUTION_H
#define SOLUTION_H

#include <cstdint>

struct task_t {
    uint32_t number1;
    uint32_t number2;
    uint64_t *answer;
    task_t(uint32_t number1,
           uint32_t number2,
           uint64_t *answer)
        : number1(number1), number2(number2), answer(answer) {}
};

extern void solution_a(const task_t *const task);
extern void solution_b(const task_t *const task);
extern void solution_c(const task_t *const task);

#endif
