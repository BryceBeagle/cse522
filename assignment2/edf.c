#include "task_types.h"

analysis_results edf_analysis (int size, Task *data)
{
	analysis_results results = {0, 0};

	for(int i = 0; i < size; i++)
	{
		results.utilization += data[i].wcet / data[i].period;
	}

	if(results.utilization <= 1.0) results.is_schedulable = 1;

	return results;
}
