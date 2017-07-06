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
import numpy
import pylab
import sys
import os

time, node, appid, seq, delay, delayu, rtx, hops = numpy.loadtxt(sys.argv[1], skiprows=1,
                                                                 usecols=(0, 1, 2, 3, 5, 6, 7, 8),
                                                                 unpack=True)

delay_filtered = []
hops_filtered =[]
idx = 0
zero_entries = 0
for delay_i, hop_i in zip(delay, hops):
    idx += 1
    if delay_i == 0:
        zero_entries += 1
        continue
    if idx % 2:
        delay_filtered.append(delay_i)
        hops_filtered.append(hop_i)

print "Found:", zero_entries, "zero delay entries. From a total of", idx, "received packets. So", zero_entries * 100 / idx, "% of the total entries is 0."

pylab.subplot(221)
pylab.hist(delay_filtered, bins=500)
pylab.xlabel("Delay(s)")
pylab.ylabel("Number of packets")
# pylab.axis([0, 0.2, 0, 10])
pylab.grid(True)
pylab.subplot(222)
pylab.plot(delay_filtered, 'go')
pylab.xlabel("Packet sequence number")
pylab.ylabel("Delay (s)")
pylab.grid(True)

pylab.subplot(223)

pylab.hist(hops_filtered, bins=14)
pylab.xlabel("Hops")
pylab.ylabel("Number of packets")
pylab.grid(True)
pylab.subplot(224)
pylab.plot(hops_filtered, 'go')
pylab.ylabel("Hops")
pylab.xlabel("Packet sequence number")
pylab.grid(True)
pylab.show()
