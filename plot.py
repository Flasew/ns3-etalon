import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
def plot_all(tstr, dt, save=False):
    yupper = [11000000000, 100000, 10000, 11000000000]
    ylab = ['Bandwidth (bps)', 'queue length (bytes)', 'packet drop count', 'RX received (bytes)']
    title = ['Bandwidth_change_dT={:s}ns'.format(dt),
             'Queue_length_dT={:s}ns'.format(dt),
             'Packet_drop_dT={:s}ns'.format(dt),
             'RX_dT={:s}ns'.format(dt)]
    s = [1, 0.0005, 1, 0.1]
    sam = [1,0.3,1,0.5]

    fnames = [i + tstr for i in ["bwlog_", "qllog_", "drlog_", "rxlog_"]]
    logs = [pd.read_csv(f, header=None, names=['Time (ns)', ylab[i]]) for f, i in zip(fnames, range(len(fnames)))]
    for i in range(len(logs)):
        logs[i]['Time (ns)'] = pd.to_numeric(logs[i]['Time (ns)'], errors='coerce')
        logs[i][ylab[i]] = pd.to_numeric(logs[i][ylab[i]], errors='coerce')
    # logs = [pd.read_csv(f, header=None, names=['Time (ns)', ylab[i]], dtype={'Time (ns)': int, ylab[i]: int}) for f, i in zip(fnames, range(len(fnames)))]
    for i in range(len(logs)):
        print(title[i])
        logs[i] = logs[i].sample(frac=sam[i], replace=False, random_state=1)

    axes = [log.plot.scatter(x="Time (ns)", y=ylab[i], s=s[i], title = title[i],
                                            xlim=(0,24000000000), ylim=(0, yupper[i]),
                                            figsize=(12,8)) for log, i in zip(logs, range(len(logs)))]
#     for m in range(4):
#         print(m)
#         axes.append()
    if save:
        for i in range(len(axes)):
            fig = axes[i].get_figure()
            fig.savefig('{:s}.png'.format(title[i]))
            plt.close(fig)

    else:
        return axes

with open('fct', 'r') as f:
    for line in f:
        tok = line.split('\t')
        print(tok[0], tok[1])
        plot_all(tok[0], tok[1], True)
