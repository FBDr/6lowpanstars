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

et=ElementTree.parse(sys.argv[1])
bitrates=[]
losses=[]
delays=[]
for flow in et.findall("FlowStats/Flow"):
	#filteroutOLSR
	for tpl in et.findall("Ipv6FlowClassifier/Flow"):
		if tpl.get('flowId') == flow.get('flowId'):
			break
	if tpl.get("destinationPort")=='698':
		continue
	losses.append(int(flow.get('lostPackets')))
	rxPackets=int(flow.get('rxPackets'))
	if rxPackets==0:
		bitrates.append(0)
	else:
		t0=long(flow.get('timeFirstRxPacket')[:-4])
		t1=long(flow.get('timeLastRxPacket')[:-4])
		#duration=(t1 -t0)*1e-9
		#bitrates.append(8 * long(flow.get('rxBytes'))/duration *1e-3)
		delays.append(float(flow.get('delaySum')[: -4])*1e-9/rxPackets)
#pylab.subplot(311)
#pylab.hist(bitrates,bins=40)
#pylab.xlabel("Flowbitrate(bit/s)")
#pylab.ylabel("Numberofflows")
#pylab.subplot(312)
#pylab.hist(losses,bins=40)
#pylab.xlabel("Numberoflostpackets")
#pylab.ylabel("Numberofflows")
#pylab.subplot(313)
pylab.hist(delays,bins=100)
pylab.xlabel("Delay(s)")
pylab.ylabel("Number of flows")
#pylab.subplots_adjust(hspace=0.4)
#pylab.plot(delays,'bo', label='sampled')
pylab.savefig("results.pdf")