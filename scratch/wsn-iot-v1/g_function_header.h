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
#include <set>

#ifndef G_HEADER_H
#define G_HEADER_H
namespace ns3 {
    void ReduceRouteFreq(NodeContainer routers);
    Ptr<Node> SelectRandomLeafNodeConsumer(BriteTopologyHelper & briteth, Ptr<UniformRandomVariable> Rnode);
    Ptr<Node> SelectRandomLeafNode(BriteTopologyHelper & briteth, Ptr<UniformRandomVariable> Rnode);

    Ptr<Node> SelectRandomNodeFromContainer(NodeContainer container, Ptr<UniformRandomVariable> Rnode);

    void shuffle_array(std::vector<int>& arrayf, Ptr<UniformRandomVariable> shuffles);
    void shuffle_array_ip(std::vector<Ipv6Address>& arrayf, Ptr<UniformRandomVariable> shuffles, int64_t stream, int size);

    std::vector< std::vector<Ipv6Address> > CreateAddrResBucket(std::vector< std::vector<Ipv6Address> > &arrayf, int numContentsPerDomain);

    template<typename _RandomAccessIterator>
    inline void
    random_shuffle_ns3(_RandomAccessIterator __first, _RandomAccessIterator __last, Ptr<UniformRandomVariable> shuffles);
}
#endif /* G_HEADER_H */

