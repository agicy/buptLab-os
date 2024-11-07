#include "solution.hpp"
#include <algorithm>
#include <cassert>
#include <format>
#include <iostream>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <vector>

constexpr size_t student_number = 10;
constexpr std::string student_program = "./student";
constexpr size_t STACK_SIZE = 1024 * 1024;
constexpr int min_limit = 7e4;
constexpr int max_limit = 1e5;
constexpr uint exam_time = 2;

static inline auto get_random(int l, int r) -> int {
    return rand() % (r - l + 1) + l;
}

uint64_t answer;
task_t task(0, 0, &answer);

// Professor's function
int professor_func(void *arg) {
    assert(arg == nullptr);

    std::cout
        << std::format("[Professor, pid={}, pgid={}]: I will set the task.",
                       getpid(), getpgid(0))
        << std::endl;

    task.number1 = get_random(min_limit, max_limit);
    task.number2 = get_random(min_limit, max_limit);

    std::cout
        << std::format("[Professor, pid={}, pgid={}]: Task is set. And I will calculate the answer.",
                       getpid(), getpgid(0))
        << std::endl;

    solution_a(&task);

    std::cout
        << std::format("[Professor, pid={}, pgid={}]: {} * {} = {}. Ok. All things are done.",
                       getpid(), getpgid(0), task.number1, task.number2, *task.answer)
        << std::endl;
    return 0;
}

// TA functions
int ta_func(void *arg) {
    assert(arg != nullptr);

    auto method = (void (*)(const task_t *const))arg;

    std::cout
        << std::format(
               "[Tutor, pid={}, pgid={}]: I will check the answer.",
               getpid(), getpgid(0))
        << std::endl;

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    auto tmp_task = task;
    (*method)(&tmp_task);

    clock_gettime(CLOCK_REALTIME, &end);
    auto seconds = end.tv_sec - start.tv_sec;
    auto nanoseconds = end.tv_nsec - start.tv_nsec;
    auto milliseconds = seconds * 1000.0 + nanoseconds / 1000000.0;

    std::cout
        << std::format("[Tutor, pid={}, pgid={}]: After {} ms, I finished, two answers are {} and {}.",
                       getpid(), getpgid(0),
                       milliseconds, *tmp_task.answer, *task.answer)
        << std::endl;
    return 0;
}

int main() {
    // Professor set the task
    std::cout << std::format("\n***Professor is setting the task...***\n") << std::endl;

    char *prof_stack = (char *)malloc(STACK_SIZE);
    if (!prof_stack) {
        perror("malloc");
        abort();
    }

    pid_t prof_pid = clone(professor_func, prof_stack + STACK_SIZE, SIGCHLD | CLONE_VM, nullptr);
    if (prof_pid == -1) {
        perror("clone");
        abort();
    }
    waitpid(prof_pid, nullptr, 0);
    free(prof_stack);

    // TAs try different methods
    std::cout << std::format("\n***TAs are trying different methods...***\n") << std::endl;

    char *ta1_stack = (char *)malloc(STACK_SIZE);
    char *ta2_stack = (char *)malloc(STACK_SIZE);
    char *ta3_stack = (char *)malloc(STACK_SIZE);
    if (!ta1_stack || !ta2_stack || !ta3_stack) {
        perror("malloc");
        abort();
    }

    pid_t ta1_pid = clone(ta_func, ta1_stack + STACK_SIZE, SIGCHLD | CLONE_VM, (void *)(solution_a));
    pid_t ta2_pid = clone(ta_func, ta2_stack + STACK_SIZE, SIGCHLD | CLONE_VM, (void *)(solution_b));
    pid_t ta3_pid = clone(ta_func, ta3_stack + STACK_SIZE, SIGCHLD | CLONE_VM, (void *)(solution_c));
    if (ta1_pid == -1 || ta2_pid == -1 || ta3_pid == -1) {
        perror("clone");
        abort();
    }

    waitpid(ta1_pid, nullptr, 0);
    waitpid(ta2_pid, nullptr, 0);
    waitpid(ta3_pid, nullptr, 0);
    free(ta1_stack);
    free(ta2_stack);
    free(ta3_stack);

    // Students take the exam
    std::cout << std::format("\n***Students are taking the exam...***\n") << std::endl;

    std::vector<pid_t> students;
    for (size_t i = 0; i < student_number; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execl(
                student_program.data(),
                student_program.data(),
                std::to_string(task.number1).data(),
                std::to_string(task.number2).data(),
                nullptr);
            perror("execl");
            abort();
        } else if (pid > 0) {
            // Parent process
            students.push_back(pid);
        } else {
            perror("fork");
            abort();
        }
    }

    sleep(exam_time);

    for (auto pid : students)
        if (!kill(pid, 0))
            kill(pid, SIGTERM);
        else
            perror("Process does not exist or has terminated");

    while (!students.empty()) {
        int status;
        pid_t pid = waitpid(-1, &status, 0);
        if (pid > 0)
            students.erase(std::remove(students.begin(), students.end(), pid), students.end());
        else if (pid == -1) {
            if (errno == ECHILD)
                break;
            else {
                perror("waitpid failed");
                break;
            }
        }
    }

    std::cout << std::format("\n***Exam finished...***\n") << std::endl;

    return 0;
}
