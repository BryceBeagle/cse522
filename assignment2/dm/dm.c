#include <math.h>
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
     * We call the ratio of the execution time e_k of a task T_k to the minimum
     * of its relative deadline D_k and period p_k the density δ_k of the task.
     *
     *     δ_k = e_k / min(D_k, p_k)
     *
     * The sum of the densities of all tasks in a system is the density of the
     * system and is denoted by Δ
     *
     * When D_i < p_i for some task T_i, Δ > U. If the density of a system is
     * larger than 1, the system may not be feasible.
     *
     * A system T of independent, preemptable tasks can be feasibly scheduled on
     * one processor if its density is equal to or less than 1.
     *
     * While the RM algorithm is not optimal for tasks with arbitrary periods,
     * it is optimal in the special case when the periodic tasks in the system
     * are simply periodic and the deadlines of the tasks are no less than
     * their respective periods.
     *
     * δ_k = e_k / min(D_k, p_k)           <--- only works if d_i = p_i    TODO: Is this correct?
     * Δ = sum(δ)
     * if Δ < 1:
     *     schedulable
     *
     *  U_max = N * ( 2^(1/N) -1 )
     *  U_i = t_i / p_i   -> ratio of burst time to period
     *
     *  sum(U_i) < U_max  -> will never miss period if RMA is applied
     *
     */


    int n = task_set->num_tasks;

    analysis_results ret = {0, 0};

    // Sufficient condition. Utilization test
    // If true, task is schedulable. If not we need to do time-demand analysis
    for (int i = 0; i < n; i++) {
        Task task = task_set->tasks[i];
        ret.utilization += task.wcet / fmin(task.period, task.deadline);
    }

    ret.is_schedulable = ret.utilization < n * (pow(2, 1. / n) - 1);

    if (ret.is_schedulable) {
        // Done
        return ret;
    }

    /* Time-demand analysis
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

    for (unsigned int i = 0; i < task_set->num_tasks; i++) {

        Task task = task_set->tasks[i];

        double w_n, w_n1 = task.wcet;

        for (;;) {

            w_n = w_n1;
            w_n1 = task.wcet;

            for (unsigned int k = 0; k < task_set->num_tasks; k++) {

                Task hp_task = task_set->tasks[k];

                // Tasks aren't sorted by priority (yet?) so we need to find the
                // ones with higher priorities (shorter periods) that can preempt
                if (hp_task.deadline >= task.deadline) {
                    continue;
                }

                w_n1 += ceil(w_n / hp_task.period) * hp_task.wcet;
            }

            if (w_n1 == w_n) {
                // Current task is potentially schedulable
                break;
            }

            if (w_n1 > task.period) {
                // Still not schedulable
                return ret;
            }
        }

        // After the for(;;) loop is broken, w_n1 represents worst case response
        // time after critical instance
        if (w_n1 > task.deadline) {
            // Still not schedulable
            return ret;
        }
    }

    // If we reach here, task is schedulable
    ret.is_schedulable = 1;
    return ret;

}