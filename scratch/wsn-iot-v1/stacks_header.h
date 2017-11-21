/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ndn_header.h
 * Author: floris
 *
 * Created on August 7, 2017, 11:07 AM
 */
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/basic-energy-source.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/netanim-module.h"
#include "src/network/model/node.h"
#include "ns3/brite-module.h"
#include "ns3/flow-monitor-module.h"
#include "helper/ndn-stack-helper.hpp"
#include "src/ndnSIM/helper/ndn-global-routing-helper.hpp"
#include "ns3/ndnSIM-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "src/network/helper/node-container.h"
#include "src/network/utils/output-stream-wrapper.h"
#include "src/core/model/log.h"
#include <string>
#include <vector>

#include "g_function_header.h"
#ifndef NDN_HEADER_H
#define NDN_HEADER_H
namespace ns3 {
    void NDN_stack(int &node_head, int &node_periph, NodeContainer iot[], NodeContainer & backhaul, NodeContainer &endnodes,
            BriteTopologyHelper &bth, int &simtime, int &report_time_cu,  int &con_leaf, int &con_inside, int &con_gtw,
            int &cache, double &freshness, bool &ipbackhaul, int &payloadsize, std::string zm_q, std::string zm_s,
            double &min_freq, double &max_freq);

    void sixlowpan_stack(int &node_periph, int &node_head, int &totnumcontents, BriteTopologyHelper &bth,
            NetDeviceContainer LrWpanDevice[], NetDeviceContainer SixLowpanDevice[], NetDeviceContainer CSMADevice[],
            Ipv6InterfaceContainer i_6lowpan[], Ipv6InterfaceContainer i_csma[],
            std::vector< std::vector<Ipv6Address> > &IPv6Bucket, std::vector< std::vector<Ipv6Address> > &AddrResBucket,
            NodeContainer &endnodes, NodeContainer &br, NodeContainer & backhaul);

    void sixlowpan_apps(int &node_periph, int &node_head, NodeContainer iot[], NodeContainer all,
            std::vector< std::vector<Ipv6Address> > &AddrResBucket, ApplicationContainer &apps,
            Ipv6InterfaceContainer i_6lowpan[], int &simtime, BriteTopologyHelper & briteth, int &payloadsize,
            std::string zm_q, std::string zm_s, int &con_leaf, int &con_inside, int &con_gtw,
            double &min_freq, double &max_freq, bool &useIPCache, double &freshness, 
            int &cache);

}
#endif /* NDN_HEADER_H */

