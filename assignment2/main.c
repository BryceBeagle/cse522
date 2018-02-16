#include <stdio.h>
#include <stdlib.h>

#include "main.h"

int main(int argc, char *argv[]) {
    ProgramInfo program = parseFile(argv[1]);
}

ProgramInfo parseFile(char *filename) {

    // TODO: Too long. segment into multiple functions

    ProgramInfo program;
    FILE *file = (filename == NULL ? stdin : fopen(filename, "r"));

    char line[256];  // Should be long enough

    // First line designating number of task sets
    fgets(line, sizeof(line), file);
    program.num_task_sets = (int) strtoul(line, NULL, 10);
    program.task_sets = malloc(program.num_task_sets * sizeof(TaskSet*));

    // Task set declaration
    for (unsigned int i = 0; i < program.num_task_sets; i++) {

        TaskSet *task_set = &program.task_sets[i];

        // Get line declaring task set
        fgets(line, sizeof(line), file);

        // Set up task set
        task_set->num_tasks = (unsigned int) strtol(line, NULL, 10);
        task_set->tasks = malloc(task_set->num_tasks * sizeof(Task));

        // Task declaration
        for (unsigned int j = 0; j < task_set->num_tasks; j++) {

            // Get line declaring task
            fgets(line, sizeof(line), file);

            Task *task = &task_set->tasks[j];

            char *state;

            // WCET
            task->wcet = strtod(line, &state);

            // Deadline
            task->deadline = strtod(state, &state);

            // Period
            task->period = strtod(state, &state);

        }

    }

    return program;
}