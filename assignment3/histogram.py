import numpy as np
import matplotlib.pyplot as plot

output1 = "1.out"
output2 = "2.out"
output3 = "3.out"

res = np.genfromtxt(output1, delimiter="\n")

plot.title(f"Interrupt Latency Without Background Computing")
plot.xlabel("Cycles")
plot.ylabel("Percent of results")
plot.hist(res, normed=True, bins=15)
plot.show()

res = np.genfromtxt(output1, delimiter="\n")

plot.title(f"Interrupt Latency With Background Computing")
plot.xlabel("Cycles")
plot.ylabel("Percent of results")
plot.hist(res, normed=True, bins=15)
plot.show()

res = np.genfromtxt(output1, delimiter="\n")

plot.title(f"Context Switch Overhead")
plot.xlabel("Cycles")
plot.ylabel("Percent of results")
plot.hist(res, normed=True, bins=15)
plot.show()

