#include "../task_types.h"


analysis_results edf_analysis (TaskSet *task_set)
{
	analysis_results results = {0, 0};

	for(int i = 0; i < task_set->num_tasks; i++)
	{
		results.utilization += task_set->tasks[i].wcet / task_set->tasks[i].period;
	}

	if(results.utilization <= 1.0) results.is_schedulable = 1;

	return results;
}
