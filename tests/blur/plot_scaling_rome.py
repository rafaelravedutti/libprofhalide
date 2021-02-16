import matplotlib.pyplot as plt

threads = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]
breadth_first = [
    248.912,
    176.867,
    146.424,
    165.557,
    169.773,
    173.016,
    177.472,
    183.628,
    187.573,
    191.104,
    196.493,
    210.173,
    214.644,
    222.96,
    233.182,
    253.107
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
    145.487,
    94.794,
    82.267,
    82.271,
    89.445,
    86.702,
    93.959,
    100.209,
    106.587,
    110.719,
    113.092,
    133.032,
    130.915,
    143.557,
    144.657,
    161.342
]

fig = plt.figure()
ax = plt.axes()
ax.plot(threads, [1.0 / t for t in breadth_first], label="breadth_first")
ax.plot(threads, [1.0 / t for t in full_fusion], label="full_fusion")
ax.plot(threads, [1.0 / t for t in tile_32x32], label="tile_32x32")
ax.set(xlabel='# threads', ylabel='inverse runtime (1/ms)', title="Scaling runtime for Rome")
ax.set_ylim(0, 0.02)
plt.legend()

fig.savefig("scaling.pdf", bbox_inches = 'tight', pad_inches = 0)
