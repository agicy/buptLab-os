#include "solution.hpp"
#include <unistd.h>

extern void solution_a(const task_t *const task) {
    uint32_t arg1 = task->number1;
    uint32_t arg2 = task->number2;

    uint64_t answer = static_cast<uint64_t>(arg1) * static_cast<uint64_t>(arg2);
    *task->answer = answer;
}

extern void solution_b(const task_t *const task) {
    uint32_t count = task->number1, value = task->number2;

    uint64_t answer = 0;
    for (uint32_t i = 0; i < count; ++i)
        answer += value;
    *task->answer = answer;
}

extern void solution_c(const task_t *const task) {
    uint32_t count1 = task->number1, count2 = task->number2;

    uint64_t answer = 0;
    for (uint32_t i = 0; i < count1; ++i)
        for (uint32_t j = 0; j < count2; ++j)
            ++answer;
    *task->answer = answer;
}
