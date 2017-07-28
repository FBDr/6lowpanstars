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
#

from __future__ import division
import numpy as np
import pylab
import sys
import os
import itertools
from matplotlib import rcParams

node, time, hops, delay = np.loadtxt(sys.argv[1], unpack=True)

pylab.subplot(221)
pylab.hist(delay, bins=300)
pylab.xlabel("Average Delay per flow (ms)")
pylab.ylabel("Number of packets per flow")
# pylab.axis([0, 0.2, 0, 10])
pylab.grid(True)


pylab.subplot(223)
pylab.hist(hops, bins=14)
pylab.xlabel("Average number of hops")
pylab.ylabel("Number of packets")
pylab.grid(True)

pylab.subplot(224)
pylab.plot(hops, 'go')
pylab.ylabel("Average number of hops")
pylab.xlabel("Packet sequence number")
pylab.grid(True)
fig = pylab.gcf()
fig.canvas.set_window_title(sys.argv[1])
pylab.show()