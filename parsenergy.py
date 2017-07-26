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
import numpy as np
import pylab
import sys
import os
from matplotlib import rcParams

# rcParams['font.family'] = 'sans-serif'
# rcParams['font.sans-serif'] = ['Bitstream Vera Sans']
# rcParams['font.serif'] = ['Bitstream Vera Sans']
# rcParams["font.size"] = "6"
totenergy = []

node, time, energy = np.loadtxt(sys.argv[1], unpack=True)

node = map(int, node)
sort_node =np.sort(node)
sort_node = list(set(sort_node))

for p in sort_node:
    print p
    indices = [i for i, x in enumerate(node) if x == p]
    cur_energy = [energy[i] for i in indices]
    totenergy.append(cur_energy)

pylab.plot(time,energy)
pylab.grid(True)
fig = pylab.gcf()
fig.canvas.set_window_title(sys.argv[1])
pylab.show()