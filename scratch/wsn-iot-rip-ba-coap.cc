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
#include <vector>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Ping6WsnExample");


std::vector<Ipv6Address> CreateAddrResBucket(std::vector<Ipv6Address>& arrayf, int numContents) {
    std::vector<Ipv6Address> returnBucket;


    Ptr<UniformRandomVariable> shuffles = CreateObject<UniformRandomVariable> ();
    shuffles->SetAttribute("Min", DoubleValue(0));
    shuffles->SetAttribute("Max", DoubleValue(arrayf.size()-1));


    for (int itx = 0; itx < numContents; itx++) {
            returnBucket.push_back(arrayf[shuffles->GetValue()]);
            //std::cout<<arrayf[shuffles->GetValue()]<<std::endl;
            //returnBucket.push_back(arrayf[1]);

    }
    
    return returnBucket;
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

    //ICMPv6 redirects do not optimise routes in single router case.
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
    backhaul = bth.BuildBriteTopology2();

    br.Create(node_head);
    routers.Add(backhaul);
    routers.Add(br);


    // Create mobility objects for master and (border) routers.
    MobilityHelper mobility;
    MobilityHelper mobility_backhaul;
    mobility_backhaul.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility_backhaul.SetPositionAllocator("ns3::GridPositionAllocator",
            "MinX", DoubleValue(0),
            "MinY", DoubleValue(0),
            "DeltaX", DoubleValue(10),
            "DeltaY", DoubleValue(10),
            "GridWidth", UintegerValue(20),
            "LayoutType", StringValue("RowFirst"));
    mobility_backhaul.Install(backhaul);

    // Create NetDeviceContainers for LrWpan, 6lowpan and CSMA (LAN).
    NetDeviceContainer LrWpanDevice[node_head];
    NetDeviceContainer SixLowpanDevice[node_head];
    NetDeviceContainer CSMADevice[node_head];
    Ptr<ListPositionAllocator> BorderRouterPositionAlloc = CreateObject<ListPositionAllocator> ();

    for (int jdx = 0; jdx < node_head; jdx++) {
        //Add BR and master to csma-NodeContainers
        border_backhaul[jdx].Add(br.Get(jdx));
        border_backhaul[jdx].Add(backhaul.Get(jdx + 5));
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
    LrWpanHelper lrWpanHelper;

    for (int jdx = 0; jdx < node_head; jdx++) {
        CSMADevice[jdx] = csma.Install(border_backhaul[jdx]);
        LrWpanDevice[jdx] = lrWpanHelper.Install(iot[jdx]);
        //lrWpanHelper.AssociateToPan(LrWpanDevCon[jdx], jdx + 10);
        lrWpanHelper.AssociateToPan(LrWpanDevice[jdx], 10);
    }

    //BRITE
    // Install IPv6 stack
    NS_LOG_INFO("Install Internet stack.");
    RipNgHelper ripNgRouting;
    Ipv6ListRoutingHelper listRH;
    listRH.Add(ripNgRouting, 0);

    InternetStackHelper internetv6;
    internetv6.SetIpv4StackInstall(false);
    internetv6.Install(endnodes);
    internetv6.SetRoutingHelper(listRH);
    internetv6.Install(br);
    internetv6.Install(backhaul);

    // Install 6LowPan layer
    NS_LOG_INFO("Install 6LoWPAN.");
    SixLowPanHelper sixlowpan;
    Ipv6AddressHelper ipv6;
    Ipv6InterfaceContainer i_6lowpan[node_head];
    Ipv6InterfaceContainer i_csma[node_head];
    Ipv6InterfaceContainer i_backhaul;

    // Assign IPv6 addresses
    // For backhaul network
    std::string addresss;
    addresss = "2001:0:" + std::to_string(1337) + "::";
    const char * c = addresss.c_str();
    ipv6.SetBase(Ipv6Address(c), Ipv6Prefix(64));
    bth.AssignIpv6Addresses(ipv6);

    //For outer network
    NS_LOG_INFO("Assign addresses.");
    for (int jdx = 0; jdx < node_head; jdx++) {
        SixLowpanDevice[jdx] = sixlowpan.Install(LrWpanDevice[jdx]);
        subn++;
        addresss = "2001:0:" + std::to_string(subn) + "::";
        const char * c = addresss.c_str();
        ipv6.SetBase(Ipv6Address(c), Ipv6Prefix(64));
        i_6lowpan[jdx] = ipv6.Assign(SixLowpanDevice[jdx]);
        subn++;
        addresss = "2001:0:" + std::to_string(subn) + "::";
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
    std::vector<Ipv6Address> IPv6Bucket;
    for (int idx = 0; idx < node_head; idx++) {
        for (int jdx = 0; jdx < node_periph; jdx++) {
            IPv6Bucket.push_back(i_6lowpan[idx].GetAddress(jdx, 1));
        }
    }

    std::vector<Ipv6Address> test = CreateAddrResBucket(IPv6Bucket, 100);

    // Static routing rules
    //Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);
    //ripNgRouting.PrintRoutingTableEvery(Seconds(1.0), master.Get(0), routingStream);

    // Install and create Ping applications.
    NS_LOG_INFO("Create Applications.");
    uint16_t port = 9;
    CoapServerHelper server(port);
    ApplicationContainer apps = server.Install(iot[1].Get(4));
    server.SetContent(apps.Get(0), 32);
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(10.0));

    uint32_t packetSize = 1024;
    uint32_t maxPacketCount = 20;
    Time interPacketInterval = Seconds(0.2);
    CoapClientHelper client(i_6lowpan[1].GetAddress(4, 1), port);
    client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    client.SetAttribute("Interval", TimeValue(interPacketInterval));
    client.SetAttribute("PacketSize", UintegerValue(packetSize));
    apps = client.Install(iot[1].Get(0));
    apps.Start(Seconds(2.0));
    apps.Stop(Seconds(10.0));

    /*
        CoapClientHelper client2(i_6lowpan[1].GetAddress(4, 1), port);
        client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount*100));
        client.SetAttribute("Interval", TimeValue(Seconds(0.01)));
        client.SetAttribute("PacketSize", UintegerValue(packetSize));
        apps = client2.Install(iot[1].Get(0));
        apps.Start(Seconds(2.0));
        apps.Stop(Seconds(10.0));
     */

    AsciiTraceHelper ascii;
    lrWpanHelper.EnablePcapAll(std::string("./traces/6lowpan/wsn"), true);
    csma.EnablePcapAll(std::string("./traces/csma"), true);
    NS_LOG_INFO("Run Simulation.");
    //AnimationInterface anim("AWSNanimation.xml");
    //anim.EnablePacketMetadata(true);
    Simulator::Stop(Seconds(20));
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");
}

