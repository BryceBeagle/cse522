import matplotlib.pylab as plt
import numpy as np

edf = "out/results_edf.txt"
rm = "out/results_rm.txt"
dm = "out/results_dm.txt"

bins = np.arange(0, 1.1, .1)

for fi in [edf, rm, dm]:

    results = np.genfromtxt(fi,
                            delimiter=" ",
                            dtype=[("sched", int),
                                   ("util", float)])

    indices = np.digitize(results["util"], bins)

    percents = []
    for bin in range(1, bins.size):
        filtered = results[np.where(indices == bin)]
        sched_count = (filtered["sched"]).sum()
        percents.append(sched_count / filtered.size)

    x_axis = np.arange(0.05, 1.05, .1)

    plt.plot(x_axis, percents, label=fi.split('_')[1].split('.')[0])
    plt.axis()

plt.legend(loc="lower left")
plt.show()