#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////
// MACRO DEFINITIONS

// TODO: Why did I make this a macro again?
#define busyLoop(iterations) \
    for (int i=0, j=0; i < (iterations); i++) { \
        j += i; \
    } \

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
    long period;  // long was chosen arbitrarily
    Operation *operations;

} Thread;

typedef struct {

    long numThreads;  // long was chosen arbitrarily
    long duration;    // long was chosen arbitrarily
    Thread *threads;

} ProgramInfo;


////////////////////////////////////////////////////////////////////////////////
// THREADS

void periodic(void *structure) {

    // Local variables
    // Init

    // Wait for activation

//    while (1) {
//
//        busyLoop(0);
//        // Wait for period
//
//    }

}


void aperiodic(void *structure) {

    // Local variables
    // Init

    // Wait for activation

    // Wait for event

//    while(1) {
//
//        if (lock) {
//            pthread_mutex_lock(mutexes[lockNumber]);
//        }
//
//        if (unlock) {
//            pthread_mutex_unlock(mutexes[lockNumber]);
//        }
//
//        busyLoop(0);
//        // Wait of event
//
//    }

}



////////////////////////////////////////////////////////////////////////////////
//MAIN PROGRAM CODE

ProgramInfo parseFile(char *filename) {

    ProgramInfo program;
    FILE* file = fopen(filename, "r");  // TODO: Default?

    char line[256];  // Should be long enough
    char *word_end;

    // First line designating number of threads and runtime
    fgets(line, sizeof(line), file);
    program.numThreads = strtol(line, &word_end, 10);
    program.duration   = strtol(line, &word_end, 10);
    program.threads    = malloc(program.numThreads * sizeof(Thread));

    // Thread declaration
    char *token;
    char *state;

    for (int i = 0; i < program.numThreads; i++) {

        Thread thread = program.threads[i];

        // Get line declaring thread
        fgets(line, sizeof(line), file);

        Operation *root;
        Operation **operation = &root;

        thread.operations = root;

        while(token = strtok_r(line, " ", &state)) {

            *operation = malloc(sizeof(Operation));

            switch(token[0]) {
                case 'P':
                    thread.threadType = PERIODIC;
                    continue;  // Don't make this an operation
                case 'A':
                    thread.threadType = APERIODIC;
                    continue;  // Don't make this an operation
                case 'L':
                    (*operation)->operation = LOCK;
                    (*operation)->value = strtol(token + 1, NULL, 10);
                    break;
                case 'U':
                    (*operation)->operation = UNLOCK;
                    (*operation)->value = strtol(token + 1, NULL, 10);
                    break;
                default:
                    (*operation)->operation = BUSY_LOOP;
                    (*operation)->value = strtol(token, NULL, 10);
                    break;
            }

            operation = &(*operation)->nextOp;

        }

        // Terminate last node
        *operation = NULL;
    }
}

void main(int argc, char* argv[]) {

    ProgramInfo threadStructure = parseFile(argv[1]);

    pthread_t threads[threadStructure.numThreads];

    // Parse file
    // Alloc space for each thread




}