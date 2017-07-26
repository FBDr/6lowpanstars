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


# rcParams['font.family'] = 'sans-serif'
# rcParams['font.sans-serif'] = ['Bitstream Vera Sans']
# rcParams['font.serif'] = ['Bitstream Vera Sans']
# rcParams["font.size"] = "6"


idx = 0

node, time, energy = np.loadtxt(sys.argv[1], unpack=True)

node = map(int, node)
sort_node = np.sort(node)
sort_node = list(set(sort_node))
totenergy = [[] for x in xrange(max(sort_node) - min(sort_node) + 1)]

for p in sort_node:
    print p
    indices = [i for i, x in enumerate(node) if x == p]
    cur_energy = [energy[i] for i in indices]
    totenergy[idx] = cur_energy
    idx += 1

tot_trans = list(map(list, itertools.izip_longest(*totenergy)))

res = map(mean, zip(*tot_trans))

pylab.plot(totenergy[1])
pylab.grid(True)
fig = pylab.gcf()
fig.canvas.set_window_title(sys.argv[1])

pylab.show()
