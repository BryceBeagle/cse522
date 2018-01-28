#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

////////////////////////////////////////////////////////////////////////////////
// Global variables
pthread_barrier_t thread_sync;

pthread_mutex_t mutexes[10] = {PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP};
pthread_mutex_t activate_mut = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t event_mut = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
pthread_cond_t event_cond[2] = {PTHREAD_COND_INITIALIZER};

////////////////////////////////////////////////////////////////////////////////
// DATA STRUCTURES

typedef enum {LOCK, UNLOCK, BUSY_LOOP} OperationType;
typedef enum {PERIODIC, APERIODIC} ThreadType;
enum {LEFT, RIGHT};

typedef struct Operation{

    OperationType     operation;
    unsigned long     value;  // Mutex number or number of iterations. long was chosen arbitrarily

    struct Operation *nextOp;

} Operation;

typedef struct Thread {

    ThreadType    thread_type;
    unsigned int  priority;
    unsigned long period;    // long was chosen arbitrarily
    unsigned long event;     // long was chosen arbitrarily

    Operation    *operations;

} Thread;

typedef struct {

    unsigned int  numThreads;
    unsigned long duration;    // long was chosen arbitrarily
    Thread       *threads;

} ProgramInfo;

void busyLoop(long iterations);
void next_operation(Operation **ptr);
void do_operation(Operation **operation);

int overrun(struct timespec *start_time, struct timespec *current_time, int duration);
int msleep(struct timespec start, long msec);

void *periodic(void *ptr);
void *aperiodic(void *ptr);
ProgramInfo parseFile(char *filename);
void *mouse_reader(void *filename);

////////////////////////////////////////////////////////////////////////////////
// THREADS

void *mouse_reader(void *filename) {

    int fd;
    struct input_event ie;
    unsigned char mouse_left, mouse_right;

    if((fd = open((char *)filename, O_RDONLY)) == -1) {
        fprintf(stderr, "Device open ERROR\n");
        exit(-1);
    }

    while(read(fd, &ie, sizeof(struct input_event))){
        unsigned char *ptr = (unsigned char*)&ie;
        mouse_left  = ptr[0] & (unsigned char)0x1;
        mouse_right = ptr[0] & (unsigned char)0x2;

        if(mouse_left){
            fprintf(stderr, "LEFT\n");
            pthread_mutex_lock(&event_mut);
            pthread_cond_broadcast(&event_cond[LEFT]);
            pthread_mutex_unlock(&event_mut);
        }
        if(mouse_right){
            fprintf(stderr, "RIGHT\n");
            pthread_mutex_lock(&event_mut);
            pthread_cond_broadcast(&event_cond[RIGHT]);
            pthread_mutex_unlock(&event_mut);
        }
    }
    return NULL;
}

void busyLoop(long iterations) {
    for (int i=0, j=0; i < (iterations); i++) {
        j += i;
    }
}

void next_operation(Operation **ptr) {
    if(ptr != NULL && *ptr != NULL) *ptr = (*ptr)->nextOp;
}

void do_operation(Operation **operation) {

    switch((*operation)->operation) {
        case LOCK      :
            pthread_mutex_lock(&mutexes[(*operation)->value]);
            fprintf(stdout, "%lu :: LOCK %ld\n", pthread_self(), (*operation)->value);
            break;
        case UNLOCK    :
            pthread_mutex_unlock(&mutexes[(*operation)->value]);
            fprintf(stdout, "%lu :: UNLOCK %ld\n", pthread_self(), (*operation)->value);
            break;
        case BUSY_LOOP :
            busyLoop((*operation)->value);

            struct timespec t;
            clock_gettime(CLOCK_REALTIME, &t);
            fprintf(stdout, "%lu :: UNLOCK %ld\n", pthread_self(), (*operation)->value);
            break;
    }

}

// start_time + duration >? current_time
int overrun(struct timespec *start_time, struct timespec *current_time, int duration) {

    struct timespec end_time = {
        start_time->tv_sec + (duration / 1000),
        start_time->tv_sec + ((duration % 1000) * 1000000)
    };

    if (end_time.tv_nsec / 1000000000) {
        end_time.tv_sec += 1;
        end_time.tv_nsec -= 1000000000;
    }

    if (current_time->tv_sec == end_time.tv_sec) {
        return current_time->tv_nsec > end_time.tv_nsec;
    }
    else {
        return current_time->tv_sec > end_time.tv_sec;
    }

}

int msleep(struct timespec start, long msec) {

        start.tv_sec += msec / 1000;
        start.tv_nsec += (msec%1000) * 1000000L;

        if(start.tv_nsec > 999999999){
                start.tv_nsec -= 1000000000;
                start.tv_sec++;
        }

        return clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &start, NULL);
}

void *periodic(void *ptr) {

    Thread *thread = (Thread *)ptr;
    struct timespec start_time, current_time;

    // Wait for activation
    pthread_barrier_wait(&thread_sync);  // Sync all threads

    while (1) {

        Operation *current_operation = thread->operations;

        // Get start time of thread
        clock_gettime(CLOCK_REALTIME, &start_time);

        while (current_operation != NULL) {

            do_operation(&current_operation);

            pthread_testcancel();

            // Ensure thread has not overrun its period
            clock_gettime(CLOCK_REALTIME, &current_time);
            if (overrun(&start_time, &current_time, thread->period)) {
                fputs("Overrun\n", stderr);
                break;
            }

            next_operation(&current_operation); //advance

        }

        // Wait for completion of period if thread has finished early
        if (msleep(start_time, thread->period) == -1) {
            break;
        }

    }

    return NULL;
}

void *aperiodic(void *ptr) {

    Thread *thread = (Thread *)ptr;

    // Wait for activation
    pthread_barrier_wait(&thread_sync);

    while(1) {

        Operation *current_operation = thread->operations;

        while (current_operation != NULL) {

            // Wait for next event
            pthread_mutex_lock(&event_mut);
            pthread_cond_wait(&event_cond[thread->event], &event_mut);
            pthread_mutex_unlock(&event_mut);

            fprintf(stderr, "Click detected\n");

            do_operation(&current_operation);

            pthread_testcancel();

            next_operation(&current_operation);

        }
    }
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////
// MAIN PROGRAM CODE

ProgramInfo parseFile(char *filename) {

    // TODO: Too long. segment into multiple functions

    ProgramInfo program;
    FILE* file = fopen(filename, "r");  // TODO: Default?

    char line[256];  // Should be long enough
    char *word_end;

    // First line designating number of threads and runtime
    fgets(line, sizeof(line), file);
    program.numThreads = (int) strtoul(line, &word_end, 10);
    program.duration   = strtoul(word_end, &word_end, 10);
    program.threads    = malloc(program.numThreads * sizeof(Thread));

    // Thread declaration
    char *token;
    char *state;

    for (unsigned int i = 0; i < program.numThreads; i++) {

        Thread *thread = &program.threads[i];

        // Get line declaring thread
        fgets(line, sizeof(line), file);

        // Thread type
        token = strtok_r(line, " ", &state);
        switch(token[0]) {
            case 'P': thread->thread_type = PERIODIC; break;
            case 'A': thread->thread_type = APERIODIC; break;
            default: break;
        }

        // Thread priority
        token = strtok_r(NULL, " ", &state);
        thread->priority = (int) strtoul(token, NULL, 10);

        // Aperiodic thread trigger
        if (thread->thread_type == APERIODIC) {
            token = strtok_r(NULL, " ", &state);
            thread->event = strtoul(token, NULL, 10);  // 0 (LEFT) or 1 (RIGHT)
        }
        // Periodic thread period
        else {
            // Thread period
            token = strtok_r(NULL, " ", &state);
            thread->period = strtoul(token, NULL, 10);
        }

        // Thread operations
        Operation *root = NULL;
        Operation *tail = NULL;

        token = strtok_r(NULL, " ", &state);
        while(token) {

            Operation *operation = malloc(sizeof(Operation));

            switch(token[0]) {
                case 'L':
                    operation->operation = LOCK;
                    operation->value = strtoul(token + 1, NULL, 10);
                    break;
                case 'U':
                    operation->operation = UNLOCK;
                    operation->value = strtoul(token + 1, NULL, 10);
                    break;
                default:
                    operation->operation = BUSY_LOOP;
                    operation->value = strtoul(token, NULL, 10);
                    break;
            }

            if (tail == NULL) {
                tail = operation;
                root = operation;
            }
            else {
                tail->nextOp = operation;
                tail = operation;
            }

            // Get next token
            token = strtok_r(NULL, " ", &state);
        }

        // Terminate last node
        if (tail != NULL) {
            tail->nextOp = NULL;
        }

        thread->operations = root;

    }

    return program;

}

int main(int argc, char* argv[]) {

    ProgramInfo program = parseFile(argv[1]);

    pthread_t threads[program.numThreads];  // TODO: Use later
    pthread_t mouse_watcher;

    pthread_barrier_init(&thread_sync, NULL, program.numThreads + 1); // Parent thread + created threads

    cpu_set_t cpuset;
    CPU_SET(1, &cpuset);

    // Lock activation mutex
    pthread_mutex_lock(&activate_mut);

    pthread_create(&mouse_watcher, NULL, mouse_reader, "/dev/input/mice");
    pthread_setaffinity_np(mouse_watcher, sizeof(cpu_set_t), &cpuset);

    for (int i=0; i < program.numThreads; i++) {

        // Create attr for setting thread priority and policy
        struct sched_param param;

        // Thread attribute struct
        pthread_attr_t attr;
        pthread_attr_init(&attr);

        // Set thread priority, scheduling policy (FIFO), and affinity (1 CPU)
        param.sched_priority = program.threads[i].priority;
        pthread_attr_setschedparam(&attr, &param);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);

        // Start thread using attr struct
        switch (program.threads[i].thread_type) {
            case  PERIODIC: pthread_create(&threads[i], &attr,  periodic, &program.threads[i]); break;
            case APERIODIC: pthread_create(&threads[i], &attr, aperiodic, &program.threads[i]); break;
        }
    }

    pthread_barrier_wait(&thread_sync);
    fprintf(stderr, "Starting\n");

    usleep((unsigned int) program.duration * 1000);

    // Terminate all threads (cleanly)
    for (int i = 0; i < program.numThreads; i++) {
        fprintf(stderr, "Killing %i\n", i);
        int err = pthread_cancel(threads[i]);
        fprintf(stderr, "Thread %i closed: %i\n", i, err);
    }

    return 0;
}