#include <array>
#include <atomic>
#include <chrono>
#include <format>
#include <iostream>
#include <pthread.h>
#include <random>
#include <thread>

static constexpr size_t person_number = 8;
static constexpr auto study_time = std::chrono::milliseconds(700);
static constexpr auto wait_time = std::chrono::milliseconds(166);
static constexpr double request_probility = 0.2720;
static constexpr double startup_probility = 0.1363 * 4.50;
static constexpr size_t wait_limit = 10;
static constexpr uint32_t seed = 2022212720u + 2022211363u;

class DormRoom {
  private:
    mutable pthread_mutex_t mutex_state;
    mutable pthread_mutex_t mutex_cout;
    std::atomic<size_t> active_count;
    std::chrono::steady_clock::time_point start_time;

  public:
    /**
     * Constructs a new DormRoom object, initializing its state and mutexes.
     *
     * @note This constructor is used to set up the initial state of the DormRoom.
     *
     * @return None
     *
     * @throws None
     */
    DormRoom() : active_count(0), start_time(std::chrono::steady_clock::now()) {
        pthread_mutex_init(&mutex_state, nullptr);
        pthread_mutex_init(&mutex_cout, nullptr);
    }

    /**
     * Destroys a DormRoom object, releasing its mutexes.
     *
     * @note This destructor is used to clean up the DormRoom object.
     *
     * @return None
     *
     * @throws None
     */
    ~DormRoom() {
        pthread_mutex_destroy(&mutex_state);
        pthread_mutex_destroy(&mutex_cout);
    }

    /**
     * Prints a message to the console with a timestamp.
     *
     * @param message the message to be printed
     *
     * @return None
     *
     * @throws None
     */
    auto print_message(const std::string_view message) const -> void {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
        double milliseconds = duration.count();

        pthread_mutex_lock(&mutex_cout);
        std::cout << std::format("[{:>8.3f} ms] {}\n", milliseconds, message);
        pthread_mutex_unlock(&mutex_cout);
    }

    /**
     * Simulates a student studying in a dorm room.
     *
     * @param id the ID of the student
     * @param last_study the time point when the student last studied
     *
     * @return None
     *
     * @throws None
     */
    auto study(size_t id, const std::chrono::steady_clock::time_point last_study) -> void {
        auto now = std::chrono::steady_clock::now();
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_study).count();
        print_message(std::format("Student {} starts studying after waiting for {:>4} ms", id, milliseconds));

        pthread_mutex_lock(&mutex_state);
        active_count++;
        pthread_mutex_unlock(&mutex_state);

        std::this_thread::sleep_for(study_time);

        pthread_mutex_lock(&mutex_state);
        active_count--;
        pthread_mutex_unlock(&mutex_state);

        print_message(std::format("Student {} stops studying", id));
    }

    /**
     * Checks if a student decides to study based on the current state of the dorm room and random probability.
     *
     * @param id the ID of the student
     * @param gen a random number generator
     * @param dis a uniform real distribution for generating random numbers
     *
     * @return true if the student decides to study, false otherwise
     *
     * @throws None
     */
    auto check_and_decide(const size_t id, std::mt19937 &gen, std::uniform_real_distribution<> dis) -> bool {
        if (dis(gen) < request_probility) {
            pthread_mutex_lock(&mutex_state);
            bool others_studying = active_count > 0;
            pthread_mutex_unlock(&mutex_state);

            if (others_studying) {
                print_message(std::format("Student {} is checking others", id));
                if (dis(gen) < startup_probility) {
                    print_message(std::format("Student {} decides to study", id));
                    return true;
                }
                print_message(std::format("Student {} decides NOT to study", id));
            }
        }

        return false;
    }

    /**
     * Checks if all students in the dorm room are sleeping.
     *
     * @return true if all students are sleeping, false otherwise
     *
     * @throws None
     */
    auto is_everyone_sleeping() const -> bool {
        pthread_mutex_lock(&mutex_state);
        bool result = (active_count == 0);
        pthread_mutex_unlock(&mutex_state);
        return result;
    }
};

struct StudentArgs {
    DormRoom *dorm;
    size_t id;
    bool is_studier;
};

/**
 * A function that handles a student's complaint.
 *
 * When a student complains, they will be complaining about the current state of the dorm room, which
 * includes whether others are studying. This function is used to simulate the student's complaint.
 *
 * @param arg a pointer to a StudentArgs object containing the student's ID, dorm room, and whether they are a studier
 *
 * @return void
 *
 * @throws None
 */
static inline auto student_complaint(void *arg) -> void {
    auto *const args = static_cast<StudentArgs *>(arg);
    auto *const dorm = args->dorm;
    const size_t id = args->id;

    // The student is complaining about the current state of the dorm room
    dorm->print_message(std::format("Student {} is complaining, and he decides to die", id));
}

/**
 * A routine that simulates a student's behavior in a dorm room.
 *
 * This function implements a student's behavior in a dorm room, which includes deciding whether to study,
 * checking if others are studying, and sleeping. It also handles the student's complaint using a cleanup handler.
 *
 * @param arg a pointer to a StudentArgs object containing the student's ID, dorm room, and whether they are a studier
 *
 * @return a void pointer indicating the end of the student's routine
 *
 * @throws None
 */
static inline auto student_routine(void *arg) -> void * {
    auto *const args = static_cast<StudentArgs *>(arg);
    auto *const dorm = args->dorm;
    const size_t id = args->id;
    bool is_studier = args->is_studier;
    auto last_study = std::chrono::steady_clock::now();

    // Set up a cleanup handler to handle the student's complaint
    pthread_cleanup_push(student_complaint, args);

    std::mt19937 eng(id + seed);
    std::uniform_real_distribution<> dis(0.0, 1.0);

    // Simulate the student's behavior
    if (is_studier) {
        dorm->study(id, last_study);
        last_study = std::chrono::steady_clock::now();
    }

    size_t waiting_count = 0;
    while (!dorm->is_everyone_sleeping()) {
        ++waiting_count;
        if (waiting_count > wait_limit)
            pthread_exit(nullptr);

        // Check if the student decides to study
        if (!is_studier && dorm->check_and_decide(id, eng, dis)) {
            dorm->study(id, last_study);
            last_study = std::chrono::steady_clock::now();
        }
        std::this_thread::sleep_for(wait_time);
    }

    // Pop the cleanup handler
    pthread_cleanup_pop(0);
    pthread_exit(nullptr);
}

/**
 * The main entry point of the program, simulating a dorm room scenario with multiple students.
 *
 * @return 0 on successful execution
 *
 * @throws None
 */
int main() {
    DormRoom dorm;
    std::array<pthread_t, person_number> threads;
    std::array<StudentArgs, person_number> args;

    // Create threads for students
    for (size_t i = 0; i < person_number; ++i) {
        args[i] = {&dorm, i, i < person_number / 2};
        pthread_create(&threads[i], nullptr, student_routine, &args[i]);
    }

    // Wait for all threads to finish
    for (auto &thread : threads)
        pthread_join(thread, nullptr);

    // Print a message to indicate the end of the simulation
    dorm.print_message("Close the light");
    return 0;
}
