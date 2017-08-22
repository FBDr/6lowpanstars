/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"


namespace ns3
{

    // Network topology
    //
    //       n0    n1	n2	...
    //       |     |	|	...
    //       =================
    //        WSN (802.15.4)
    //
    // - ICMPv6 echo request flows from n0 to n1 and back with ICMPv6 echo reply
    // - DropTail queues 
    // - Tracing of queues and packet receptions to file "wsn-ping6.tr"
    //
    // This example is based on the "ping6.cc" example.

    int
    main(int argc, char* argv[]) {
        bool pcaptrace = false;
        // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
        CommandLine cmd;
        cmd.AddValue("pcaps", "Number of packets to echo", pcaptrace);
        cmd.Parse(argc, argv);

        // Creating nodes
        NodeContainer nodes;
        nodes.Create(3);

        // Set seed for random numbers
        //
        //!!!TODO: Important to change this dynamically.
        //
        SeedManager::SetSeed(177);

        // Install mobility
        MobilityHelper mobility;
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                "MinX", DoubleValue(0.0),
                "MinY", DoubleValue(0.0),
                "DeltaX", DoubleValue(1),
                "DeltaY", DoubleValue(1),
                "GridWidth", UintegerValue(5),
                "LayoutType", StringValue("RowFirst"));
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(nodes);

        // Connecting nodes using two links
        LrWpanHelper lrWpanHelper;
        NetDeviceContainer devContainer = lrWpanHelper.Install(nodes);
        lrWpanHelper.AssociateToPan(devContainer, 1);

        std::cout << "Created " << devContainer.GetN() << " devices" << std::endl;
        std::cout << "There are " << nodes.GetN() << " nodes" << std::endl;

        // Install NDN stack on all nodes
        ndn::StackHelper ndnHelper;
        ndnHelper.SetDefaultRoutes(true);
        //ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000");
        ndnHelper.InstallAll();
        ndn::StrategyChoiceHelper::Install(nodes, "/", "/localhost/nfd/strategy/multicast");
        // Installing applications

        // Consumer
        ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
        consumerHelper.SetAttribute("NumberOfContents", StringValue("10"));
        consumerHelper.SetPrefix("/temp");
        consumerHelper.SetAttribute("Frequency", StringValue("0.025"));
        //consumerHelper.SetAttribute("LifeTime", StringValue("2"));
        //consumerHelper.SetAttribute("Randomize", StringValue("exponential"));
        consumerHelper.Install(nodes.Get(0)); // first node

        // Producer
        ndn::AppHelper producerHelper("ns3::ndn::Producer");
        // Producer will reply to all requests starting with /prefix
        producerHelper.SetPrefix("/temp");
        producerHelper.SetAttribute("PayloadSize", StringValue("50"));
        producerHelper.Install(nodes.Get(2)); // last node

        if (pcaptrace == true) {
            AsciiTraceHelper ascii;
            //lrWpanHelper.EnableAsciiAll(ascii.CreateFileStream("./tracing/ndn-trace.tr"));
            lrWpanHelper.EnablePcapAll(std::string("./tracing/ndn/ndn-trace"), true);
        }
        Simulator::Stop(Seconds(70.0));

        Simulator::Run();
        Simulator::Destroy();

        return 0;
    }

} // namespace ns3

int
main(int argc, char* argv[]) {
    return ns3::main(argc, argv);
}
