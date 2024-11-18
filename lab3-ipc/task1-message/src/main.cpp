#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>

constexpr size_t message_size = 1 << 10;
constexpr size_t output_interval = 1e6;
constexpr size_t round = 1e7;
constexpr uint64_t generating_seed = 2022212720;
constexpr long message_type = 1;

struct frame {
    std::byte data[message_size];
    auto operator==(const frame &another) const -> bool {
        return std::memcmp(data, another.data, message_size) == 0;
    }
    auto generate(uint64_t seed) -> void {
        std::fill(data, data + message_size, static_cast<std::byte>(seed));
    }
};

struct message {
    long msg_type;
    frame data;
};

static inline auto writer(const int msgid) -> void {
    auto start_time = std::chrono::high_resolution_clock::now();

    message *msg = new message;
    for (size_t i = 0; i < round; ++i) {

        msg->msg_type = message_type;
        msg->data.generate(generating_seed + i);

        if (msgsnd(msgid, msg, sizeof(msg->data), 0) == -1) {
            perror("msgsnd");
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
    delete msg;

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_elapsed = end_time - start_time;
    double total_speed = round * message_size / (1024.0 * 1024.0) / total_elapsed.count();

    std::cout << std::format("Writer total speed: {} MiB/s", total_speed) << std::endl;
}

static inline auto reader(const int msgid) -> void {
    frame *expected_frame = new frame;
    message *msg = new message;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < round; ++i) {
        if (msgrcv(msgid, msg, sizeof(msg->data), message_type, 0) == -1) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }

#ifdef FRAME_CHECK
        expected_frame->generate(generating_seed + i);
        if (!(msg->data == *expected_frame))
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
    delete expected_frame;
    delete msg;

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_elapsed = end_time - start_time;
    double total_speed = round * message_size / (1024.0 * 1024.0) / total_elapsed.count();

    std::cout << std::format("Reader total speed: {} MiB/s", total_speed) << std::endl;
}

int main() {
    key_t key = IPC_PRIVATE;

    int msgid = msgget(key, IPC_CREAT | 0600);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    auto pid = fork();
    if (pid) {
        writer(msgid);
        wait(nullptr);
    } else
        reader(msgid);

    if (pid)
        if (msgctl(msgid, IPC_RMID, nullptr) == -1) {
            perror("msgctl");
            exit(EXIT_FAILURE);
        }

    return 0;
}
