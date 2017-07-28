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
import pylab
import sys
import os

try:
    from xml.etree import cElementTree as ElementTree
except ImportError:
    from xml.etree import ElementTree

et = ElementTree.parse(sys.argv[1])
bitrates = []
losses = []
delays = []
hops = []
totalpackets = 0
flownum = 0
for flow in et.findall("FlowStats/Flow"):
    # filteroutOLSR
    for tpl in et.findall("Ipv6FlowClassifier/Flow"):
        if tpl.get('flowId') == flow.get('flowId'):
            break
    if tpl.get("destinationPort") == '698' or tpl.get("destinationPort") == '521':
        continue
    losses.append(int(flow.get('lostPackets')))
    rxPackets = int(flow.get('rxPackets'))
    if rxPackets == 0:
        bitrates.append(0)
    else:
        # t0 = long(flow.get('timeFirstRxPacket')[:-4])
        # t1 = long(flow.get('timeLastRxPacket')[:-4])
        # duration=(t1 -t0)*1e-9
        # bitrates.append(8 * long(flow.get('rxBytes'))/duration *1e-3)
        # if long(flow.get('timeFirstTxPacket')[:-4]) >= 1e10:
        delays.append(float(flow.get('delaySum')[: -4]) * 1e-9 / rxPackets)
        print "FlowID:", flow.get('flowId')  # pylab.subplot(311)
        #print "timeFirstTxPacket:", long(flow.get('timeFirstTxPacket')[:-4]) * 1e-9
        #print "timeLastTxPacket:", long(flow.get('timeLastTxPacket')[:-4]) * 1e-9
        #print "timeFirstRxPacket:", long(flow.get('timeFirstRxPacket')[:-4]) * 1e-9
        #print "TimeLastRxPacket:", long(flow.get('timeLastRxPacket')[:-4]) * 1e-9
        hop_cur = float(flow.get('timesForwarded'))/float(rxPackets) +1
        print "HOPS:", hop_cur
        hops.append(hop_cur)
        print "Delay:", float(flow.get('delaySum')[: -4]) * 1e-9 / rxPackets
        print "Flow: received packets:", rxPackets
        print "Raw hops", flow.get('timesForwarded')r
        totalpackets = totalpackets + rxPackets
        flownum+=1

print "Total received packets: ", totalpackets
print "Total flows: ", flownum
# pylab.hist(bitrates,bins=40)
# pylab.xlabel("Flowbitrate(bit/s)")
# pylab.ylabel("Numberofflows")
# pylab.subplot(312)
# pylab.hist(losses,bins=40)
# pylab.xlabel("Numberoflostpackets")
# pylab.ylabel("Numberofflows")

# pylab.subplots_adjust(hspace=0.4)
# pylab.plot(delays,'bo', label='sampled')



pylab.subplot(221)
pylab.hist(delays, bins=50)
pylab.xlabel("Average Delay per flow (s)")
pylab.ylabel("Number of packets per flow")
# pylab.axis([0, 0.2, 0, 10])
pylab.grid(True)

#pylab.subplot(222)
#pylab.plot(time_moving, moving_aves)
#pylab.xlabel("Simulation time (s)")
#pylab.ylabel("Delay (s)")
#pylab.grid(True)

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


pylab.savefig("results.pdf")
