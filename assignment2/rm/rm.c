#include <math.h>
#include "../task_types.h"

analysis_results rm_analysis(TaskSet *task_set) {

    int n = task_set->num_tasks;

    analysis_results ret = {0, 0};

    for (int i = 0; i < n; i++) {
        Task task = task_set->tasks[i];
        ret.utilization += task.wcet / task.period;
    }

    ret.is_schedulable = ret.utilization < n * (pow(2, 1. / n) - 1);

    return ret;

}