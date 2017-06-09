#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/netanim-module.h"
#include "src/network/model/node.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/brite-module.h"
#include <string>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Ping6WsnExample");

int main(int argc, char **argv) {

    //Variables and simulation configuration
    bool verbose = false;
    int rngfeed = 43221;
    int node_num = 10;
    int node_periph = 9;
    int node_head = 2;
    int con_per;
    int pro_per;
    int dist = 500;
    int subn = 0;

    CommandLine cmd;
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.AddValue("rng", "Set feed for random number generator", rngfeed);
    cmd.AddValue("nodes", "Number of nodes", node_num);
    cmd.AddValue("periph", "Number of peripheral nodes", node_periph);
    cmd.AddValue("head", "Number of head nodes", node_head);
    cmd.AddValue("ConPercent", "Consumer percentage", con_per);
    cmd.AddValue("ProPercent", "Producer percentage", pro_per);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(rngfeed);

    Config::SetDefault("ns3::Ipv6L3Protocol::SendIcmpv6Redirect", BooleanValue(false));

    if (verbose) {
        LogComponentEnable("Ping6WsnExample", LOG_LEVEL_INFO);
        /*
                LogComponentEnable("Ipv6Extension", LOG_LEVEL_ALL);
                LogComponentEnable("Ipv6StaticRouting", LOG_LEVEL_ALL);
                LogComponentEnable("Ipv6ListRouting", LOG_LEVEL_ALL);
                LogComponentEnable("Icmpv6L4Protocol", LOG_LEVEL_ALL);
                LogComponentEnable("Ipv6L3Protocol", LOG_LEVEL_ALL);
                LogComponentEnable("Ipv6StaticRouting", LOG_LEVEL_ALL);
                LogComponentEnable("Ipv6Interface", LOG_LEVEL_ALL);
                LogComponentEnable("Ping6Application", LOG_LEVEL_ALL);
         */
        LogComponentEnable("Ping6Application", LOG_LEVEL_ALL);
    }

    // Creating NodeContainers
    // - iot[]: 		NodeContainer with WSN nodes.
    // - br: 	  		NodeContainer with border gateway routers.
    // - Master:  	NodeContainer with the top router only.
    // - lcsmam[]:	NodeContainer with specific border router and master node.

    NS_LOG_INFO("Creating IoT bubbles.");
    NodeContainer iot[node_head];
    NodeContainer br;
    NodeContainer master;
    NodeContainer routers;
    NodeContainer endnodes;
    NodeContainer csmam[node_head];
    NodeContainer backhaul;

    //Brite
    BriteTopologyHelper bth(std::string("src/brite/examples/conf_files/TD_ASBarabasi_RTWaxman.conf"));
    bth.AssignStreams(3);
    backhaul = bth.BuildBriteTopology2();

    br.Create(node_head);
    master.Create(1);
    //routers.Add(master);
    routers.Add(backhaul);
    routers.Add(br);

    
    // Create mobility objects for master and (border) routers.
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> routerPositionAlloc = CreateObject<ListPositionAllocator> ();
    Ptr<ListPositionAllocator> masterPositionAlloc = CreateObject<ListPositionAllocator> ();

    // Create NetDeviceContainers for LrWpan, 6lowpan and CSMA (LAN).
    NetDeviceContainer LrWpanDevCon[node_head];
    NetDeviceContainer six[node_head];
    NetDeviceContainer master_csma[node_head];

    for (int jdx = 0; jdx < node_head; jdx++) {
        //Add BR and master to csma-NodeContainers
        csmam[jdx].Add(br.Get(jdx));
        csmam[jdx].Add(backhaul.Get(jdx));
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

        routerPositionAlloc->Add(Vector(-495 + jdx * dist, 395, 0.0));
        masterPositionAlloc->Add(Vector(0, 200, 0.0));
    }

    mobility.SetPositionAllocator(routerPositionAlloc);
    mobility.Install(br);
    mobility.SetPositionAllocator(masterPositionAlloc);
    mobility.Install(master);

    NS_LOG_INFO("Create channels.");

    //Create and install CSMA and LrWpan channels.
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    LrWpanHelper lrWpanHelper;

    for (int jdx = 0; jdx < node_head; jdx++) {
        master_csma[jdx] = csma.Install(csmam[jdx]);
        LrWpanDevCon[jdx] = lrWpanHelper.Install(iot[jdx]);
        //lrWpanHelper.AssociateToPan(LrWpanDevCon[jdx], jdx + 10);
        lrWpanHelper.AssociateToPan(LrWpanDevCon[jdx], 10);
    }

    //BRITE
    Ipv6AddressHelper ipv66;

    ipv66.SetBase(Ipv6Address("2001:1337::"), Ipv6Prefix(64));

    // Install IPv6 stack
    NS_LOG_INFO("Install Internet stack.");
    RipNgHelper ripNgRouting;
    Ipv6ListRoutingHelper listRH;
    listRH.Add(ripNgRouting, 0);

    InternetStackHelper internetv6;
    internetv6.SetIpv4StackInstall(false);
    internetv6.Install(endnodes);
    internetv6.SetRoutingHelper(listRH);
    internetv6.Install(routers);


    // Install 6LowPan layer
    NS_LOG_INFO("Install 6LoWPAN.");
    SixLowPanHelper sixlowpan;
    Ipv6AddressHelper ipv6;
    Ipv6InterfaceContainer i[node_head];
    Ipv6InterfaceContainer i_csma[node_head];

    // Assign IPv6 addresses
    NS_LOG_INFO("Assign addresses.");
    for (int jdx = 0; jdx < node_head; jdx++) {
        std::string addresss;
        six[jdx] = sixlowpan.Install(LrWpanDevCon[jdx]);
        subn++;
        addresss = "2001:0:" + std::to_string(subn) + "::";
        const char * c = addresss.c_str();
        ipv6.SetBase(Ipv6Address(c), Ipv6Prefix(64));
        i[jdx] = ipv6.Assign(six[jdx]);
        subn++;
        addresss = "2001:0:" + std::to_string(subn) + "::";
        c = addresss.c_str();
        ipv6.SetBase(Ipv6Address(c), Ipv6Prefix(64));
        i_csma[jdx] = ipv6.Assign(master_csma[jdx]);

        //Set forwarding rules.
        i[jdx].SetDefaultRouteInAllNodes(node_periph);
        i[jdx].SetForwarding(node_periph, true);
        //i_csma[jdx].SetDefaultRouteInAllNodes(1);
        //i_csma[jdx].SetDefaultRouteInAllNodes(0);
        i_csma[jdx].SetForwarding(0, true); //Is this line really needed? since all interfaces are set by i[jdx].Setforwarding...
        i_csma[jdx].SetForwarding(1, true);
    }

    // Static routing rules
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);
    ripNgRouting.PrintRoutingTableEvery(Seconds(1.0), master.Get(0), routingStream);

    // Install and create Ping applications.
    NS_LOG_INFO("Create Applications.");
    uint32_t packetSize = 10;
    uint32_t maxPacketCount = 1000;
    Time interPacketInterval = Seconds(0.1);
    Ping6Helper ping6;

    ping6.SetLocal(i[1].GetAddress(0, 1));
    ping6.SetRemote(i[2].GetAddress(4, 1));

    ping6.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    ping6.SetAttribute("Interval", TimeValue(interPacketInterval));
    ping6.SetAttribute("PacketSize", UintegerValue(packetSize));

    ApplicationContainer apps = ping6.Install(iot[1].Get(0)); //Install app on node 1.
    apps.Start(Seconds(5.0));
    apps.Stop(Seconds(15.0));

    AsciiTraceHelper ascii;
    //lrWpanHelper.EnableAsciiAll(ascii.CreateFileStream("ping6wsn.tr"));
    lrWpanHelper.EnablePcapAll(std::string("./traces/6lowpan/ping6wsn"), true);
    csma.EnablePcapAll(std::string("./traces/csma"), true);
    NS_LOG_INFO("Run Simulation.");
    AnimationInterface anim("AWSNanimation.xml");
    anim.EnablePacketMetadata(true);
    Simulator::Stop(Seconds(20));
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");
}

