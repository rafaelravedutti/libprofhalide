import matplotlib.pyplot as plt

threads = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]
breadth_first = [
    249.141,
    176.897,
    146.424,
    166.189,
    170.834,
    174.088,
    178.523,
    183.628,
    187.795,
    191.104,
    196.524,
    210.211,
    214.67,
    223.04,
    233.182,
    254.105
]

full_fusion = [
    128.628,
    92.623,
    79.057,
    96.967,
    101.336,
    106.588,
    113.643,
    118.309,
    122.63,
    129.904,
    136.237,
    146.747,
    153.055,
    161.757,
    167.87,
    178.215
]

tile_32x32 = [
    162.732,
    106.363,
    89.124,
    92.794,
    92.718,
    96,
    98.191,
    104.444,
    108.742,
    113.955,
    122.496,
    133.032,
    138.236,
    143.557,
    152.165,
    163.433
]

fig = plt.figure()
ax = plt.axes()
ax.plot(threads, [1.0 / t for t in breadth_first], label="breadth_first")
ax.plot(threads, [1.0 / t for t in full_fusion], label="full_fusion")
ax.plot(threads, [1.0 / t for t in tile_32x32], label="tile_32x32")
ax.set(xlabel='# threads', ylabel='inverse runtime (1/ms)', title="Scaling runtime for Rome")
ax.set_ylim(0, 0.02)
plt.legend()

fig.savefig("scaling_rome.pdf", bbox_inches = 'tight', pad_inches = 0)
