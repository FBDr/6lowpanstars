/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Universita' di Firenze
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

// Network topology
//
//       n0    n1
//       |     |
//       =================
//        WSN (802.15.4)
//
// - ICMPv6 echo request flows from n0 to n1 and back with ICMPv6 echo reply
// - DropTail queues 
// - Tracing of queues and packet receptions to file "wsn-ping6.tr"
//
// This example is based on the "ping6.cc" example.

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Ping6WsnExample");

class StackHelper
{
    public :

    /**
     * \brief Add an address to a IPv6 node.
     * \param n node
     * \param interface interface index
     * \param address IPv6 address to add
     */
    inline void AddAddress(Ptr<Node>& n, uint32_t interface, Ipv6Address address)
    {
        Ptr<Ipv6> ipv6 = n->GetObject<Ipv6> ();
        ipv6->AddAddress(interface, address);
    }

    /**
     * \brief Print the routing table.
     * \param n the node
     */
    inline void PrintRoutingTable(Ptr<Node>& n)
    {
        Ptr<Ipv6StaticRouting> routing = 0;
        Ipv6StaticRoutingHelper routingHelper;
        Ptr<Ipv6> ipv6 = n->GetObject<Ipv6> ();
        uint32_t nbRoutes = 0;
        Ipv6RoutingTableEntry route;

        routing = routingHelper.GetStaticRouting(ipv6);

        std::cout << "Routing table of " << n << " : " << std::endl;
        std::cout << "Destination\t\t\t\t" << "Gateway\t\t\t\t\t" << "Interface\t" << "Prefix to use" << std::endl;

        nbRoutes = routing->GetNRoutes();
        for (uint32_t i = 0; i < nbRoutes; i++) {
            route = routing->GetRoute(i);
            std::cout << route.GetDest() << "\t"
                    << route.GetGateway() << "\t"
                    << route.GetInterface() << "\t"
                    << route.GetPrefixToUse() << "\t"
                    << std::endl;
        }
    }};

int main(int argc, char **argv) {
    bool verbose = false;
    unsigned int rngfeed = 43221;
    unsigned int node_num = 10;
    unsigned int node_periph = 5;
    unsigned int node_head = 2;
    unsigned int con_per;
    unsigned int pro_per;

    CommandLine cmd;
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.AddValue("rng", "Set feed for random number generator", rngfeed);
    cmd.AddValue("nodes", "Number of nodes", node_num);
    cmd.AddValue("periph", "Number of peripheral nodes", node_periph);
    cmd.AddValue("head", "Number of head nodes", node_head);
    cmd.AddValue("ConPercent", "Consumer percentage", con_per);
    cmd.AddValue("ProPercent", "Producer percentage", pro_per);
    cmd.Parse(argc, argv);
    StackHelper stackHelper;

    if (verbose) {
        LogComponentEnable("Ping6WsnExample", LOG_LEVEL_INFO);
        LogComponentEnable("Ping6Application", LOG_LEVEL_ALL);

    }

    NS_LOG_INFO("Create nodes.");
    NodeContainer iot[2];
    NodeContainer br;
    iot[0].Create(9);
    iot[1].Create(9);

    br.Create(2);

    iot[0].Add(br.Get(0));
    iot[1].Add(br.Get(1));

    // Set seed for random numbers
    //Seeding RNG
    //time_t time_ran;
    //time(&time_ran);
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(rngfeed);

    // Install mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
            "MinX", DoubleValue(0.0),
            "MinY", DoubleValue(0.0),
            "DeltaX", DoubleValue(2),
            "DeltaY", DoubleValue(2),
            "GridWidth", UintegerValue(3),
            "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(iot[0]);

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
            "MinX", DoubleValue(20.0),
            "MinY", DoubleValue(0.0),
            "DeltaX", DoubleValue(2),
            "DeltaY", DoubleValue(2),
            "GridWidth", UintegerValue(3),
            "LayoutType", StringValue("RowFirst"));
    mobility.Install(iot[1]);

    Ptr<ListPositionAllocator> nodesPositionAlloc = CreateObject<ListPositionAllocator> ();
    nodesPositionAlloc->Add(Vector(2.0, -2.0, 0.0));
    nodesPositionAlloc->Add(Vector(22.0, -2.0, 0.0));
    mobility.SetPositionAllocator(nodesPositionAlloc);
    mobility.Install(br);

    NS_LOG_INFO("Create channels.");

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer d1 = csma.Install(br);

    LrWpanHelper lrWpanHelper;
    // Add and install the LrWpanNetDevice for each node
    // lrWpanHelper.EnableLogComponents();
    NetDeviceContainer devContainer = lrWpanHelper.Install(iot[0]);
    lrWpanHelper.AssociateToPan(devContainer, 10);
    NetDeviceContainer devContainer2 = lrWpanHelper.Install(iot[1]);
    lrWpanHelper.AssociateToPan(devContainer2, 11);


    /* Install IPv4/IPv6 stack */
    NS_LOG_INFO("Install Internet stack.");
    InternetStackHelper internetv6;
    internetv6.SetIpv4StackInstall(false);
    internetv6.InstallAll();

    // Install 6LowPan layer
    NS_LOG_INFO("Install 6LoWPAN.");
    SixLowPanHelper sixlowpan;
    NetDeviceContainer six1 = sixlowpan.Install(devContainer);
    NetDeviceContainer six2 = sixlowpan.Install(devContainer2);

    NS_LOG_INFO("Assign addresses.");
    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer i1 = ipv6.Assign(six1);
    i1.SetForwarding(9, true);
    i1.SetDefaultRouteInAllNodes(9);

    ipv6.SetBase(Ipv6Address("2001:2::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer i2 = ipv6.Assign(six2);
    i2.SetForwarding(9, true);
    i2.SetDefaultRouteInAllNodes(9);
    ipv6.SetBase(Ipv6Address("2001:3::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer i3 = ipv6.Assign(d1);
    i3.SetForwarding(0, true);
    i3.SetForwarding(1, true);
    i3.SetDefaultRouteInAllNodes(0);
    i3.SetDefaultRouteInAllNodes(1);


    Ptr<Node> routenode = iot[0].Get(0);
    stackHelper.PrintRoutingTable(routenode);

    NS_LOG_INFO("Create Applications.");

    /* Create a Ping6 application to send ICMPv6 echo request from node zero to
     * all-nodes (ff02::1).
     */
    uint32_t packetSize = 10;
    uint32_t maxPacketCount = 1000;
    Time interPacketInterval = Seconds(0.05);
    Ping6Helper ping6;

    ping6.SetLocal(i1.GetAddress(0, 1));
    ping6.SetRemote(i2.GetAddress(0, 1));
    // ping6.SetRemote (Ipv6Address::GetAllNodesMulticast ());

    ping6.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    ping6.SetAttribute("Interval", TimeValue(interPacketInterval));
    ping6.SetAttribute("PacketSize", UintegerValue(packetSize));
    ApplicationContainer apps = ping6.Install(iot[0].Get(0));
    apps.Start(Seconds(2.0));
    apps.Stop(Seconds(10.0));

    AsciiTraceHelper ascii;
    //lrWpanHelper.EnableAsciiAll(ascii.CreateFileStream("ping6wsn.tr"));
    lrWpanHelper.EnablePcapAll(std::string("ping6wsn"), true);

    NS_LOG_INFO("Run Simulation.");
    AnimationInterface anim("AWSNanimation.xml");
    anim.EnablePacketMetadata(true);
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");
}

