#include <cassert>
#include <chrono>
#include <format>
#include <iostream>
#include <map>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <unistd.h>
#include <vector>

constexpr size_t available_count = 30;
constexpr size_t patient_count = 35;
constexpr size_t doctor_count = 3;
constexpr int treatment_time = 2;

enum class PatientState {
    Unknown,
    Waiting,
    BeingTreated,
    Left,
};

struct PatientInfo {
    PatientState state;
    std::optional<size_t> doctor_id;

    /**
     * @brief Constructs a PatientInfo object with a default state of Unknown and no doctor ID.
     *
     * This constructor initializes a PatientInfo instance with the default state
     * PatientState::Unknown and leaves the doctor ID unset.
     */
    PatientInfo() : state(PatientState::Unknown) {
    }

    /**
     * @brief Constructs a PatientInfo object with a given state.
     *
     * This constructor initializes a PatientInfo instance with the provided state.
     * The doctor ID is left unset.
     *
     * @param state The state of the patient.
     */
    PatientInfo(PatientState state) : state(state) {
    }

    /**
     * @brief Constructs a PatientInfo object with a given state and doctor ID.
     *
     * This constructor initializes a PatientInfo instance with the provided state
     * and associates it with a doctor ID. It asserts that the state must be
     * PatientState::BeingTreated, indicating that the doctor is currently working
     * with the specified patient.
     *
     * @param state The state of the patient, must be PatientState::BeingTreated.
     * @param doctor_id The identifier of the doctor treating the patient.
     */
    PatientInfo(PatientState state, size_t doctor_id) : state(state), doctor_id(doctor_id) {

        assert(state == PatientState::BeingTreated);
    }
};

enum class DoctorState {
    Unknown,
    Resting,
    Working,
};

struct DoctorInfo {
    DoctorState state;
    std::optional<size_t> patient_id;
    /**
     * @brief Constructs a DoctorInfo object with a default state of Unknown and an empty patient ID.
     *
     * This constructor initializes a DoctorInfo instance with the default state
     * DoctorState::Unknown and an empty patient ID. The doctor ID is not set.
     */
    DoctorInfo() : state(DoctorState::Unknown), patient_id(std::nullopt) {
    }

    /**
     * @brief Constructs a DoctorInfo object with a specified state and no patient ID.
     *
     * This constructor initializes a DoctorInfo instance with the given state.
     * It is used to set the doctor's state without associating it with any patient.
     *
     * @param state The current state of the doctor.
     */
    DoctorInfo(DoctorState state) : state(state) {
    }

    /**
     * @brief Constructs a DoctorInfo object with a specified state and patient ID.
     *
     * This constructor initializes a DoctorInfo instance with the given state
     * and associates it with a patient ID. It asserts that the state must be
     * DoctorState::Working, indicating that the doctor is currently working with
     * the specified patient.
     *
     * @param state The current state of the doctor, must be DoctorState::Working.
     * @param patient_id The identifier of the patient being treated by the doctor.
     */
    DoctorInfo(DoctorState state, size_t patient_id) : state(state), patient_id(patient_id) {
        assert(state == DoctorState::Working);
    }
};

std::vector<PatientInfo> PState(patient_count);
std::vector<DoctorInfo> DState(doctor_count);

size_t available_tickets = available_count;
std::queue<size_t> patient_queue;
std::map<size_t, size_t> patient_doctor;

pthread_mutex_t mutex_ticket, mutex_cout;
pthread_mutex_t mutex_patient, mutex_doctor;
std::vector<pthread_cond_t> cond_patient(patient_count);
sem_t sem_patient;

/**
 * @brief Prints a message to the console, thread-safe.
 *
 * @param message the message to print
 */
static inline auto print(const std::string_view message) -> void {
    pthread_mutex_lock(&mutex_cout);
    std::cout << message << std::endl;
    pthread_mutex_unlock(&mutex_cout);
}

/**
 * @brief A thread function that models a patient in the hospital.
 *
 * This function first tries to get a ticket. If it succeeds, it sets the state of
 * the patient as waiting and goes to sleep until a doctor is ready to treat it.
 * If it fails, it sets the state of the patient as left and returns.
 *
 * @param arg the identifier of the patient
 */
static inline auto patient(void *arg) -> void * {
    const size_t id = *static_cast<size_t *>(arg);

    bool has_ticket = false;
    // Take a ticket if available
    pthread_mutex_lock(&mutex_ticket);
    if (available_tickets > 0) {
        --available_tickets;
        has_ticket = true;
    } else
        has_ticket = false;
    pthread_mutex_unlock(&mutex_ticket);

    if (!has_ticket) {
        // If no ticket was available, set the patient state to left
        pthread_mutex_lock(&mutex_patient);
        PState[id] = PatientState::Left;
        pthread_mutex_unlock(&mutex_patient);

        print(std::format("Patient {} couldn't get a ticket and left.", id));

        pthread_exit(nullptr);
    }

    print(std::format("Patient {} got a ticket.", id));

    // Patient is waiting
    pthread_mutex_lock(&mutex_patient);
    PState[id] = PatientState::Waiting;
    patient_queue.push(id);
    pthread_mutex_unlock(&mutex_patient);

    // Inform a doctor that a patient is available
    sem_post(&sem_patient);

    print(std::format("Patient {} is waiting for a doctor.", id));

    // Wait for a doctor to be ready
    pthread_mutex_lock(&mutex_patient);
    while (PState[id].state != PatientState::BeingTreated)
        pthread_cond_wait(&cond_patient[id], &mutex_patient);
    size_t doctor_id = PState[id].doctor_id.value();
    pthread_mutex_unlock(&mutex_patient);

    print(std::format("Patient {} is being treated by doctor {}.", id, doctor_id));

    // Simulate treatment time
    sleep(treatment_time);

    // Patient is leaving
    pthread_mutex_lock(&mutex_patient);
    PState[id] = PatientState::Left;
    pthread_mutex_unlock(&mutex_patient);

    print(std::format("Patient {} left.", id));

    pthread_exit(nullptr);
}

/**
 * @brief A cleanup handler for a doctor thread.
 *
 * This function is called when a doctor thread is canceled. It sets the state of
 * the doctor to Unknown, ensuring that the doctor is not considered active in
 * the system anymore.
 *
 * @param arg a pointer to the identifier of the doctor
 */
static inline auto doctor_cleanup(void *arg) -> void {
    size_t id = *static_cast<size_t *>(arg);

    // Set state of doctor to Unknown
    pthread_mutex_lock(&mutex_doctor);
    DState[id] = DoctorInfo(DoctorState::Unknown);
    pthread_mutex_unlock(&mutex_doctor);
}

/**
 * @brief Thread function for a doctor.
 *
 * This function implements a thread for a doctor. The doctor waits for a
 * patient to be available, then treats the patient. The doctor then waits
 * for the next patient.
 *
 * @param arg a pointer to the identifier of the doctor
 *
 * @return NULL
 */
static inline auto doctor(void *arg) -> void * {
    size_t id = *static_cast<size_t *>(arg);

    // Register cleanup handler for the doctor thread
    pthread_cleanup_push(doctor_cleanup, arg);

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
    while (true) {
        // Set doctor state to resting and print status
        pthread_mutex_lock(&mutex_doctor);
        DState[id] = DoctorState::Resting;
        pthread_mutex_unlock(&mutex_doctor);

        print(std::format("Doctor {} is waiting for patient.", id));

        // Wait for a patient to be available
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
        sem_wait(&sem_patient);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

        // Assign a patient to the doctor
        pthread_mutex_lock(&mutex_patient);
        size_t patient_id = patient_queue.front();
        patient_queue.pop();
        PState[patient_id] = PatientInfo(PatientState::BeingTreated, id);
        patient_doctor[patient_id] = id;
        pthread_mutex_unlock(&mutex_patient);

        // Update doctor state to working with the assigned patient
        pthread_mutex_lock(&mutex_doctor);
        DState[id] = DoctorInfo(DoctorState::Working, patient_id);
        pthread_mutex_unlock(&mutex_doctor);

        // Signal the patient to start treatment and print status
        pthread_cond_signal(&cond_patient[patient_id]);
        print(std::format("Doctor {} is treating patient {}.", id, patient_id));

        // Simulate treatment time
        sleep(treatment_time);

        // Print status after finishing treatment
        print(std::format("Doctor {} finished treating patient {}.", id, patient_id));
    }

    // Remove cleanup handler (never reached due to infinite loop)
    pthread_cleanup_pop(0);
    pthread_exit(nullptr);
}

/**
 * @brief The main entry point of the program.
 *
 * This function initializes mutexes and semaphores, creates threads for each
 * patient and doctor, waits for all patient threads to finish, then cancels
 * and waits for all doctor threads to finish. Finally, it prints some
 * statistics.
 *
 * @return 0 on success
 */
int main() {
    pthread_mutex_init(&mutex_ticket, nullptr);
    pthread_mutex_init(&mutex_cout, nullptr);
    pthread_mutex_init(&mutex_patient, nullptr);
    pthread_mutex_init(&mutex_doctor, nullptr);
    sem_init(&sem_patient, 0, 0);

    std::vector<size_t> patient_args(patient_count);
    std::vector<pthread_t> patients(patient_count);
    for (size_t i = 0; i < patient_count; ++i) {
        patient_args[i] = i;
        pthread_cond_init(&cond_patient[i], nullptr);
        pthread_create(&patients[i], nullptr, patient, &patient_args[i]);
    }

    std::vector<size_t> doctor_args(doctor_count);
    std::vector<pthread_t> doctors(doctor_count);
    for (size_t i = 0; i < doctor_count; ++i) {
        doctor_args[i] = i;
        pthread_create(&doctors[i], nullptr, doctor, &doctor_args[i]);
    }

    for (size_t i = 0; i < patient_count; ++i) {
        pthread_join(patients[i], nullptr);
        pthread_cond_destroy(&cond_patient[i]);
    }

    for (size_t i = 0; i < doctor_count; ++i) {
        pthread_cancel(doctors[i]);
        pthread_join(doctors[i], nullptr);
    }

    pthread_mutex_destroy(&mutex_ticket);
    pthread_mutex_destroy(&mutex_cout);
    pthread_mutex_destroy(&mutex_patient);
    pthread_mutex_destroy(&mutex_doctor);
    sem_destroy(&sem_patient);

    std::cout << std::format("\n################################################\n") << std::endl;

    std::cout << std::format("There were {} patients and {} doctors.", patient_count, doctor_count) << std::endl;
    std::cout << std::format("There were {} tickets left.", available_tickets) << std::endl;
    for (size_t i = 0; i < patient_count; ++i)
        if (patient_doctor.find(i) != patient_doctor.end())
            std::cout << std::format("\tPatient {} was treated by doctor {}.", i, patient_doctor[i]) << std::endl;
        else
            std::cout << std::format("\tPatient {} was not treated.", i) << std::endl;

    return 0;
}
