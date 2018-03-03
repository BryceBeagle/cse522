#include <math.h>
#include <stdio.h>
#include "../task_types.h"

analysis_results dm_analysis(TaskSet *task_set) {

    /*
     * When the relative deadlines are arbitrary, the DM algorithm performs
     * better in the sense that it can sometimes produce a feasible schedule
     * when the RM algorithm fails, while the RM algorithm always fails when the
     * DM algorithm fails.
     *
     * A scheduling algorithm can feasibly schedule any set of periodic tasks on
     * a processor if the total utilization of the tasks is equal to or less
     * than the schedulable utilization of the algorithm.
     *
     * The EDF algorithm has another serious disadvantage. We note that a late
     * job which has already missed its deadline has a higher-priority than a
     * job whose deadline is still in the future. Consequently, if the execution
     * of a late job is allowed to continue, it may cause some other jobs to be
     * late.
     *
     * Time-demand analysis
     *
     *     Notes taken from these lecture slides:
     *     https://csperkins.org/teaching/2014-2015/adv-os/lecture02.pdf
     *
     *     For each job J_i,c released at the critical instance:
     *         if J_i,c and all higher priority tasks can exec before their deadlines:
     *             system is schedulable
     *
     *     Consider one task, T_i, in descending priority
     *     Focus on a job, J_i, in T_i, where its release time, t_0, is at crit instance
     *     At time [t_0 + t] for [t >= 0], the processor time demand w_i(t) is
     *
     *         e_i + ∑(k=1 → i-1, ⌈t/p_k⌉ * e_k)
     *         \_/   \_________________________/
     *          ^                 |
     *          |      Exec time of higher prio jobs stared during this interval
     *          |      (ie. interference time)
     *          |
     *          Execution time of J_i
     *
     *     If w_i(t) <= t at some [    t <= D_i], J_i meets its deadline [t_0 + D_i]
     *     If w_i(t) >  t for all [0 < t <= D_i], J_i is not guaranteed to complete by deadline
     *
     *
     * Response-time analysis:
     *
     *     Shortcut if we know when the critical instance will be
     *
     *     Notes taken from these lecture slides:
     *     https://support.dce.felk.cvut.cz/psr/prednasky/4/04-rts.pdf
     *
     *     If we know the critical instance, we can find task's worst-case
     *     response time by "simulating" the schedule from the critical instant
     *
     *     R_i < D_i
     *
     *     R_i = e_i + I_i
     *         -> I_i = interference from higher priority tasks
     *
     *     Notes taken from these lecture slides:
     *     https://www.it.uu.se/edu/course/homepage/realtid/ht10/schedule/Scheduling-periodic.pdf
     *
     *     Calculate the worst case response time R_i for each task with
     *     deadline D_i.
     *
     *     If R_i <= Di for all tasks, the task set is schedulable
     *
     *     w_i_n+1 = e_i + ∑(j∈hp(i), ⌈w_i_n/p_j⌉ * e_j)
     *         -> {hp(i)} is set of all tasks with higher prio than i
     *
     */

    analysis_results ret = {0, 0};

    // Utilization
    // Doing in separate loop in case the other one returns early
    for (unsigned int i = 0; i < task_set->num_tasks; i++) {
        Task task = task_set->tasks[i];
        ret.utilization += task.wcet / task.period;

    }

    for (unsigned int i = 0; i < task_set->num_tasks; i++) {

        Task *task = &(task_set->tasks[i]);

        double a_n, a_n1 = 0;

        for (;;) {

            a_n = a_n1;
            a_n1 = task->wcet;
            int passed_task = 0;

            for (unsigned int j = 0; j < task_set->num_tasks; j++) {

                Task *hp_task = &(task_set->tasks[j]);

                // Tasks aren't sorted by priority (yet?) so we need to find the
                // ones with higher priorities (shorter periods) that can preempt
                if (task == hp_task) {
                    passed_task++;
                }

                if ((hp_task->deadline < task->deadline) || (hp_task->deadline == task->deadline && !passed_task)) {
                    a_n1 += ceil(a_n / hp_task->period) * hp_task->wcet;
                }
            }

            // Fuzzy double equality check to account for fp precision errors
            if (fabs(a_n1 - a_n) < 0.0001) {
                // Current task is potentially schedulable
                break;
            }

            if (a_n1 > task->period) {
                // Still not schedulable
                return ret;
            }
        }

        // After the for(;;) loop is broken, a_n1 represents worst case response
        // time after critical instance
        if (a_n1 > task->deadline) {
            // Still not schedulable
            return ret;
        }
    }

    // If we reach here, task is schedulable
    ret.is_schedulable = 1;
    return ret;

}