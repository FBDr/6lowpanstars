#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  old_parse.py
#
#  Copyright 2017 Floris <floris@ndn-icarus-simulator>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
from __future__ import division
import numpy as np
import pylab
import sys
import os
import itertools
from matplotlib import rcParams


def mean(a):
    a = [x for x in a if x is not None]
    return sum(a) / len(a)


def energyConsumption(a):
    maxe = max(a)
    mine = min(a)
    return maxe - mine


# rcParams['font.family'] = 'sans-serif'
# rcParams['font.sans-serif'] = ['Bitstream Vera Sans']
# rcParams['font.serif'] = ['Bitstream Vera Sans']
# rcParams["font.size"] = "6"


idx = 0
node, time, energy = np.loadtxt(sys.argv[1], unpack=True)
print "File loaded, now busy parsing..."
node = map(int, node)
sort_node = np.sort(node)
sort_node = list(set(sort_node))
totenergy = [[] for x in xrange(max(sort_node) - min(sort_node) + 1)]
tottime = [[] for x in xrange(max(sort_node) - min(sort_node) + 1)]

for p in sort_node:
    indices = [i for i, x in enumerate(node) if x == p]
    cur_energy = [energy[i] for i in indices]
    cur_time = [time[i] for i in indices]
    totenergy[idx] = cur_energy
    tottime[idx] =cur_time
    idx += 1

used_energy_v = map(energyConsumption, totenergy)
std_used_energy = np.std(used_energy_v)
mean_used_energy = np.mean(used_energy_v)
print "Mean", mean_used_energy, "J"
print "Standard deviation: ", std_used_energy, "J"


fig = pylab.figure()
ax = fig.add_subplot(2, 1, 1)
ax2 = fig.add_subplot(2, 1, 2)

#pylab.bar(sort_node, used_energy_v)
ax.errorbar(1, mean_used_energy, yerr=std_used_energy, fmt='o')
ax.grid(True)
ax.set_xlabel("Node number")
ax.set_ylabel("Energy usage (J)")
ax.get_yaxis().get_major_formatter().set_useOffset(False)
ax.text(1.02, mean_used_energy, 'Average used energy: ' + str(mean_used_energy) + ' J ' + 'Std. deviation: ' + str(std_used_energy) + ' J', style='italic',
        bbox={'facecolor':'red', 'alpha':0.5, 'pad':10})


for cu_e, cu_t in zip(totenergy, tottime):
    ax2.plot(cu_t, cu_e, "go-")

ax2.set_xlabel("Simulation time (s)")
ax2.set_ylabel("Battery capacity (J)")
pylab.savefig("energy.pdf")
pylab.show()
