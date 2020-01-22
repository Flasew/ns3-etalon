from __future__ import print_function
import matplotlib.pyplot as plt
import math
import numpy as np
import pandas as pd
def plot_all(tstr, dt, save=False, zoomin=True, omega=1):
    yupper = [11000000000, 100000, 200000, 21000000000]
    ylab = ['Bandwidth (bps)', 'queue length (bytes)', 'packet drop count', 'RX received (bytes)']
    title = ['Bandwidth_change_dT={:s}ns'.format(dt),
             'Queue_length_dT={:s}ns'.format(dt),
             'Packet_drop_dT={:s}ns'.format(dt),
             'RX_dT={:s}ns'.format(dt)]
    s = [1, 0.0005, 1, 0.1]
    sz = [1, 0.1, 1, 0.1]
    sam = [1,0.3,1,0.5]

    fnames = [i + tstr for i in ["bwlog_", "qllog_", "drlog_", "rxlog_"]]
    logs = [pd.read_csv(f, header=None, names=['Time (ns)', ylab[i]]) for f, i in zip(fnames, range(len(fnames)))]
    for i in range(len(logs)):
        logs[i]['Time (ns)'] = pd.to_numeric(logs[i]['Time (ns)'], errors='coerce')
        logs[i][ylab[i]] = pd.to_numeric(logs[i][ylab[i]], errors='coerce')
    # logs = [pd.read_csv(f, header=None, names=['Time (ns)', ylab[i]], dtype={'Time (ns)': int, ylab[i]: int}) for f, i in zip(fnames, range(len(fnames)))]
    slogs = []
    for i in range(len(logs)):
        print(title[i])
        slogs.append(logs[i].sample(frac=sam[i], replace=False, random_state=1))

    axes = [log.plot.scatter(x="Time (ns)", y=ylab[i], s=s[i], title = title[i],
                                            xlim=(0,40000000000), ylim=(0, yupper[i]),
                                            figsize=(12,8)) for log, i in zip(slogs, range(len(slogs)))]

    # zoomed = None
    #     for m in range(4):
#         print(m)
#         axes.append()
    if save:
        for i in range(len(axes)):
            fig = axes[i].get_figure()
            fig.savefig('{:s}.png'.format(title[i]))
            plt.close(fig)


        if zoomin:
            #for log, i in zip(logs, title):
            log = logs[1]
            i = 1

            print("upper:", 100/math.log(int(dt)/5100))
            print(log[(log['Time (ns)']>int(1e9)) & (log['Time (ns)']<1e9+100/math.log(int(dt)/5100)*int(dt)*omega)].dtypes)
            print(log[(log['Time (ns)']>int(1e9)) & (log['Time (ns)']<1e9+100/math.log(int(dt)/5100)*int(dt)*omega)].shape)
            print(log[(log['Time (ns)']>int(1e9)) & (log['Time (ns)']<1e9+100/math.log(int(dt)/5100)*int(dt)*omega)].values)

            #zoomedi = log[(log['Time (ns)']>int(1e9)) & (log['Time (ns)']<1e9+100/math.log(int(dt)/5100)*int(dt)*omega)].plot(kind='scatter',
            #                        x="Time (ns)", y=ylab[i], s=sz[i], title = "zoomed_init_"+title[i],
            #                        #xlim=(1e9,1e9+100*math.log(int(dt)/5100)*int(dt)*omega),
            #                        ylim=(0, yupper[i]),
            #                        figsize=(12,8))

            zoomeds = log[(log['Time (ns)']>int(3e9)) & (log['Time (ns)']<3e9+100/math.log(int(dt)/5100)*int(dt)*omega)].plot(kind='scatter',
                                    x="Time (ns)", y=ylab[i], s=sz[i], title = "zoomed_steady_"+title[i],
                                    #xlim=(1e9,1e9+100*math.log(int(dt)/5100)*int(dt)*omega),
                                    ylim=(0, yupper[i]),
                                    figsize=(12,8)) # for log, i in zip(logs, range(len(logs)))]

            #for i in range(len(axes)):
                #zoomed[i].set_xlim(1e9,1e9+10*int(dt)*omega)
            #fig = zoomedi.get_figure()
            #fig.savefig('zoomed_init_{:s}.png'.format(title[i]))
            #plt.close(fig)
            fig = zoomeds.get_figure()
            fig.savefig('zoomed_steady_{:s}.png'.format(title[i]))
            plt.close(fig)

    else:
        return axes #, zoomed

with open('fct', 'r') as f:
    for line in f:
        tok = line.split('\t')
        print(tok[0], tok[1])
        plot_all(tok[0], tok[1], True, omega=3)


