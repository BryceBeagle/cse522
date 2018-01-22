#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


////////////////////////////////////////////////////////////////////////////////
// Global variables
pthread_mutex_t mutexes[10] = {PTHREAD_MUTEX_ERRORCHECK};


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


////////////////////////////////////////////////////////////////////////////////
// THREADS


void busyLoop(int iterations) {
    for (int i=0, j=0; i < (iterations); i++) {
        j += i;
    }
}

void *periodic(void *ptr) {

    Thread *thread = (Thread *)ptr;

    // Local variables
    // Init

    // Wait for activation

    while (1) {

        busyLoop(0);
        // Wait for period

    }

}

void *aperiodic(void *ptr) {

    Thread *thread = (Thread *)ptr;

    // Local variables
    // Init

    // Wait for activation

    // Wait for event

    while(1) {

//        if (lock) {
//            pthread_mutex_lock(mutexes[lockNumber]);
//        }
//
//        if (unlock) {
//            pthread_mutex_unlock(mutexes[lockNumber]);
//        }

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

void main(int argc, char* argv[]) {

    ProgramInfo program = parseFile(argv[1]);

    pthread_t threads[program.numThreads];  // TODO: Use later

    for (int i=0; i < program.numThreads; i++) {

        switch (program.threads[0].threadType) {
            case  PERIODIC:  pthread_create(&threads[i], NULL, periodic, &program.threads[0]); break;
            case APERIODIC: aperiodic(&program.threads[0]); break;
            default: break;
        }
    }
}