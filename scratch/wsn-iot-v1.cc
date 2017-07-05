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
#include "src/network/helper/node-container.h"
#include "src/network/utils/output-stream-wrapper.h"
#include <string>
#include <vector>

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("wsn-iot-icn");

    Ptr< Node > SelectRandomLeafNode(BriteTopologyHelper & briteth) {

        uint32_t totAS = briteth.GetNAs();
        NS_LOG_INFO("The total number of AS is: " << totAS);
        uint32_t leafnum = 0;
        uint32_t selAS = 0;
        uint32_t selLN = 0;
        Ptr<UniformRandomVariable> Ras = CreateObject<UniformRandomVariable> ();
        Ptr<UniformRandomVariable> Rleaf = CreateObject<UniformRandomVariable> ();

        selAS = round(Ras->GetValue((double) (0), (double) (totAS - 1)));
        NS_LOG_INFO("Selected AS num: " << selAS);
        leafnum = briteth.GetNLeafNodesForAs(selAS);
        selLN = round(Rleaf->GetValue((double) (0), (double) (leafnum - 1)));
        NS_LOG_INFO("Selected leafnode num: " << selLN);
        return briteth.GetLeafNodeForAs(selAS, selLN);
    }

    void
    shuffle_array(std::vector<int>& arrayf) {
        Ptr<UniformRandomVariable> shuffles = CreateObject<UniformRandomVariable> ();
        shuffles->SetAttribute("Min", DoubleValue(2));
        shuffles->SetAttribute("Max", DoubleValue(20));

        for (int cnt = 0; cnt < (int) (shuffles->GetValue()); cnt++) {

            std::random_shuffle(arrayf.begin(), arrayf.end());
        }
    };

    std::vector<Ipv6Address> CreateAddrResBucket(std::vector<Ipv6Address>& arrayf, int numContents) {

        std::vector<Ipv6Address> returnBucket;
        Ptr<UniformRandomVariable> shuffles = CreateObject<UniformRandomVariable> ();
        shuffles->SetAttribute("Min", DoubleValue(0));
        shuffles->SetAttribute("Max", DoubleValue(arrayf.size() - 1));

        for (int itx = 0; itx < numContents; itx++) {
            returnBucket.push_back(arrayf[shuffles->GetValue()]);
            std::cout << "Content chunk: " << itx << " is at: " << returnBucket[itx] << std::endl;
        }

        return returnBucket;
    }

    /*  -
        -
        - NDN and IP specific functions.
        -
     */




    void NDN_stack(int &node_head, NodeContainer iot[], NodeContainer & backhaul, NodeContainer &endnodes, int &totnumcontents, BriteTopologyHelper &bth, int &simtime, int &num_Con) {
        ndn::StackHelper ndnHelper;
        std::string prefix = "/SensorData";
        ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
        ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrotV2");
        ndn::AppHelper producerHelper("ns3::ndn::Producer");
        ndnHelper.SetDefaultRoutes(true);
        Ptr<UniformRandomVariable> Rinterval = CreateObject<UniformRandomVariable> ();
        ApplicationContainer apps;

        for (int jdx = 0; jdx < node_head; jdx++) {
            ndnHelper.Install(iot[jdx]); //We want caching in the IoT domains.
        }
        //ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache"); //We don't want caching in the backhaul network.
        ndnHelper.Install(backhaul);
        ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/bestroute2");
        ndnGlobalRoutingHelper.InstallAll();
        // Consumer will request /prefix/0, /prefix/1, ...
        consumerHelper.SetPrefix(prefix);
        consumerHelper.SetAttribute("NumberOfContents", StringValue(std::to_string(totnumcontents))); // 10 different contents


        for (int jdx = 0; jdx < num_Con; jdx++) {
            consumerHelper.SetAttribute("Frequency", StringValue(boost::lexical_cast<std::string>(Rinterval->GetValue((double) (0.01), (double) (1)))));
            apps = consumerHelper.Install(SelectRandomLeafNode(bth));
            apps.Start(Seconds(120.0));
            apps.Stop(Seconds(simtime - 5));
        }

        // Producer
        // Producer will reply to all requests starting with /prefix

        //Create vector with content numbers.
        std::vector<int> content_chunks(totnumcontents);
        std::iota(std::begin(content_chunks), std::end(content_chunks), 1);
        shuffle_array(content_chunks);

        for (int jdx = 0; jdx < totnumcontents; jdx++) {
            std::string cur_prefix;
            int cur_node = jdx % ((int) endnodes.GetN());
            cur_prefix = prefix + "/" + std::to_string(content_chunks[jdx]);
            producerHelper.SetPrefix(cur_prefix);
            producerHelper.SetAttribute("PayloadSize", StringValue("10"));

            producerHelper.Install(endnodes.Get(cur_node));
            ndnGlobalRoutingHelper.AddOrigin(cur_prefix, endnodes.Get(cur_node));
        }
        ndnGlobalRoutingHelper.AddOrigin(prefix, iot[1].Get(1)); //BUG?
        std::cout << "Filling routing tables..." << std::endl;
        ndn::GlobalRoutingHelper::CalculateRoutes();
        //ndn::FibHelper::AddRoute(br.Get(0),"/prefix", 256, 3);
        std::cout << "Done, now starting simulator..." << std::endl;
    }

    void sixlowpan_stack(int &node_periph, int &node_head, int &totnumcontents, BriteTopologyHelper &bth,
            NetDeviceContainer LrWpanDevice[], NetDeviceContainer SixLowpanDevice[], NetDeviceContainer CSMADevice[],
            Ipv6InterfaceContainer i_6lowpan[], Ipv6InterfaceContainer i_csma[],
            std::vector<Ipv6Address> &IPv6Bucket, std::vector<Ipv6Address> &AddrResBucket,
            NodeContainer &endnodes, NodeContainer &br, NodeContainer & backhaul) {
        int subn = 0;
        RipNgHelper ripNgRouting;
        Ipv6ListRoutingHelper listRH;
        InternetStackHelper internetv6;
        SixLowPanHelper sixlowpan;
        Ipv6AddressHelper ipv6;

        //Install internet stack.
        listRH.Add(ripNgRouting, 0);
        internetv6.SetIpv4StackInstall(false);
        internetv6.Install(endnodes);
        internetv6.SetRoutingHelper(listRH);
        internetv6.Install(br);
        internetv6.Install(backhaul);
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("Routing table.txt", ios_base::trunc);
        //ripNgRouting.PrintRoutingTableAllEvery(Seconds(40.0), routingStream);
        
        //Assign addresses for backhaul.
        std::string addresss;
        addresss = "2001:0:" + std::to_string(1337) + "::";
        const char * c = addresss.c_str();
        ipv6.SetBase(Ipv6Address(c), Ipv6Prefix(64));
        bth.AssignIpv6Addresses(ipv6);

        //Assign addresses for iot domains.
        NS_LOG_INFO("Assign addresses.");
        for (int jdx = 0; jdx < node_head; jdx++) {
            SixLowpanDevice[jdx] = sixlowpan.Install(LrWpanDevice[jdx]);
            subn++;
            addresss = "2001:" + std::to_string(subn) + "::";
            const char * c = addresss.c_str();
            ipv6.SetBase(Ipv6Address(c), Ipv6Prefix(64));
            i_6lowpan[jdx] = ipv6.Assign(SixLowpanDevice[jdx]);
            subn++;
            addresss = "2001:" + std::to_string(subn) + "::";
            c = addresss.c_str();
            ipv6.SetBase(Ipv6Address(c), Ipv6Prefix(64));
            i_csma[jdx] = ipv6.Assign(CSMADevice[jdx]);
            //Set forwarding rules.
            i_6lowpan[jdx].SetDefaultRouteInAllNodes(node_periph);
            i_6lowpan[jdx].SetForwarding(node_periph, true);
            //i_csma[jdx].SetDefaultRouteInAllNodes(1);
            //i_csma[jdx].SetDefaultRouteInAllNodes(0);
            i_csma[jdx].SetForwarding(0, true);
            i_csma[jdx].SetForwarding(1, true);

        }

        //Create IPv6Addressbucket containing all IoT node domains.
        for (int idx = 0; idx < node_head; idx++) {
            for (int jdx = 0; jdx < node_periph; jdx++) {
                IPv6Bucket.push_back(i_6lowpan[idx].GetAddress(jdx, 1));
                NS_LOG_INFO("Filling IPv6 bucket " << i_6lowpan[idx].GetAddress(jdx, 1));
            }
        }
        AddrResBucket = CreateAddrResBucket(IPv6Bucket, totnumcontents);
    }

    void sixlowpan_apps(int &node_periph, int &node_head, int &num_Con, NodeContainer iot[], NodeContainer all,
            std::vector<Ipv6Address> &AddrResBucket, ApplicationContainer &apps,
            Ipv6InterfaceContainer i_6lowpan[], int &simtime, BriteTopologyHelper & briteth) {


        uint32_t packetSize = 1024;
        uint32_t maxPacketCount = 20000;
        uint16_t port = 9;
        int cur_con = 0;
        Ptr<UniformRandomVariable> Rinterval = CreateObject<UniformRandomVariable> ();
        Ptr<UniformRandomVariable> Rcon = CreateObject<UniformRandomVariable> ();

        CoapClientHelper client(i_6lowpan[1].GetAddress(4, 1), port);
        CoapServerHelper server(port);

        //Server
        for (int itr = 0; itr < node_head; itr++) {
            for (int jdx = 0; jdx < node_periph; jdx++) {
                //Install server application on every node, in every IoT domain.
                apps = server.Install(iot[itr].Get(jdx));
                server.SetIPv6Bucket(apps.Get(0), AddrResBucket);

            }
        }
        apps.Start(Seconds(1.0));
        apps.Stop(Seconds(simtime));

        //Client
        client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        client.SetAttribute("PacketSize", UintegerValue(packetSize));

        for (int jdx = 0; jdx < num_Con; jdx++) {
            cur_con = (int) (Rcon->GetValue()*(all.GetN() - 1));
            NS_LOG_INFO("Selected: " << cur_con << " as consumer. ");
            client.SetAttribute("Interval", TimeValue(Seconds(Rinterval->GetValue((double) (0.01), (double) (1)))));

            apps = client.Install(SelectRandomLeafNode(briteth));
            client.SetIPv6Bucket(apps.Get(0), AddrResBucket);
            apps.Start(Seconds(120.0));
            apps.Stop(Seconds(simtime - 5));
        }

    }

    int main(int argc, char **argv) {

        //Variables and simulation configuration
        bool verbose = false;
        int rngfeed = 43221;
        int node_num = 10;
        int node_periph = 9;
        int node_head = 3;
        int con_per = 50;
        int pro_per = 0;
        int num_Con = 0;
        //int num_Pro;
        int dist = 100;
        int totnumcontents = 100;
        int totalnodes = 0;
        int simtime = 60;
        bool ndn = true;
        bool pcaptracing = true;

        CommandLine cmd;
        cmd.AddValue("verbose", "turn on log components", verbose);
        cmd.AddValue("rng", "Set feed for random number generator", rngfeed);
        cmd.AddValue("nodes", "Number of nodes", node_num);
        cmd.AddValue("periph", "Number of peripheral nodes", node_periph);
        cmd.AddValue("head", "Number of head nodes", node_head);
        cmd.AddValue("ConPercent", "Consumer percentage", con_per);
        cmd.AddValue("ProPercent", "Producer percentage", pro_per);
        cmd.AddValue("Contents", "Total number of contents", totnumcontents);
        cmd.AddValue("ndn", "ndn=0 --> ip, ndn=1 --> NDN", ndn);
        cmd.AddValue("simtime", "Simulation duration in seconds", simtime);
        cmd.AddValue("pcap", "Enable or disable pcap tracing", pcaptracing);
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
        NodeContainer all;

        //Brite
        //BriteTopologyHelper bth(std::string("src/brite/examples/conf_files/RTBarabasi20.conf"));
        BriteTopologyHelper bth(std::string("src/brite/examples/conf_files/TD_ASBarabasi_RTWaxman.conf"));
        bth.AssignStreams(3);
        backhaul = bth.BuildBriteTopology2();

        br.Create(node_head);
        routers.Add(backhaul);
        routers.Add(br);
        totalnodes = node_head * node_periph + node_head + bth.GetNNodesTopology();
        num_Con = totalnodes * (float) con_per / 100.0;
        all.Add(routers);
        all.Add(endnodes);
        all.Add(backhaul);

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
                    "GridWidth", UintegerValue(5),
                    "LayoutType", StringValue("RowFirst"));
            mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            mobility.Install(iot[jdx]);
            BorderRouterPositionAlloc->Add(Vector(-95 + jdx * dist, 395, 0.0));
        }

        NS_ASSERT_MSG(endnodes.GetN() <= (uint32_t) totnumcontents, "The number of contents should be larger than the number of endnodes in the current implementation");

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


        if (ndn) {
            NDN_stack(node_head, iot, backhaul, endnodes, totnumcontents, bth, simtime, num_Con);
            ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");
        }

        /*
         IP
         */
        NetDeviceContainer SixLowpanDevice[node_head];
        Ipv6InterfaceContainer i_6lowpan[node_head];
        Ipv6InterfaceContainer i_csma[node_head];
        Ipv6InterfaceContainer i_backhaul;
        std::vector<Ipv6Address> IPv6Bucket;
        std::vector<Ipv6Address> AddrResBucket;
        ApplicationContainer apps;

        if (!ndn) {
            NS_LOG_INFO("Installing 6lowpan stack.");
            sixlowpan_stack(node_periph, node_head, totnumcontents, bth, LrWpanDevice, SixLowpanDevice, CSMADevice, i_6lowpan, i_csma, IPv6Bucket, AddrResBucket, endnodes, br, backhaul);

            NS_LOG_INFO("Creating Applications.");
            sixlowpan_apps(node_periph, node_head, num_Con, iot, all, AddrResBucket, apps, i_6lowpan, simtime, bth);
        }



        /*Tracing*/
        //Flowmonitor
        Ptr<FlowMonitor> flowMonitor;
        FlowMonitorHelper flowHelper;

        if (!ndn) {
            flowMonitor = flowHelper.InstallAll();
        }


        //PCAP
        AsciiTraceHelper ascii;
        //lrWpanHelper.EnablePcapAll(std::string("./traces/6lowpan/wsn"), true);
        csma.EnablePcapAll(std::string("traces/csma"), true);

        NS_LOG_INFO("Run Simulation.");

        //AnimationInterface anim("AWSNanimation2.xml");
        //anim.EnablePacketMetadata(true);
        if (pcaptracing)
            for (int jdx = 0; jdx < node_head; jdx++) {
                lrWpanHelper[jdx].EnablePcapAll(std::string("traces/6lowpan/wsn"), true);
            }

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
