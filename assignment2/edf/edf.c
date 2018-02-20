#include "../task_types.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

analysis_results edf_analysis (TaskSet *task_set)
{
	fprintf(stderr, "EDF\n");
	analysis_results results = {0, 0.0};
	double density = 0.0;

	//get utilization
	for(int i = 0; i < task_set->num_tasks; i++)
	{
		Task *t = &(task_set->tasks[i]);
		results.utilization += t->wcet / t->period;
	}

	//get density
	for(int i = 0; i < task_set->num_tasks; i++)
	{
		Task *t = &(task_set->tasks[i]);
		density += t->wcet / fmin(t->period, t->deadline);
	}

	if(density > 1.0){
		//get loading factor
		for(int i = 0; i < task_set->num_tasks; i++)
		{
			double loading_factor = 0.0;
			Task *t_reference = &(task_set->tasks[i]);
			for(int j = 0; j < task_set->num_tasks; j++)
			{
				Task *t_iteration = &(task_set->tasks[j]);
				if(t_iteration->deadline <= t_reference->deadline){
					loading_factor += t_iteration->wcet;
				}
			}

			if(loading_factor / t_reference->deadline > 1.0){
				// too high a load
				fputs("Not Schedulable\n", stderr);
				return results;
			}
		}
	}

	fputs("Schedulable\n", stderr);
	results.is_schedulable = 1;
	return results;
}
