import matplotlib.pylab as plt
import numpy as np

edf = "out/results_edf.txt"
rm = "out/results_rm.txt"
dm = "out/results_dm.txt"

bins = np.arange(0, 1.1, .1)

for fi in [edf, rm, dm]:

    # Create 2D structured numpy array from output file
    results = np.genfromtxt(fi,
                            delimiter=" ",
                            dtype=[("sched", int),
                                   ("util", float)])

    # Create array where each value is the bin at which the original list's
    # entry fits into. Values are floored to fit into bin (ie. 0.19 goes in the
    # 0.1 bin)
    indices = np.digitize(results["util"], bins)

    # Get the percent of schedulable task sets at each bin
    percents = []
    for bin in range(1, bins.size):
        # Get all results that are within current bin
        filtered = results[np.where(indices == bin)]

        # Count them (schedulable == 1, so sum works perfectly)
        sched_count = (filtered["sched"]).sum()

        # Add to list
        percents.append(sched_count / filtered.size)

    # Plot
    x_axis = np.arange(0.05, 1.05, .1)
    plt.plot(x_axis, percents, label=fi.split('_')[1].split('.')[0])
    plt.axis()

# Show legend and plot
plt.legend(loc="lower left")
plt.show()