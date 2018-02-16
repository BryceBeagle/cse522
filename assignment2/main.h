#define _GNU_SOURCE


////////////////////////////////////////////////////////////////////////////////
// DATA STRUCTURES

typedef struct Task {

    double wcet;
    double deadline;
    double period;

} Task;

typedef struct TaskSet {

    unsigned int num_tasks;
    Task *tasks;

} TaskSet;

typedef struct ProgramInfo {

    unsigned int num_task_sets;
    TaskSet *task_sets;

} ProgramInfo;


////////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

ProgramInfo parseFile(char *filename);
int main(int argc, char *argv[]);