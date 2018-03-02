#include <stdio.h>
#include <stdlib.h>

#include "task_types.h"
#include "edf/edf.h"
#include "rm/rm.h"
#include "dm/dm.h"

#define RESULTS_FILE_EDF "out/results_edf.txt"
#define RESULTS_FILE_RM  "out/results_rm.txt"
#define RESULTS_FILE_DM  "out/results_dm.txt"

/*
 *  FORWARD DECLARATIONS
 */

ProgramInfo parseFile(char *filename);
void writeFile(analysis_results results, FILE *file);

/*
 *  Function Bodies
 */

int main(int argc, char *argv[]) {

    ProgramInfo program = parseFile(argv[1]);
    FILE *results_edf = fopen(RESULTS_FILE_EDF, "w+");
    FILE *results_rm  = fopen(RESULTS_FILE_RM , "w+");
    FILE *results_dm  = fopen(RESULTS_FILE_DM , "w+");

    for (int i = 0; i < program.num_task_sets; i++) {

//        fprintf(stderr, "%i\n", i);

        TaskSet task_set = program.task_sets[i];

        writeFile(edf_analysis(&task_set), results_edf);
        writeFile( rm_analysis(&task_set), results_rm );
        writeFile( dm_analysis(&task_set), results_dm );

    }

}

ProgramInfo parseFile(char *filename) {

    // TODO: Too long. segment into multiple functions

    ProgramInfo program;
    FILE *file = (filename == NULL ? stdin : fopen(filename, "r"));

    char line[256];  // Should be long enough

    // First line designating number of task sets
    if(!fgets(line, sizeof(line), file)) exit(-1);
    program.num_task_sets = (int) strtoul(line, NULL, 10);
    program.task_sets = malloc(program.num_task_sets * sizeof(TaskSet));

    // Task set declaration
    for (unsigned int i = 0; i < program.num_task_sets; i++) {

        TaskSet *task_set = &program.task_sets[i];

        // Get line declaring task set
        if(!fgets(line, sizeof(line), file)) exit(-1);

        // Set up task set
        task_set->num_tasks = (unsigned int) strtol(line, NULL, 10);
        task_set->tasks = malloc(task_set->num_tasks * sizeof(Task));

        // Task declaration
        for (unsigned int j = 0; j < task_set->num_tasks; j++) {

            // Get line declaring task
            if(!fgets(line, sizeof(line), file)) exit(-1);

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

void writeFile(analysis_results results, FILE *file) {

    fprintf(file, "%i %f\n", results.is_schedulable, results.utilization);

}