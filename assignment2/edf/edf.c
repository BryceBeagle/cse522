#include "../task_types.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

unsigned int for_some_index_less_than_or_equal(double *list, int size, double value)
{
	unsigned int ret = 0;
	for(int i = 0; i < size; i++)
	{
		ret |= (list[i] <= value);
	}
	return ret;
}

unsigned int smallest_index(double *list, int size)
{
	unsigned int index = 0;
	for(int i = 0; i < size; i++)
	{
		if(list[i] < list[index]){
			index = i;
		}
	}
	return index;
}

analysis_results edf_analysis (TaskSet *task_set)
{
	// fprintf(stderr, "EDF\n");
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
		//get L value
		double l;
		{
			l = 0.0;
			for(int i = 0; i < task_set->num_tasks; i++)
			{
				l += task_set->tasks[i].wcet;
			}

			double new_l = l;
			do{
				l = new_l;
				new_l = 0.0;
				for(int i = 0; i < task_set->num_tasks; i++)
				{
					Task *t = &(task_set->tasks[i]);
					new_l += ceil(l / t->period) * t->wcet;
				}
			}while(l > (new_l * 1.01) || l < (new_l * 0.99));
		}


		//get loading factor
		// make array to keep track of time stamps for each task, initialize array to respective task deadlines
		// make h variable, initialize to zero
		// while some array value < l
			// find smallest value in array, indicating nearest deadline
			// increase h by that deadline's task's wcet
			// check if utilization > 1.0. if so, return
			// increase that task's next deadline by it's period

		double time_stamps[task_set->num_tasks];
		double h;

		for(int i = 0; i < task_set->num_tasks; i++)
		{
			time_stamps[i] = task_set->tasks[i].deadline;
		}
		h = 0.0;


		while(for_some_index_less_than_or_equal(time_stamps, task_set->num_tasks, l))
		{
			unsigned int index = smallest_index(time_stamps, task_set->num_tasks);
			h += task_set->tasks[index].wcet;
			// printf("h:%lf\n", h);
			// printf("t:%lf\n", time_stamps[index]);
			if(h > time_stamps[index]){
				// fputs("Not Schedulable\n", stderr);
				return results;
			}
			time_stamps[index] += task_set->tasks[index].period;
		}
	}

	// fputs("Schedulable\n", stderr);
	results.is_schedulable = 1;
	return results;
}
