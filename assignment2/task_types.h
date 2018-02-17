#define _GNU_SOURCE

#ifndef TASK_TYPES_H
#define TASK_TYPES_H

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

typedef struct analysis_results
{
	int is_schedulable;
	double utilization;
} analysis_results;

#endif
