#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/syscall.h>

////////////////////////////////////////////////////////////////////////////////
// Global variables
pthread_barrier_t thread_sync;

pthread_mutexattr_t mta;
pthread_mutex_t mutexes[10];

pthread_mutex_t event_mut;
pthread_cond_t event_cond[2];

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

int msleep(struct timespec start, long msec);

void *periodic(void *ptr);
void *aperiodic(void *ptr);

ProgramInfo parseFile(char *filename);

void *mouse_reader(void *filename);

long int get_tid();

void print_thread_info(char *descriptor);
void print_pthread_create_info(int err);

////////////////////////////////////////////////////////////////////////////////
// THREADS

long int get_tid() {
    return syscall(SYS_gettid);
}

void print_pthread_create_info(int err) {
    fprintf(stderr, "pthread_create error code %s\n",
        (err == EAGAIN ? "EAGAIN"
        : (err == EINVAL ? "EINVAL"
        : (err == EPERM ? "EPERM"
        : (err == 0 ? "GOOD"
        : "unknown")))));
}

void print_thread_info(char *descriptor) {
    int my_policy;
    struct sched_param my_param;

    pthread_getschedparam (pthread_self(), &my_policy, &my_param);
    fprintf (stderr, "# %s %ld :: thread_routine running at %s/%d\n",
        descriptor,
        get_tid(),
        (my_policy == SCHED_FIFO ? "FIFO"
        : (my_policy == SCHED_RR ? "RR"
        : (my_policy == SCHED_OTHER ? "OTHER"
        : "unknown"))),
        my_param.sched_priority);
}

void *mouse_reader(void *filename) {

    print_thread_info("mouse_reader");

    int fd;
    struct input_event ie;
    unsigned char mouse_left, mouse_right;
    unsigned char previous_mouse_left, previous_mouse_right;

    if((fd = open((char *)filename, O_RDONLY)) == -1) {
        fprintf(stderr, "Device open ERROR\n");
        exit(-1);
    }

    {
        read(fd, &ie, sizeof(struct input_event));
        unsigned char *ptr = (unsigned char*)&ie;
        previous_mouse_left  = ptr[0] & (unsigned char)0x1;
        previous_mouse_right = ptr[0] & (unsigned char)0x2;
    }

    while(read(fd, &ie, sizeof(struct input_event))){
        unsigned char *ptr = (unsigned char*)&ie;
        mouse_left  = ptr[0] & (unsigned char)0x1;
        mouse_right = ptr[0] & (unsigned char)0x2;

        if(mouse_left < previous_mouse_left){ // transition from high to low
            fprintf(stderr, "LEFT\n");
            pthread_cond_broadcast(&event_cond[LEFT]);
        }
        if(mouse_right < previous_mouse_right){ // transition from high to low
            fprintf(stderr, "RIGHT\n");
            pthread_cond_broadcast(&event_cond[RIGHT]);
        }

        previous_mouse_left  = mouse_left;
        previous_mouse_right = mouse_right;
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

    // Disable thread cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    switch((*operation)->operation) {
        case LOCK      :
            pthread_mutex_lock(&mutexes[(*operation)->value]);
            fprintf(stdout, "%lu :: LOCK %ld\n", get_tid(), (*operation)->value);
            break;
        case UNLOCK    :
            pthread_mutex_unlock(&mutexes[(*operation)->value]);
            fprintf(stdout, "%lu :: UNLOCK %ld\n", get_tid(), (*operation)->value);
            break;
        case BUSY_LOOP :
            busyLoop((*operation)->value);
            fprintf(stdout, "%lu :: LOOP %ld\n", get_tid(), (*operation)->value);
            break;

    }

    // Re-enable thread cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
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
    struct timespec start_time;

    print_thread_info("periodic");

    // Wait for activation
    pthread_barrier_wait(&thread_sync);  // Sync all threads

    while (1) {
        // Get start time of thread
        clock_gettime(CLOCK_REALTIME, &start_time);

        pthread_testcancel();
        Operation *current_operation = thread->operations;

        while (current_operation != NULL) {
            do_operation(&current_operation);
            next_operation(&current_operation); //advance
        }

        // Wait for completion of period if thread has finished early
        if (msleep(start_time, thread->period) != 0) {
            fprintf(stderr, "BREAKING %ld\n", get_tid());
            break;
        }
    }

    return NULL;
}

void *aperiodic(void *ptr) {

    Thread *thread = (Thread *)ptr;

    print_thread_info("aperiodic");

    // Wait for activation
    pthread_barrier_wait(&thread_sync);

    while(1) {

        // Wait for next event
        pthread_mutex_lock(&event_mut);
        pthread_cond_wait(&event_cond[thread->event], &event_mut);
        pthread_mutex_unlock(&event_mut);

        pthread_testcancel();

        Operation *current_operation = thread->operations;

        while (current_operation != NULL) {
            do_operation(&current_operation);
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
    FILE* file = (filename == NULL ? stdin : fopen(filename, "r"));

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
    fprintf(stderr, "main :: %ld\n", get_tid());

    ProgramInfo program = parseFile(argv[1]);

    pthread_t threads[program.numThreads];  // TODO: Use later
    pthread_t mouse_watcher;

    pthread_barrier_init(&thread_sync, NULL, program.numThreads + 1); // Parent thread + created threads

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);

    // Initialize Mutex
    {
        pthread_cond_init(&event_cond[LEFT], NULL);
        pthread_cond_init(&event_cond[RIGHT], NULL);
        pthread_mutexattr_init(&mta);
        #ifdef PI
            pthread_mutexattr_setprotocol(&mta, PTHREAD_PRIO_INHERIT);
            printf("PI ENABLED\n");
        #else
            printf("PI -NOT- ENABLED\n");
        #endif
        pthread_mutex_init(&event_mut, &mta);
        for (int i = 0; i < sizeof(mutexes)/sizeof(mutexes[0]); i++) {
            pthread_mutex_init(&mutexes[i], &mta);
        }
    }

    //Start Mouse Watcher
    {
        struct sched_param param;
        pthread_attr_t attr;
        pthread_attr_init(&attr);

        param.sched_priority = sched_get_priority_max(SCHED_RR);
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setschedparam(&attr, &param);

        pthread_create(&mouse_watcher, &attr, mouse_reader, "/dev/input/mice");
        pthread_setaffinity_np(mouse_watcher, sizeof(cpu_set_t), &cpuset);
    }

    for (int i=0; i < program.numThreads; i++) {

        // Create attr for setting thread priority and policy
        struct sched_param param;

        // Thread attribute struct
        pthread_attr_t attr;
        pthread_attr_init(&attr);

        // Set thread priority, scheduling policy (FIFO), and affinity (1 CPU)
        param.sched_priority = program.threads[i].priority;
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        pthread_attr_setschedparam(&attr, &param);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);

        void *(*thread_function)(void *ptr) =
              (program.threads[i].thread_type ==  PERIODIC ? &periodic
            : (program.threads[i].thread_type == APERIODIC ? &aperiodic
            :  NULL));

        // Start thread using attr struct
        int err = pthread_create(&threads[i], &attr, thread_function, &program.threads[i]);
        if(err){
            print_pthread_create_info(err);
            exit(-1);
        }
    }

    pthread_barrier_wait(&thread_sync);
    fprintf(stderr, "Starting\n");

    usleep((unsigned int) program.duration * 1000);

    // Cancel all threads (cleanly)
    pthread_cancel(mouse_watcher);
    for (int i = 0; i < program.numThreads; i++) {
        int err = pthread_cancel(threads[i]);
        fprintf(stderr, "Thread %i cancellation requested: %i\n", i, err);
    }

    for (int i = 0; i < sizeof(mutexes)/sizeof(mutexes[0]); i++) {
        pthread_mutex_unlock(&mutexes[i]);
    }

    return 0;
}