import matplotlib.pyplot as plt

threads = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20]
breadth_first_numactl = [
    357.411,
    237.288,
    193.076,
    176.251,
    167.816,
    165.68,
    163.599,
    169.942,
    193.202,
    208.024,
    217.48,
    221.851,
    228.082,
    229.335,
    235.685,
    244.195,
    251.599,
    268.373,
    271.578,
    279.278
]

breadth_first_likwid_pin = [
    357.43,
    238.352,
    193.111,
    176.24,
    167.863,
    165.682,
    171.017,
    178.362,
    187.957,
    211.082,
    216.395,
    222.787,
    225.982,
    231.409,
    236.667,
    246.141,
    256.771,
    264.276,
    267.462,
    277.611
]

full_fusion_numactl = [
    190.851,
    129.729,
    107.673,
    97.106,
    89.705,
    89.761,
    90.79,
    99.253,
    106.61,
    110.842,
    111.909,
    114.032,
    115.102,
    117.254,
    120.433,
    124.724,
    129.983,
    129.994,
    135.326,
    135.391
]

full_fusion_likwid_pin = [
    189.79,
    128.707,
    108.71,
    99.194,
    92.909,
    91.838,
    92.891,
    93.936,
    107.675,
    110.835,
    112.949,
    115.116,
    116.229,
    118.32,
    122.571,
    124.663,
    132.039,
    133.17,
    128.974,
    135.431
]

tile_32x32_numactl = [
    308.791,
    177.204,
    121.296,
    99.145,
    81.235,
    69.663,
    62.274,
    54.893,
    50.677,
    46.47,
    43.312,
    41.188,
    40.135,
    38.033,
    36.985,
    35.926,
    35.936,
    34.895,
    34.881,
    34.879
]

tile_32x32_likwid_pin = [
    311.961,
    179.208,
    123.385,
    100.203,
    82.301,
    70.72,
    62.292,
    55.979,
    50.681,
    47.514,
    44.354,
    42.244,
    40.138,
    38.05,
    38.04,
    35.936,
    35.926,
    35.936,
    34.88,
    34.889
]

fig = plt.figure()
ax = plt.axes()
ax.grid(linestyle='-', linewidth=0.5)
ax.plot(threads, [1.0 / t for t in breadth_first_numactl],      color='g',  linestyle='solid',  marker='*', label="breadth_first numactl")
ax.plot(threads, [1.0 / t for t in breadth_first_likwid_pin],   color='g',  linestyle='dashed', marker='.', label="breadth_first likwid-pin")
ax.plot(threads, [1.0 / t for t in full_fusion_numactl],        color='b',  linestyle='solid',  marker='*', label="full_fusion numactl")
ax.plot(threads, [1.0 / t for t in full_fusion_likwid_pin],     color='b',  linestyle='dashed', marker='.', label="full_fusion likwid-pin")
ax.plot(threads, [1.0 / t for t in tile_32x32_numactl],         color='r',  linestyle='solid',  marker='*', label="tile_32x32 numactl")
ax.plot(threads, [1.0 / t for t in tile_32x32_likwid_pin],      color='r',  linestyle='dashed', marker='.', label="tile_32x32 likwid-pin")
ax.set(xlabel="# threads", ylabel="inverse runtime (1/ms)")
#ax.set(xlabel='# threads', ylabel='inverse runtime (1/ms)', title="Scaling runtime for Cascade Lake")
ax.set_ylim(0, 0.05)
plt.xticks(threads, threads)
plt.legend()

fig.savefig("scaling_casclakesp2.pdf", bbox_inches = 'tight', pad_inches = 0)
