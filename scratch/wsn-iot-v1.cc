#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
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
#include <string>
#include <vector>

namespace ns3 {

    Ptr< Node > SelectRandomLeafNode(BriteTopologyHelper & briteth) {

        uint32_t totAS = briteth.GetNAs();
        uint32_t leafnum = 0;
        uint32_t selAS = 0;
        uint32_t selLN = 0;
        Ptr<UniformRandomVariable> Ras = CreateObject<UniformRandomVariable> ();
        Ptr<UniformRandomVariable> Rleaf = CreateObject<UniformRandomVariable> ();

        selAS = Ras->GetValue((double) (0), (double) (totAS - 1));
        leafnum = briteth.GetNLeafNodesForAs(selAS);
        selLN = Rleaf->GetValue((double) (0), (double) (leafnum - 1));

        return briteth.GetLeafNodeForAs(selAS, selLN);
    }

    std::vector<Ipv6Address> CreateAddrResBucket(std::vector<Ipv6Address>& arrayf, int numContents) {

        std::vector<Ipv6Address> returnBucket;
        Ptr<UniformRandomVariable> shuffles = CreateObject<UniformRandomVariable> ();
        shuffles->SetAttribute("Min", DoubleValue(0));
        shuffles->SetAttribute("Max", DoubleValue(arrayf.size() - 1));

        for (int itx = 0; itx < numContents; itx++) {
            returnBucket.push_back(arrayf[shuffles->GetValue()]);
            //std::cout << "Content chunk: " << itx << " is at: " << returnBucket[itx] << std::endl;
        }

        return returnBucket;
    }




    NS_LOG_COMPONENT_DEFINE("wsn-iot-icn");

    void NDN_stack(int &node_head, NodeContainer iot[], NodeContainer &backhaul, ndn::StackHelper &ndnHelper, ndn::GlobalRoutingHelper &ndnGlobalRoutingHelper, ndn::AppHelper &consumerHelper, ndn::AppHelper &producerHelper) {
        ndnHelper.SetDefaultRoutes(true);
        for (int jdx = 0; jdx < node_head; jdx++) {
            ndnHelper.Install(iot[jdx]); //We want caching in the IoT domains.
        }
        ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache"); //We don't want caching in the backhaul network.
        ndnHelper.Install(backhaul);
        ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/bestroute2");
        ndnGlobalRoutingHelper.InstallAll();
        // Consumer will request /prefix/0, /prefix/1, ...
        consumerHelper.SetPrefix("/prefix");
        consumerHelper.SetAttribute("Frequency", StringValue("0.5")); // 10 interests a second
        //consumerHelper.Install(backhaul.Get(0)); // first node
        consumerHelper.Install(iot[0].Get(5));
        // Producer
        // Producer will reply to all requests starting with /prefix
        producerHelper.SetPrefix("/prefix");
        producerHelper.SetAttribute("PayloadSize", StringValue("10"));
        producerHelper.Install(iot[1].Get(1)); // last node
        ndnGlobalRoutingHelper.AddOrigin("/prefix", iot[1].Get(1));
        std::cout << "Filling routing tables..." << std::endl;
        ndn::GlobalRoutingHelper::CalculateRoutes();
        //ndn::FibHelper::AddRoute(br.Get(0),"/prefix", 256, 3);
        std::cout << "Done, now starting simulator..." << std::endl;
    }

    int main(int argc, char **argv) {

        //Variables and simulation configuration
        bool verbose = false;
        int rngfeed = 43221;
        int node_num = 10;
        int node_periph = 9;
        int node_head = 3;
        int con_per;
        int pro_per;
        int dist = 100;
        int totnumcontents = 100;
        int subn = 0;
        bool ndn = false;

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
        //BriteTopologyHelper bth(std::string("src/brite/examples/conf_files/RTBarabasi20.conf"));
        BriteTopologyHelper bth(std::string("src/brite/examples/conf_files/TD_ASBarabasi_RTWaxman.conf"));
        bth.AssignStreams(3);
        backhaul = bth.BuildBriteTopology2();

        br.Create(node_head);
        routers.Add(backhaul);
        routers.Add(br);

        // Create mobility objects for master and (border) routers.
        MobilityHelper mobility;

        // Create NetDeviceContainers for LrWpan and CSMA (LAN).
        NetDeviceContainer LrWpanDevice[node_head];
        NetDeviceContainer CSMADevice[node_head];
        Ptr<ListPositionAllocator> BorderRouterPositionAlloc = CreateObject<ListPositionAllocator> ();

        for (int jdx = 0; jdx < node_head; jdx++) {

            //Add BR and master to csma-NodeContainers
            border_backhaul[jdx].Add(br.Get(jdx));
            border_backhaul[jdx].Add(SelectRandomLeafNode(bth));
            //Add BR and WSN nodes to IoT[] domain
            iot[jdx].Create(node_periph);
            endnodes.Add(iot[jdx]);
            iot[jdx].Add(br.Get(jdx));

            mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                    "MinX", DoubleValue(-100 + jdx * dist),
                    "MinY", DoubleValue(400),
                    "DeltaX", DoubleValue(5),
                    "DeltaY", DoubleValue(5),
                    "GridWidth", UintegerValue(3),
                    "LayoutType", StringValue("RowFirst"));
            mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            mobility.Install(iot[jdx]);
            BorderRouterPositionAlloc->Add(Vector(-95 + jdx * dist, 395, 0.0));
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


        /*
         NDN 
         */

        ndn::StackHelper ndnHelper;
        ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
        ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
        ndn::AppHelper producerHelper("ns3::ndn::Producer");
        if (ndn) {
            NDN_stack(node_head, iot, backhaul, ndnHelper, ndnGlobalRoutingHelper, consumerHelper, producerHelper);
        }


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
