import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import matplotlib
import statistics
import struct

with open("/home/mergen/Documents/data_21_10_03/1980l_m", mode='rb') as file:
    datal = file.read()
with open("/home/mergen/Documents/data_21_10_03/1980s_m", mode='rb') as file:
    datas = file.read()

fig, ax = plt.subplots(figsize=(14, 9.1))
im = None
fmt = 'B' * 181
shift = 0
fig.patch.set_facecolor('white')
for i in range(1464):
    l = [];
    for j in range(161):
        shift = shift + 181
        l.append(struct.unpack(fmt, datas[shift:shift + 181]) + struct.unpack(fmt, datal[shift:shift + 181]))
    if not im:
        im = ax.imshow(l, interpolation="bilinear")
    else:
        im.set_data(l)
    fig.canvas.draw()
    plt.savefig('{}/img{:04}'.format("/home/mergen/Documents/data_21_10_03/pic", i), bbox_inches='tight')
fig.clear()
plt.close(fig)

