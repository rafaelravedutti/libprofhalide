import matplotlib.pyplot as plt

threads = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20]
breadth_first = [
    343.378,
    245.741,
    213.163,
    207.895,
    215.304,
    225.957,
    234.359,
    254.451,
    284.987,
    317.772,
    344.213,
    354.836,
    362.089,
    381.266,
    409.848,
    431.179,
    451.005,
    475.427,
    501.93,
    528.517
]

full_fusion = [
    179.696,
    140.283,
    132.941,
    132.952,
    141.5,
    149.914,
    166.795,
    181.648,
    199.548,
    221.794,
    235.523,
    248.198,
    264.096,
    276.665,
    281.792,
    307.39,
    320.004,
    343.248,
    363.406,
    381.228
]

tile_32x32 = [
    283.413,
    165.567,
    137.176,
    134.023,
    122.488,
    122.43,
    127.744,
    136.192,
    140.435,
    150.015,
    159.472,
    175.286,
    184.893,
    195.384,
    203.864,
    208.121,
    221.777,
    224.81,
    251.413,
    257.301
]

fig = plt.figure()
ax = plt.axes()
ax.plot(threads, [1.0 / t for t in breadth_first], label="breadth_first")
ax.plot(threads, [1.0 / t for t in full_fusion], label="full_fusion")
ax.plot(threads, [1.0 / t for t in tile_32x32], label="tile_32x32")
ax.set(xlabel='# threads', ylabel='inverse runtime (1/ms)', title="Scaling runtime for Cascade Lake")
ax.set_ylim(0, 0.01)
plt.legend()

fig.savefig("scaling_casclakesp2.pdf", bbox_inches = 'tight', pad_inches = 0)
