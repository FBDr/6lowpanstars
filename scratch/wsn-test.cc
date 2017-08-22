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
#include "ns3/lr-wpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("Ping6WsnExample");

    int main(int argc, char **argv) {
        bool verbose = false;

        CommandLine cmd;
        cmd.AddValue("verbose", "turn on log components", verbose);
        cmd.Parse(argc, argv);

        NS_LOG_INFO("Create nodes.");
        NodeContainer nodes;
        nodes.Create(2);

        // Set seed for random numbers
        SeedManager::SetSeed(167);

        // Install mobility
        MobilityHelper mobility;
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

        Ptr<ListPositionAllocator> nodesPositionAlloc = CreateObject<ListPositionAllocator> ();
        nodesPositionAlloc->Add(Vector(0.0, 0.0, 0.0));
        nodesPositionAlloc->Add(Vector(50.0, 0.0, 0.0));
        mobility.SetPositionAllocator(nodesPositionAlloc);
        mobility.Install(nodes);

        NS_LOG_INFO("Create channels.");
        LrWpanHelper lrWpanHelper;
        // Add and install the LrWpanNetDevice for each node
        // lrWpanHelper.EnableLogComponents();
        NetDeviceContainer devContainer = lrWpanHelper.Install(nodes);
        lrWpanHelper.AssociateToPan(devContainer, 10);

        std::cout << "Created " << devContainer.GetN() << " devices" << std::endl;
        std::cout << "There are " << nodes.GetN() << " nodes" << std::endl;

        /* Install IPv4/IPv6 stack */
        NS_LOG_INFO("Install Internet stack.");
        InternetStackHelper internetv6;
        internetv6.SetIpv4StackInstall(false);
        internetv6.Install(nodes);

        // Install NDN stack on all nodes
        ndn::StackHelper ndnHelper;
        ndnHelper.SetDefaultRoutes(true);
        ndnHelper.InstallAll();

        // Choosing forwarding strategy
        ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");

        // Installing applications

        // Consumer
        ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
        // Consumer will request /prefix/0, /prefix/1, ...
        consumerHelper.SetPrefix("/prefix");
        consumerHelper.SetAttribute("Frequency", StringValue("10")); // 10 interests a second
        consumerHelper.Install(nodes.Get(0)); // first node

        // Producer
        ndn::AppHelper producerHelper("ns3::ndn::Producer");
        // Producer will reply to all requests starting with /prefix
        producerHelper.SetPrefix("/prefix");
        producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
        producerHelper.Install(nodes.Get(1)); // last node

        AsciiTraceHelper ascii;
        lrWpanHelper.EnableAsciiAll(ascii.CreateFileStream("ping6wsn.tr"));
        lrWpanHelper.EnablePcapAll(std::string("ping6wsn"), true);

        NS_LOG_INFO("Run Simulation.");
        Simulator::Stop(Seconds(20.0));
        Simulator::Run();
        Simulator::Destroy();
        NS_LOG_INFO("Done.");
        return 0;
    }
}

int
main(int argc, char* argv[]) {
    //LogComponentEnable("ndn.Producer", LOG_LEVEL_FUNCTION);
    return ns3::main(argc, argv);
}

