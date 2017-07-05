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

time, node, appid, seq, delay, delayu, rtx, hop = numpy.loadtxt('app-delays-trace.txt', skiprows=1,
                                                                usecols=(0, 1, 2, 3, 5, 6, 7, 8),
                                                                unpack=True)

delay_filtered = []
idx = 0
for delay_i in delay:
    idx +=1
    if delay_i != 0 and idx % 2:
        delay_filtered.append(delay_i)
        print delay_i

pylab.subplot(211)
pylab.hist(delay_filtered, bins=500)
pylab.xlabel("Delay(s)")
pylab.ylabel("Number of packets")
# pylab.axis([0, 0.2, 0, 10])
pylab.grid(True)
pylab.subplot(212)
pylab.plot(delay_filtered, 'go')
pylab.xlabel("Packet number")
pylab.ylabel("Delay (s)")
pylab.show()
