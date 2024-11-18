#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

constexpr size_t message_size = 1 << 20;
constexpr size_t interval = 1e4;
constexpr size_t round = 1e5;
constexpr uint64_t generating_seed = 2022212720;

struct frame {
    std::byte data[message_size];
    auto operator==(const frame &another) const -> bool {
        return std::memcmp(data, another.data, message_size) == 0;
    }
    auto generate(uint64_t seed) -> void {
        std::fill(data, data + message_size, static_cast<std::byte>(seed));
    }
};

struct sembuf sem_op;

static inline auto init_semaphore(int semid, int val) -> void {
    if (semctl(semid, 0, SETVAL, val) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
}

static inline auto sem_p(int semid) -> void {
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    if (semop(semid, &sem_op, 1) == -1) {
        perror("semop P");
        exit(EXIT_FAILURE);
    }
}

static inline auto sem_v(int semid) -> void {
    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    if (semop(semid, &sem_op, 1) == -1) {
        perror("semop V");
        exit(EXIT_FAILURE);
    }
}

static inline auto writer(const int read_semid, const int write_semid, frame *const shared_frame) -> void {
    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < round; ++i) {
        sem_p(write_semid);
        shared_frame->generate(generating_seed + i);
        sem_v(read_semid);

        if ((i + 1) % interval == 0) {
            auto current_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = current_time - start_time;
            double speed = (i + 1) * message_size / (1024.0 * 1024.0) / elapsed.count();
            std::cout
                << std::format("Writer processed {} frames at speed: {} MiB/s", i + 1, speed)
                << std::endl;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_elapsed = end_time - start_time;
    double total_speed = round * message_size / (1024.0 * 1024.0) / total_elapsed.count();

    std::cout << std::format("Writer total speed: {} MiB/s", total_speed) << std::endl;
}

static inline auto reader(const int read_semid, const int write_semid, frame *const shared_frame) -> void {
    frame *local_frame = new frame;
    frame *expected_frame = new frame;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < round; ++i) {
        sem_p(read_semid);
        *local_frame = *shared_frame;
        sem_v(write_semid);

#ifdef FRAME_CHECK
        expected_frame->generate(generating_seed + i);
        if (!(*local_frame == *expected_frame))
            std::cerr << std::format("Data mismatch at round {}", i) << std::endl;
#endif

        if ((i + 1) % interval == 0) {
            auto current_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = current_time - start_time;
            double speed = (i + 1) * message_size / (1024.0 * 1024.0) / elapsed.count();
            std::cout
                << std::format("Reader processed {} frames at speed: {} MiB/s", i + 1, speed)
                << std::endl;
        }
    }
    delete local_frame;
    delete expected_frame;

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_elapsed = end_time - start_time;
    double total_speed = round * message_size / (1024.0 * 1024.0) / total_elapsed.count();

    std::cout << std::format("Reader total speed: {} MiB/s", total_speed) << std::endl;
}

int main() {
    key_t key = IPC_PRIVATE;

    int shmid = shmget(key, sizeof(frame) + sizeof(uint64_t), IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    int write_semid = semget(key, 1, IPC_CREAT | 0600);
    if (write_semid == -1) {
        perror("semget write");
        exit(EXIT_FAILURE);
    }

    int read_semid = semget(key + 1, 1, IPC_CREAT | 0600);
    if (read_semid == -1) {
        perror("semget read");
        exit(EXIT_FAILURE);
    }

    void *shared_memory = shmat(shmid, nullptr, 0);
    if (shared_memory == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    init_semaphore(write_semid, 1);
    init_semaphore(read_semid, 0);
    frame *shared_frame = static_cast<frame *>(shared_memory);

    auto pid = fork();
    if (pid) {
        writer(read_semid, write_semid, shared_frame);
        wait(nullptr);
    } else
        reader(read_semid, write_semid, shared_frame);

    shmdt(shared_memory);
    if (pid) {
        shmctl(shmid, IPC_RMID, nullptr);
        semctl(write_semid, 0, IPC_RMID);
        semctl(read_semid, 0, IPC_RMID);
    }

    return 0;
}
