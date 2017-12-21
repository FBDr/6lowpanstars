from __future__ import division
import numpy as np
import pylab
import sys
import os
import itertools
from locale import str
from matplotlib import rcParams
from scipy.constants.codata import value

try:
    from xml.etree import cElementTree as ElementTree
except ImportError:
    from xml.etree import ElementTree

flowxml = ElementTree.parse(sys.argv[1])
txNWU_coap = []
for flow in flowxml.findall("FlowStats/Flow"):
    # filteroutOLSR
    for tpl in flowxml.findall("Ipv6FlowClassifier/Flow"):
        if tpl.get('flowId') == flow.get('flowId'):
            break

    if tpl.get("destinationPort") == '9':
        print "C->P: Tx packets", int(flow.get('rxPackets')), "Src: ", tpl.get('sourceAddress'), "Dst: ", tpl.get('destinationAddress')
        if int(flow.get('timesForwarded')) != 0:
            print "---Times forwarded: ", flow.get('timesForwarded')
            print "---TxPackets", flow.get('rxPackets')
            print "---Txbytes", flow.get('rxBytes')
            print "TxNWUCoAP", int(((float(flow.get('timesForwarded')) / float(flow.get('rxPackets'))) + 1) * float(
                    flow.get('rxBytes')))
        else:
            print "TxNWUCoAP0",int(flow.get('rxBytes'))
    if tpl.get("sourcePort") == '9':
        print "P->C: Rx packets", int(flow.get('rxPackets')), "Src: ", tpl.get('sourceAddress'), "Dst: ", tpl.get('destinationAddress')

        if int(flow.get('timesForwarded')) != 0:
            print "---Times forwarded: ", flow.get('timesForwarded')
            print "---TxPackets", flow.get('rxPackets')
            print "---Txbytes", flow.get('rxBytes')
            print "TxNWUCoAP", int(((float(flow.get('timesForwarded')) / float(flow.get('rxPackets'))) + 1) * float(
                    flow.get('rxBytes')))
        else:
            print "Rx NWU", int(flow.get('rxBytes'))

            
            
for flow in flowxml.findall("FlowStats/Flow"):
    # filteroutOLSR
    for tpl in flowxml.findall("Ipv6FlowClassifier/Flow"):
        if tpl.get('flowId') == flow.get('flowId'):
            break

    if (tpl.get("destinationPort") == '9') or (tpl.get("sourcePort") == '9'):
        # lostpkts_coap.append(int(flow.get('lostPackets')))
        # txPackets_coap.append(int(flow.get('rxPackets')))
        if int(flow.get('timesForwarded')) != 0:
            txNWU_coap.append(
                int(((float(flow.get('timesForwarded')) / float(flow.get('rxPackets'))) + 1) * float(
                    flow.get('rxBytes'))))
        else:
            txNWU_coap.append(int(flow.get('rxBytes')))

print sum(txNWU_coap)