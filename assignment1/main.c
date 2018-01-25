#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <linux/input.h>

////////////////////////////////////////////////////////////////////////////////
// Global variables
pthread_mutex_t mutexes[10] = {PTHREAD_MUTEX_ERRORCHECK};
pthread_mutex_t activate_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t activate_left = PTHREAD_COND_INITIALIZER;
pthread_cond_t activate_right = PTHREAD_COND_INITIALIZER;


////////////////////////////////////////////////////////////////////////////////
// DATA STRUCTURES

typedef enum {LOCK, UNLOCK, BUSY_LOOP} OperationType;
typedef enum {PERIODIC, APERIODIC} ThreadType;

typedef struct Operation{

    OperationType operation;
    long value;  // Mutex number or number of iterations. long was chosen arbitrarily

    struct Operation *nextOp;

} Operation;

typedef struct Thread {

    ThreadType threadType;
    long priority;  // long was chosen arbitrarily
    long period;    // long was chosen arbitrarily
    long trigger;   // long was chosen arbitrarily

    Operation *operations;

} Thread;

typedef struct {

    long numThreads;  // long was chosen arbitrarily
    long duration;    // long was chosen arbitrarily
    Thread *threads;

} ProgramInfo;

void busyLoop(int iterations);
void next_operation(Operation **ptr);
int msleep(struct timespec start, int msec);
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
        mouse_left = ptr[0] & (unsigned char)0x1;
        mouse_right = (ptr[0] & (unsigned char)0x2);
        if(mouse_left){
            fprintf(stderr, "LEFT\n");
            pthread_cond_broadcast(&activate_left);
        }
        if(mouse_right){
            fprintf(stderr, "RIGHT\n");
            pthread_cond_broadcast(&activate_right);
        }
    }
    return NULL;
}

void busyLoop(int iterations) {
    for (int i=0, j=0; i < (iterations); i++) {
        j += i;
    }
}

void next_operation(Operation **ptr) {
    if(ptr != NULL && *ptr != NULL) *ptr = (*ptr)->nextOp;
}

int msleep(struct timespec start, int msec) {

        start.tv_sec += (long)msec / 1000;
        start.tv_nsec += ((long)msec%1000) * 1000000L;

        if(start.tv_nsec > 999999999){
                start.tv_nsec -= 1000000000;
                start.tv_sec++;
        }

        return clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &start, NULL);
}

void *periodic(void *ptr) {

    Thread *thread = (Thread *)ptr;

    // Local variables
    Operation *current_operation = thread->operations;

    // Init

    // Wait for activation
    pthread_mutex_lock(&activate_mut);
    pthread_mutex_unlock(&activate_mut);

    while (1) {
        struct timespec start_time;
        clock_gettime(CLOCK_REALTIME, &start_time);

        switch(current_operation->operation) {
            case LOCK      :
                pthread_mutex_lock(&mutexes[current_operation->value]);
                fprintf(stderr, "%lu :: LOCK %ld\n", pthread_self(), current_operation->value);
                break;
            case UNLOCK    :
                pthread_mutex_unlock(&mutexes[current_operation->value]);
                fprintf(stderr, "%lu :: UNLOCK %ld\n", pthread_self(), current_operation->value);
                break;
            case BUSY_LOOP :
                busyLoop(current_operation->value);
                fprintf(stderr, "%lu :: BUSY_LOOP %ld\n", pthread_self(), current_operation->value);
                break;
        }

        next_operation(&current_operation); //advance
        if(current_operation == NULL){ // loop back around
            current_operation = thread->operations;
        }

        // Wait for period
        if(msleep(start_time, thread->period) == -1){
            break;
        }
    }

    return NULL;
}

void *aperiodic(void *ptr) {

    Thread *thread = (Thread *)ptr;

    // Local variables
    // Init

    // Wait for activation

    // Wait for event

    while(1) {

       // if (lock) {
       //     pthread_mutex_lock(mutexes[lockNumber]);
       // }

       // if (unlock) {
       //     pthread_mutex_unlock(mutexes[lockNumber]);
       // }

        busyLoop(0);
        // Wait of event

    }

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
    program.numThreads = strtol(line,     &word_end, 10);
    program.duration   = strtol(word_end, &word_end, 10);
    program.threads    = malloc(program.numThreads * sizeof(Thread));

    // Thread declaration
    char *token;
    char *state;

    for (int i = 0; i < program.numThreads; i++) {

        Thread *thread = &program.threads[i];

        // Get line declaring thread
        fgets(line, sizeof(line), file);

        // Thread type
        token = strtok_r(line, " ", &state);
        switch(token[0]) {
            case 'P': thread->threadType = PERIODIC; break;
            case 'A': thread->threadType = APERIODIC; break;
            default: break;
        }

        // Thread priority
        token = strtok_r(NULL, " ", &state);
        thread->priority = strtol(token, NULL, 10);

        // Aperiodic Trigger
        if (thread->threadType == APERIODIC) {
            token = strtok_r(NULL, " ", &state);
            thread->trigger = strtol(token, NULL, 10);
        }

        // Thread period
        token = strtok_r(NULL, " ", &state);
        thread->period = strtol(token, NULL, 10);

        // Thread operations
        Operation *root = NULL;
        Operation *tail = NULL;

        token = strtok_r(NULL, " ", &state);
        while(token) {

            Operation *operation = malloc(sizeof(Operation));

            switch(token[0]) {
                case 'L':
                    operation->operation = LOCK;
                    operation->value = strtol(token + 1, NULL, 10);
                    break;
                case 'U':
                    operation->operation = UNLOCK;
                    operation->value = strtol(token + 1, NULL, 10);
                    break;
                default:
                    operation->operation = BUSY_LOOP;
                    operation->value = strtol(token, NULL, 10);
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
    cpu_set_t cpuset;

    pthread_mutex_lock(&activate_mut);

    CPU_SET(1, &cpuset);
    pthread_create(&mouse_watcher, NULL, mouse_reader, "/dev/input/mouse0");
    pthread_setaffinity_np(mouse_watcher, sizeof(cpu_set_t), &cpuset);
    for (int i=0; i < program.numThreads; i++) {
        switch (program.threads[i].threadType) {
            case  PERIODIC:
                {
                    pthread_attr_t attr;
                    struct sched_param param = {program.threads[i].priority};
                    pthread_attr_init(&attr);
                    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
                    pthread_attr_setschedparam(&attr, &param);
                    pthread_create(&threads[i], &attr, periodic, &program.threads[i]);
                    pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpuset);
                }
                break;
            // case APERIODIC: aperiodic(&program.threads[i]); break;
            default: break;
        }
    }

    pthread_mutex_unlock(&activate_mut);

    struct timespec start_time;
    clock_gettime(CLOCK_REALTIME, &start_time);
    msleep(start_time, program.duration);

    return 0;
}