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
#

from __future__ import division
import numpy as np
import pylab
import sys
import os
import itertools
from matplotlib import rcParams


try:
    from xml.etree import cElementTree as ElementTree
except ImportError:
    from xml.etree import ElementTree

# Read files

node, time, hops, delay = np.loadtxt(sys.argv[1], unpack=True)
node, num_tx, num_rx, perc = np.loadtxt(sys.argv[2], unpack=True)
flowxml = ElementTree.parse(sys.argv[3])

lostpkts_coap = []
rxPackets_coap = []
txPackets_coap = []
rxBytes_coap = []
txBytes_coap = []
lostpkts_over = []
rxPackets_over = []
txPackets_over = []
rxBytes_over = []
txBytes_over = []


# Parse
N = len(node)
x = range(N)
tot_trans = sum(num_tx)
tot_received = sum(num_rx)
pylab.bar(x, (num_tx - num_rx) / num_tx * 100, color='g', align='center')

for flow in flowxml.findall("FlowStats/Flow"):
    # filteroutOLSR
    for tpl in flowxml.findall("Ipv6FlowClassifier/Flow"):
        if tpl.get('flowId') == flow.get('flowId'):
            break
    if tpl.get("destinationPort") == '49153' or tpl.get("destinationPort") == '49154' or tpl.get(
            "destinationPort") == '49155' or tpl.get("destinationPort") == '9':
        lostpkts_coap.append(int(flow.get('lostPackets')))
        txPackets_coap.append(int(flow.get('txPackets')))
        rxPackets_coap.append(int(flow.get('rxPackets')))
        rxBytes_coap.append(int(flow.get('rxBytes')))
        txBytes_coap.append(int(flow.get('txBytes')))
    else:
        lostpkts_over.append(int(flow.get('lostPackets')))
        txPackets_over.append(int(flow.get('txPackets')))
        rxPackets_over.append(int(flow.get('rxPackets')))
        rxBytes_over.append(int(flow.get('rxBytes')))
        txBytes_over.append(int(flow.get('txBytes')))

txPacketsTotal_coap = sum(txPackets_coap)
rxPacketsTotal_coap = sum(rxPackets_coap)
txPacketsTotal_over = sum(txPackets_over)
rxPacketsTotal_over = sum(rxPackets_over)
txBytesTotal_coap = sum(txBytes_coap)
rxBytesTotal_coap = sum(rxBytes_coap)
txBytesTotal_over = sum(txBytes_over)
rxBytesTotal_over = sum(rxBytes_over)
lostpktsTotal_coap = sum(lostpkts_coap)
lostpktsTotal_over = sum(lostpkts_over)


txBytesTotal_coap = sum(txBytes_coap)

# Plotting
pylab.figure(0)
pylab.subplot(221)
pylab.hist(delay, bins=300)
pylab.xlabel("Average Delay (ms)")
pylab.ylabel("Number of packets")
# pylab.axis([0, 0.2, 0, 10])
pylab.grid(True)

pylab.subplot(223)
pylab.hist(hops, bins=14)
pylab.xlabel("Average number of hops")
pylab.ylabel("Number of packets")
pylab.grid(True)

pylab.subplot(222)
pylab.xlabel("Node number")
pylab.ylabel("Lost packets in %")
pylab.grid(True)
pylab.axis([0, max(x), -2, 100])
pylab.text(0.05 * max(x), 90, 'Total tx/rx pkts: ' + str(int(tot_trans)) + "/" + str(int(tot_received)), style='italic',
           bbox={'facecolor': 'red', 'alpha': 0.5, 'pad': 10})
pylab.bar(x, (num_tx - num_rx) / num_tx * 100, color='g',align='center')
pylab.subplot(224)
pylab.plot(hops, 'go')
pylab.ylabel("Average number of hops")
pylab.xlabel("Packet sequence number")
pylab.grid(True)
fig = pylab.gcf()
fig.canvas.set_window_title(sys.argv[1])
pylab.savefig("metricsip.pdf")

pylab.figure(1)
pylab.subplot(231)
pylab.title("TxPackets")
pylab.bar([1, 2], [txPacketsTotal_coap, txPacketsTotal_over], color=['red', 'blue'])
pylab.subplot(232)
pylab.title("RxPackets")
pylab.bar([1, 2], [rxPacketsTotal_coap, rxPacketsTotal_over], color=['red', 'blue'])
pylab.subplot(233)
pylab.title("TxBytes")
pylab.bar([1, 2], [txBytesTotal_coap, txBytesTotal_over], color=['red', 'blue'])
pylab.subplot(234)
pylab.title("RxBytes")
pylab.bar([1, 2], [rxBytesTotal_coap, rxBytesTotal_over], color=['red', 'blue'])
pylab.subplot(235)
pylab.title("Lost packets")
pylab.bar([1, 2], [lostpktsTotal_coap, lostpktsTotal_over], color=['red', 'blue'])

pylab.show()
