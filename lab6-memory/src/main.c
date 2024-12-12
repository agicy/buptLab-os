#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STACK_ARRAY_SIZE (1 << 15) // 32 KiB
#define HEAP_ARRAY_SIZE (1 << 25)  // 32 MiB
#define THREAD_COUNT 2

pthread_mutex_t mutex_output;

static inline void output(const char *fmt, ...) {
    pthread_mutex_lock(&mutex_output);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    putchar('\n');
    fflush(stdout);
    va_end(args);

    pthread_mutex_unlock(&mutex_output);
}

const char *string_literal_1 = "i will be put in the .rodata segment.";
const char *string_literal_2 = "i will be put in the .rodata segment.";
const char *string_literal_3 = "i will be the different one!";
const int integer_constant = 42;

int bss_integer;
double bss_real;

int integer = 2022212720;
double real = 2022211363;

static inline void easy_to_break() {}

static inline void *task(void *arg) {
    size_t id = *(size_t *)arg;

    int array_on_stack[STACK_ARRAY_SIZE];
    int *array_on_heap = malloc(HEAP_ARRAY_SIZE * sizeof(int));
    output("Thread %zu started, stack array address: %p, heap array address: %p", id, array_on_stack, array_on_heap);

    pthread_cleanup_push(free, array_on_heap);

    easy_to_break();

    pthread_cleanup_pop(1);
    pthread_exit(NULL);
}

int main() {
    pthread_mutex_init(&mutex_output, NULL);
    int array_on_stack[STACK_ARRAY_SIZE];
    int *array_on_heap = malloc(HEAP_ARRAY_SIZE * sizeof(int));
    pthread_t threads[THREAD_COUNT];
    size_t ids[THREAD_COUNT];

    output("Hello, World! My pid is %d", getpid());
    output("Stack array address: %p", array_on_stack);
    output("Heap array address: %p", array_on_heap);

    output("Threads creating...");
    for (size_t i = 0; i < THREAD_COUNT; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, task, &ids[i]);
    }

    easy_to_break();

    output("Threads joining...");
    for (size_t i = 0; i < THREAD_COUNT; i++) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }

    output("Bye, World!");

    free(array_on_heap), array_on_heap = NULL;
    pthread_mutex_destroy(&mutex_output);

    easy_to_break();

    return 0;
}
