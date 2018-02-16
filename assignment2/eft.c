//holder data types

typedef struct analysis_results
{
	int is_schedulable;
	float utilization;
} analysis_results;

typedef struct task_type
{
	float wcet;
	float deadline;
	float period;
} task_type;

analysis_results edf_analysis (int size, task_type *data)
{
	analysis_results results = {0, 0};

	for(int i = 0; i < size; i++)
	{
		results.utilization += data[i].wcet / data[i].period;
	}

	if(results.utilization >= 1.0) results.is_schedulable = 1;

	return results;
}
