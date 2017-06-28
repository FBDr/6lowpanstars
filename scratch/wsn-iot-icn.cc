#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
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
#include <string>
#include <vector>


namespace ns3
{


    NS_LOG_COMPONENT_DEFINE("wsn-iot-icn");

    int main(int argc, char **argv) {

        //Variables and simulation configuration
        bool verbose = false;
        int rngfeed = 43221;
        int node_num = 10;
        int node_periph = 9;
        int node_head = 3;
        int con_per;
        int pro_per;
        int dist = 500;
        int totnumcontents = 100;
        int totnumnodes = 0;





        CommandLine cmd;
        cmd.AddValue("verbose", "turn on log components", verbose);
        cmd.AddValue("rng", "Set feed for random number generator", rngfeed);
        cmd.AddValue("nodes", "Number of nodes", node_num);
        cmd.AddValue("periph", "Number of peripheral nodes", node_periph);
        cmd.AddValue("head", "Number of head nodes", node_head);
        cmd.AddValue("ConPercent", "Consumer percentage", con_per);
        cmd.AddValue("ProPercent", "Producer percentage", pro_per);
        cmd.AddValue("Contents", "Total number of contents", totnumcontents);
        cmd.Parse(argc, argv);

        //Random variables
        RngSeedManager::SetSeed(1);
        RngSeedManager::SetRun(rngfeed);
        Ptr<UniformRandomVariable> Rnodes = CreateObject<UniformRandomVariable> ();
        Rnodes->SetAttribute("Min", DoubleValue(0));


        // Creating NodeContainers
        // - iot[]: 		NodeContainer with WSN nodes.
        // - br: 	  		NodeContainer with border gateway routers.
        // - csmam[]:               NodeContainer with specific border router and master node.
        // - backhaul:              NodeContainer with backhaul network nodes.

        NS_LOG_INFO("Creating IoT bubbles.");
        NodeContainer iot[node_head];
        NodeContainer br;
        NodeContainer routers;
        NodeContainer endnodes;
        NodeContainer border_backhaul[node_head];
        NodeContainer backhaul;

        //Brite
        BriteTopologyHelper bth(std::string("src/brite/examples/conf_files/RTBarabasi20.conf"));
        bth.AssignStreams(3);
        backhaul = bth.BuildBriteTopologyNDN();

        br.Create(node_head);
        routers.Add(backhaul);
        routers.Add(br);

        //Set the highest value for the node random variable.        
        totnumnodes = bth.GetNNodesTopology();
        Rnodes->SetAttribute("Max", DoubleValue(totnumnodes));

        // Create mobility objects for master and (border) routers.
        MobilityHelper mobility;
        MobilityHelper mobility_backhaul;
        mobility_backhaul.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility_backhaul.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=200.0]"), "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=200.0]"));

        mobility_backhaul.Install(backhaul);

        // Create NetDeviceContainers for LrWpan and CSMA (LAN).
        NetDeviceContainer LrWpanDevice[node_head];
        NetDeviceContainer CSMADevice[node_head];
        Ptr<ListPositionAllocator> BorderRouterPositionAlloc = CreateObject<ListPositionAllocator> ();

        for (int jdx = 0; jdx < node_head; jdx++) {
            //Add BR and master to csma-NodeContainers
            border_backhaul[jdx].Add(br.Get(jdx));
            border_backhaul[jdx].Add(backhaul.Get(Rnodes->GetValue()));
            //Add BR and WSN nodes to IoT[] domain
            iot[jdx].Create(node_periph);
            endnodes.Add(iot[jdx]);
            iot[jdx].Add(br.Get(jdx));

            mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                    "MinX", DoubleValue(-500 + jdx * dist),
                    "MinY", DoubleValue(400),
                    "DeltaX", DoubleValue(5),
                    "DeltaY", DoubleValue(5),
                    "GridWidth", UintegerValue(3),
                    "LayoutType", StringValue("RowFirst"));
            mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            mobility.Install(iot[jdx]);
            BorderRouterPositionAlloc->Add(Vector(-495 + jdx * dist, 395, 0.0));
        }

        mobility.SetPositionAllocator(BorderRouterPositionAlloc);
        mobility.Install(br);

        NS_LOG_INFO("Create channels.");

        //Create and install CSMA and LrWpan channels.
        CsmaHelper csma;
        csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
        csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
        LrWpanHelper lrWpanHelper[node_head];

        for (int jdx = 0; jdx < node_head; jdx++) {
            CSMADevice[jdx] = csma.Install(border_backhaul[jdx]);
            LrWpanDevice[jdx] = lrWpanHelper[jdx].Install(iot[jdx]);
            //lrWpanHelper.AssociateToPan(LrWpanDevCon[jdx], jdx + 10);
            lrWpanHelper[jdx].AssociateToPan(LrWpanDevice[jdx], 10);


        }

        ndn::StackHelper ndnHelper;
        ndnHelper.SetDefaultRoutes(true);
        ndnHelper.InstallAll();
        ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/bestroute2");
        ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
        ndnGlobalRoutingHelper.InstallAll();


        ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
        // Consumer will request /prefix/0, /prefix/1, ...
        consumerHelper.SetPrefix("/prefix");
        consumerHelper.SetAttribute("Frequency", StringValue("0.5")); // 10 interests a second
        //consumerHelper.Install(backhaul.Get(0)); // first node
        consumerHelper.Install(iot[0].Get(5));
        // Producer
        ndn::AppHelper producerHelper("ns3::ndn::Producer");
        // Producer will reply to all requests starting with /prefix
        producerHelper.SetPrefix("/prefix");
        producerHelper.SetAttribute("PayloadSize", StringValue("10"));
        producerHelper.Install(iot[1].Get(1)); // last node
        ndnGlobalRoutingHelper.AddOrigin("/prefix", iot[1].Get(1));
        std::cout << "Filling routing tables..." << std::endl;
        ndn::GlobalRoutingHelper::CalculateRoutes();
        //ndn::FibHelper::AddRoute(br.Get(0),"/prefix", 256, 3);

        std::cout << "Done, now starting simulator..." << std::endl;

        //ndn::FibHelper::AddRoute(br.Get(0), "/prefix", 258, 0);
        /*Tracing*/
        //Flowmonitor
        //Ptr<FlowMonitor> flowMonitor;
        //FlowMonitorHelper flowHelper;
        //flowMonitor = flowHelper.InstallAll();

        //PCAP
        AsciiTraceHelper ascii;
        //lrWpanHelper.EnablePcapAll(std::string("./traces/6lowpan/wsn"), true);
        csma.EnablePcapAll(std::string("./traces/csma"), true);
        //lrWpanHelper.EnableAsciiAll(ascii.CreateFileStream("123123.tr"));
        NS_LOG_INFO("Run Simulation.");

        //AnimationInterface anim("AWSNanimation2.xml");
        //anim.EnablePacketMetadata(true);
        for (int jdx = 0; jdx < node_head; jdx++) {
            lrWpanHelper[jdx].EnablePcapAll(std::string("./traces/6lowpan/wsn"), true);
        }

        Simulator::Stop(Seconds(60));
        Simulator::Run();
        //flowMonitor->SerializeToXmlFile("Flows.xml", true, true);
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
