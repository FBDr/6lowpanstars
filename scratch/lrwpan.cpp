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

 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/internet-apps-module.h"
using namespace ns3;

int main(int argc, char **argv) {

    std::string SplitHorizon("PoisonReverse");
    int noden = 10;
    CommandLine cmd;
    cmd.AddValue("splitHorizonStrategy", "Split Horizon strategy to use (NoSplitHorizon, SplitHorizon, PoisonReverse)", SplitHorizon);
    cmd.AddValue("numNodes", "Number of nodes", noden);
    cmd.Parse(argc, argv);


    if (true || SplitHorizon == "NoSplitHorizon") {
        Config::SetDefault("ns3::RipNg::SplitHorizon", EnumValue(RipNg::NO_SPLIT_HORIZON));
    } else if (SplitHorizon == "SplitHorizon") {
        Config::SetDefault("ns3::RipNg::SplitHorizon", EnumValue(RipNg::SPLIT_HORIZON));
    } else {
        Config::SetDefault("ns3::RipNg::SplitHorizon", EnumValue(RipNg::POISON_REVERSE));
    }

    //Important line preventing erroneous redirects.
    Config::SetDefault("ns3::Ipv6L3Protocol::SendIcmpv6Redirect", BooleanValue(false));

    NodeContainer nodes;
    nodes.Create (noden);

    // Set seed for random numbers
    SeedManager::SetSeed(133);

    // Install mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
            "MinX", DoubleValue(0.0),
            "MinY", DoubleValue(0.0),
            "DeltaX", DoubleValue(100),
            "DeltaY", DoubleValue(100),
            "GridWidth", UintegerValue(4),
            "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);


    LrWpanHelper lrWpanHelper;
    // Add and install the LrWpanNetDevice for each node
    // lrWpanHelper.EnableLogComponents();
    NetDeviceContainer devContainer = lrWpanHelper.Install(nodes);
    lrWpanHelper.AssociateToPan(devContainer, 10);

    std::cout << "Created " << devContainer.GetN() << " devices" << std::endl;
    std::cout << "There are " << nodes.GetN() << " nodes" << std::endl;

    /* Routing*/
    RipNgHelper ripNgRouting;


    Ipv6ListRoutingHelper listRH;
    listRH.Add(ripNgRouting, 0);
    Ipv6StaticRoutingHelper staticRh;
    listRH.Add(staticRh, 5);

    /* Install IPv4/IPv6 stack */

    InternetStackHelper internetv6;
    internetv6.SetIpv4StackInstall(false);
    internetv6.SetRoutingHelper(listRH);
    internetv6.Install(nodes);

    // Install 6LowPan layer

    SixLowPanHelper sixlowpan;
    NetDeviceContainer six1 = sixlowpan.Install(devContainer);


    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:1337::"), Ipv6Prefix(64));

    Ipv6InterfaceContainer* ipic = new Ipv6InterfaceContainer[noden];
    for (int i = 0; i < noden; i++) {
        ipic[i] = ipv6.Assign(six1.Get(i));
        ipic[i].SetForwarding(0, true);
        ipic[i].SetDefaultRouteInAllNodes(0);
        std::cout << ipic[i].GetAddress(0, 1) << std::endl;
        ipv6.NewNetwork();
        
    }



    //i.SetDefaultRouteInAllNodes (0);


    if (0) {
        RipNgHelper routingHelper;

        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);
        for (uint32_t jdx = 0; jdx < 121; (jdx = jdx + 20)) {
            for (uint32_t idx = 0; idx < six1.GetN(); idx++) {
                routingHelper.PrintRoutingTableAt(Seconds(jdx), nodes.Get(idx), routingStream);
            }
        }

        std::cout << std::endl << std::endl;
    }




    /* Create a Ping6 application to send ICMPv6 echo request from node zero to
     * all-nodes (ff02::1).
     */
    uint32_t packetSize = 50;
    uint32_t maxPacketCount = 100;
    Time interPacketInterval = Seconds(3.0);
    Ping6Helper ping6;

    ping6.SetLocal(ipic[0].GetAddress(0, 1));
    ping6.SetRemote(ipic[3].GetAddress(0, 1));
    // ping6.SetRemote (Ipv6Address::GetAllNodesMulticast ());

    ping6.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    ping6.SetAttribute("Interval", TimeValue(interPacketInterval));
    ping6.SetAttribute("PacketSize", UintegerValue(packetSize));
    ApplicationContainer apps = ping6.Install(nodes.Get(0));
    apps.Start(Seconds(50.0));
    apps.Stop(Seconds(120.0));

    AsciiTraceHelper ascii;
    lrWpanHelper.EnableAsciiAll(ascii.CreateFileStream("ping6wsn.tr"));
    lrWpanHelper.EnablePcapAll(std::string("./tracing/ip/ping6wsn"), true);
    AnimationInterface anim("animation.xml");
    Simulator::Stop(Seconds(240));
    Simulator::Run();
    Simulator::Destroy();
}

