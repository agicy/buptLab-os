#include <algorithm>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

constexpr size_t message_size = 1 << 20;
constexpr size_t output_interval = 1e4;
constexpr size_t send_round = 1e5;
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

static inline auto writer(int write_fd) -> void {
    frame *msg_frame = new frame;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < send_round; ++i) {
        msg_frame->generate(generating_seed + i);

        if (write(write_fd, msg_frame, sizeof(frame)) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        if ((i + 1) % output_interval == 0) {
            auto current_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = current_time - start_time;
            double speed = (i + 1) * message_size / (1024.0 * 1024.0) / elapsed.count();
            std::cout
                << std::format("Writer processed {} frames at speed: {} MiB/s", i + 1, speed)
                << std::endl;
        }
    }
    delete msg_frame;

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_elapsed = end_time - start_time;
    double total_speed = send_round * message_size / (1024.0 * 1024.0) / total_elapsed.count();

    std::cout << std::format("Writer total speed: {} MiB/s", total_speed) << std::endl;
}

static inline auto reader(int read_fd) -> void {
    frame *local_frame = new frame;
    frame *expected_frame = new frame;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < send_round; ++i) {
        auto ptr = reinterpret_cast<char *>(local_frame);
        auto end = ptr + sizeof(frame);
        while (ptr != end) {
            auto result = read(read_fd, ptr, end - ptr);
            if (result == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            ptr += result;
        }

#ifdef FRAME_CHECK
        expected_frame->generate(generating_seed + i);
        if (!(*local_frame == *expected_frame))
            std::cerr << std::format("Data mismatch at round {}", i) << std::endl;
#endif

        if ((i + 1) % output_interval == 0) {
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
    double total_speed = send_round * message_size / (1024.0 * 1024.0) / total_elapsed.count();

    std::cout << std::format("Reader total speed: {} MiB/s", total_speed) << std::endl;
}

int main() {
    const char *fifo_path = "/tmp/my_fifo";

    if (mkfifo(fifo_path, 0600) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    auto pid = fork();
    if (pid == 0) {
        int read_fd = open(fifo_path, O_RDONLY);
        if (read_fd == -1) {
            perror("open fifo for reading");
            exit(EXIT_FAILURE);
        }

        reader(read_fd);
        close(read_fd);
    } else {
        int write_fd = open(fifo_path, O_WRONLY);
        if (write_fd == -1) {
            perror("open fifo for writing");
            exit(EXIT_FAILURE);
        }

        writer(write_fd);
        close(write_fd);

        wait(nullptr);
        unlink(fifo_path);
    }

    return 0;
}
