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

#include "stacks_header.h"
#include "g_function_header.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("wsn-iot-v1");

    static void GetTotalEnergyConsumption(std::string context, double oldValue, double newValue) {
        
        double nodenum = std::stoi(context);
        std::ofstream outfile;
        outfile.open("energy.txt", std::ios_base::app);
        outfile << nodenum << " " << Simulator::Now().GetSeconds() << " " << newValue << std::endl;
    }

    int main(int argc, char **argv) {

        //Variables and simulation configuration
        bool verbose = false;
        int rngfeed = 43221;
        int node_num = 10;
        int node_periph = 9;
        int node_head = 3;

        //Consumers
        int con_leaf = 0;
        int con_inside = 0;
        int con_gtw = 0;



        //int num_Pro;
        int dist = 100;
        int totnumcontents = 100;
        int totalleafs = 0;
        int simtime = 60;
        bool ndn = true;
        bool pcaptracing = true;
        int cache = 100;
        double freshness = 0;
        bool ipbackhaul = false;
        bool useContiki = false;
        int payloadsize = 10;
        double min_freq = 0.0166;
        double max_freq = 5;
        int csma_delay = 20;
        int dtracefreq = 10000;
        std::string zm_q = "0.7";
        std::string zm_s = "0.7";

        CommandLine cmd;
        cmd.AddValue("verbose", "turn on log components", verbose); //Not implemented
        cmd.AddValue("rng", "Set feed for random number generator", rngfeed);
        cmd.AddValue("nodes", "Number of nodes", node_num); //Not implemented
        cmd.AddValue("periph", "Number of peripheral nodes", node_periph);
        cmd.AddValue("head", "Number of head nodes", node_head);
        cmd.AddValue("con_leaf", "Number of leafnodes acting as consumers", con_leaf);
        cmd.AddValue("con_inside", "Number of nodes inside domains acting as consumers", con_inside);
        cmd.AddValue("con_gtw", "Number of consumers on gateway nodes.", con_gtw);
        cmd.AddValue("min_freq", "Minimum packet transmission frequency in pkts/s", min_freq);
        cmd.AddValue("max_freq", "Maximum packet transmission frequency in pkts/s", max_freq);
        cmd.AddValue("Contents", "Total number of contents", totnumcontents);
        cmd.AddValue("ndn", "ndn=0 --> ip, ndn=1 --> NDN", ndn);
        cmd.AddValue("simtime", "Simulation duration in seconds", simtime);
        cmd.AddValue("pcap", "Enable or disable pcap tracing", pcaptracing);
        cmd.AddValue("cache", "Enable caching. Set the number of items in caches", cache);
        cmd.AddValue("freshness", "Enable freshness checking, by setting freshness duration.", freshness);
        cmd.AddValue("ipbackhaul", "Use IP model for backhaul network.", ipbackhaul); //Not implemented
        cmd.AddValue("payloadsize", "Set the default payloadsize", payloadsize);
        cmd.AddValue("zm_q", "Set the alpha parameter of the ZM distribution", zm_q);
        cmd.AddValue("zm_s", "Set the alpha parameter of the ZM distribution", zm_s);
        cmd.AddValue("csma_delay", "Set the delay for the Br <-> Backhaul connection.", csma_delay);
        cmd.AddValue("contiki", "Enable contikimac on nodes.", useContiki);
        cmd.AddValue("dtracefreq", "Averaging period for droptrace file.", dtracefreq);
        cmd.Parse(argc, argv);

        //Random variables
        RngSeedManager::SetSeed(1);
        RngSeedManager::SetRun(rngfeed);
/*
        for (int jdx = 0; jdx < 10; jdx++) {
            RngSeedManager::SetSeed(1);
            RngSeedManager::SetRun(2);
            Ptr<UniformRandomVariable> Rnode = CreateObject<UniformRandomVariable> ();
            Rnode->SetStream(1);
            std::cout<<Rnode->GetValue(0,1) <<std::endl;
            std::cout<<Rnode->GetValue(0,1) <<std::endl;
            std::cout<<Rnode->GetValue(0,1) <<std::endl;
            std::cout<<Rnode->GetValue(0,1) <<std::endl;
            std::cout<<Rnode->GetValue(0,1) <<std::endl;
            std::cout<<std::endl;
        }
*/

        //Clean up old files
        remove("energy.txt");
        remove("hopdelay.txt");
        remove("pktloss.txt");
        remove("bytes.txt");
        remove("bytes2.txt");
        /*
                LogComponentEnable("CoapClientApplication", LOG_LEVEL_ALL);
                LogComponentEnable("CoapServerApplication", LOG_LEVEL_ALL);
                LogComponentEnable("wsn-iot-v1", LOG_LEVEL_ALL);
         */

        //Paramter settings
        //GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true)); //Calculate checksums for Wireshark

        // Creating NodeContainers
        // - iot[]:                 NodeContainer with WSN nodes + border gateway nodes.
        // - br:                    NodeContainer with border gateway routers.
        // - border_backhaul[]:     NodeContainer with specific connected border router and master node pair.
        // - backhaul:              NodeContainer with backhaul network nodes.
        // - routers:               NodeContainer with all nodes doing routing = backhaul + br.
        // - endnodes:              NodeContainer with all IoT sensor nodes in all bubbles.
        // - all                    NodeContainer with all nodes.
        // - bubble_leafnode        Std::set with all leafnodes associated to IoT bubbles. 

        NS_LOG_INFO("Creating NodeContainers.");
        NodeContainer iot[node_head];
        NodeContainer br;
        NodeContainer routers;
        NodeContainer endnodes;
        NodeContainer border_backhaul[node_head];
        NodeContainer backhaul;
        NodeContainer all;
        std::set<Ptr < Node>> bubble_leafnode;

        //Brite
        //BriteTopologyHelper bth(std::string("src/brite/examples/conf_files/RTBarabasi20.conf"));
        BriteTopologyHelper bth(std::string("./TD_ASBarabasi_RTWaxman.conf"));
        bth.AssignStreams(3);
        backhaul = bth.BuildBriteTopology2();
        br.Create(node_head);
        routers.Add(backhaul);
        routers.Add(br);
        //totalnodes = node_head* node_periph + node_head + bth.GetNNodesTopology();
        totalleafs = bth.GetNLeafNodes();

        NS_LOG_INFO("This topology contains: " << (int) totalleafs << " leafnodes.");
        all.Add(routers);
        all.Add(endnodes);
        all.Add(backhaul);

        // Create mobility objects for IoT sensor and (border) routers.
        MobilityHelper mobility;

        // Create NetDeviceContainers for LrWpan and CSMA (LAN).
        NetDeviceContainer LrWpanDevice[node_head];
        NetDeviceContainer CSMADevice[node_head];
        Ptr<ListPositionAllocator> BorderRouterPositionAlloc = CreateObject<ListPositionAllocator> ();
        Ptr<UniformRandomVariable> Rnode = CreateObject<UniformRandomVariable> (); //Random number for selecting random leafnode.
        Rnode->SetStream(1);
        for (int jdx = 0; jdx < node_head; jdx++) {
            Ptr<Node> cur_blnode;
            //Add BR and master to csma-NodeContainers
            border_backhaul[jdx].Add(br.Get(jdx));
            cur_blnode = SelectRandomLeafNode(bth, Rnode);
            border_backhaul[jdx].Add(cur_blnode);
            bth.SetConnectedLeaf(cur_blnode);
            //Add BR and WSN nodes to IoT[] domain
            iot[jdx].Create(node_periph);
            endnodes.Add(iot[jdx]); // Add iot[] container to endnodes container before adding br.
            iot[jdx].Add(br.Get(jdx));

            mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                    "MinX", DoubleValue(-100 + jdx * dist),
                    "MinY", DoubleValue(400),
                    "DeltaX", DoubleValue(5),
                    "DeltaY", DoubleValue(5),
                    "GridWidth", UintegerValue(5),
                    "LayoutType", StringValue("RowFirst"));
            mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            mobility.Install(iot[jdx]);
            BorderRouterPositionAlloc->Add(Vector(-95 + jdx * dist, 395, 0.0));
        }

        //NS_ASSERT_MSG(endnodes.GetN() <= (uint32_t) totnumcontents, "The number of contents should be larger than the number of endnodes in the current implementation");

        mobility.SetPositionAllocator(BorderRouterPositionAlloc);
        mobility.Install(br);

        NS_LOG_INFO("Create channels.");

        //Create and install CSMA and LrWpan channels.
        CsmaHelper csma;
        csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
        if (ndn && ipbackhaul) {
            csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(csma_delay)));
        } else {
            csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
        }

        LrWpanHelper lrWpanHelper[node_head];

        for (int jdx = 0; jdx < node_head; jdx++) {
            CSMADevice[jdx] = csma.Install(border_backhaul[jdx]);
            LrWpanDevice[jdx] = lrWpanHelper[jdx].InstallIoT(iot[jdx], node_periph, node_head, useContiki, node_periph);
            //LrWpanDevice[jdx] = lrWpanHelper[jdx].Install(iot[jdx]);
            lrWpanHelper[jdx].AssociateToPan(LrWpanDevice[jdx], 10);
        }



        //Energy framework

        int size = node_head * node_periph + 1;
        Ptr<LrWpanRadioEnergyModel> em[size];
        Ptr<BasicEnergySource> es[size];
        Ptr<LrWpanContikiMac> mac[size]; //Change Mac

        int endN = 0;
        for (int jdx = 0; jdx < node_head; jdx++) {
            for (int idx = 0; idx < node_periph; idx++) {
                endN++;
                em[endN] = CreateObject<LrWpanRadioEnergyModel> ();
                es[endN] = CreateObject<BasicEnergySource> ();
                mac[endN] = CreateObject<LrWpanContikiMac> ();
                es[endN]->SetSupplyVoltage(3.3);
                es[endN]->SetInitialEnergy(10800); //1 AA battery
                es[endN]->SetEnergyUpdateInterval(Time(Seconds(3000)));
                es[endN]->SetNode(iot[jdx].Get(idx));
                es[endN]->AppendDeviceEnergyModel(em[endN]);
                em[endN]->SetEnergySource(es[endN]);

                Ptr<LrWpanNetDevice> device = DynamicCast<LrWpanNetDevice> (iot[jdx].Get(idx)->GetDevice(0));
                em[endN]->AttachPhy(device->GetPhy()); //Loopback=0?
                es[endN]->TraceConnect("RemainingEnergy", std::to_string(iot[jdx].Get(idx)->GetId()), MakeCallback(&GetTotalEnergyConsumption));

                //device->SetMac(mac[endN]); //Meerdere devices hebben hetzelfde mac address!
                //device->GetMac ()->SetAttribute ("SleepTime", DoubleValue (0.05));
                //device->GetPhy()->TraceConnect("TrxState", std::string("phy0"), MakeCallback(&StateChangeNotification));
            }
        }

        /*
         NDN 
         */


        if (ndn) {
            NDN_stack(node_head, node_periph, iot, backhaul, endnodes, bth, simtime, con_leaf, con_inside, con_gtw,
                    cache, freshness, ipbackhaul, payloadsize, zm_q, zm_s, min_freq, max_freq);
            ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");
            L2RateTracer::InstallAll("drop-trace.txt", Seconds(dtracefreq));
        }


        /*
         IP
         */


        NetDeviceContainer SixLowpanDevice[node_head];
        Ipv6InterfaceContainer i_6lowpan[node_head];
        Ipv6InterfaceContainer i_csma[node_head];
        Ipv6InterfaceContainer i_backhaul;
        std::vector< std::vector<Ipv6Address> > IPv6Bucket; //Vector containing all producer IPv6 addresses.
        std::vector< std::vector<Ipv6Address> > AddrResBucket; //Content address lookup vector.
        IPv6Bucket.resize(node_head);
        AddrResBucket.resize(node_head);

        ApplicationContainer apps;

        if (!ndn) {
            NS_LOG_INFO("Installing 6lowpan stack.");
            sixlowpan_stack(node_periph, node_head, totnumcontents, bth, LrWpanDevice, SixLowpanDevice, CSMADevice, i_6lowpan, i_csma, IPv6Bucket, AddrResBucket, endnodes, br, backhaul);

            NS_LOG_INFO("Creating Applications.");
            sixlowpan_apps(node_periph, node_head, iot, all, AddrResBucket, apps, i_6lowpan, simtime, bth, payloadsize, zm_q, zm_s, con_leaf, con_inside, con_gtw, min_freq, max_freq);
        }



        /*
         Tracing
         */
        Ptr<FlowMonitor> flowMonitor;
        FlowMonitorHelper flowHelper;
        AsciiTraceHelper ascii;

        if (!ndn) {
            flowMonitor = flowHelper.InstallAll();
            Simulator::Schedule(Seconds(120), &ReduceRouteFreq, routers);
        }


        //AnimationInterface anim("AWSNanimation2.xml");
        //anim.EnablePacketMetadata(true);

        if (pcaptracing) {
            csma.EnablePcapAll(std::string("traces/csma"), true);

            for (int jdx = 0; jdx < node_head; jdx++) {
                lrWpanHelper[jdx].EnablePcapAll(std::string("traces/6lowpan/wsn"), true);
            }

        }


        NS_LOG_INFO("Run Simulation.");
        Simulator::Stop(Seconds(simtime));
        Simulator::Run();
        if (!ndn) {
            flowMonitor->SerializeToXmlFile("Flows.xml", true, true);
        }
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
