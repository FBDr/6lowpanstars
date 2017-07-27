#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  parse.py
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


def printmatrix(matrix):
    s = [[str(e) for e in row] for row in matrix]
    lens = [max(map(len, col)) for col in zip(*s)]
    fmt = '\t'.join('{{:{}}}'.format(x) for x in lens)
    table = [fmt.format(*row) for row in s]
    print '\n'.join(table)


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


for p in sort_node:
    indices = [i for i, x in enumerate(node) if x == p]
    cur_energy = [energy[i] for i in indices]
    totenergy[idx] = cur_energy
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

ax2.bar(sort_node, used_energy_v)
ax2.set_xlabel("Node number")
ax2.set_ylabel("Energy usage (J)")

pylab.show()
# Onjuist
# tot_trans = list(map(list, itertools.izip_longest(*totenergy)))
# res = map(mean, zip(*tot_trans))
