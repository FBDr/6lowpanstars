#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/netanim-module.h"
#include <string>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Ping6WsnExample");

int main(int argc, char **argv) {

    bool verbose = false;
    int rngfeed = 43221;
    int node_num = 10;
    int node_periph = 9;
    int node_head = 2;
    int con_per;
    int pro_per;
    int dist = 500;

    CommandLine cmd;
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.AddValue("rng", "Set feed for random number generator", rngfeed);
    cmd.AddValue("nodes", "Number of nodes", node_num);
    cmd.AddValue("periph", "Number of peripheral nodes", node_periph);
    cmd.AddValue("head", "Number of head nodes", node_head);
    cmd.AddValue("ConPercent", "Consumer percentage", con_per);
    cmd.AddValue("ProPercent", "Producer percentage", pro_per);
    cmd.Parse(argc, argv);

    // Set seed for random numbers
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(rngfeed);

    if (verbose) {
        LogComponentEnable("Ping6WsnExample", LOG_LEVEL_INFO);
        //LogComponentEnable("Ipv6StaticRouting", LOG_LEVEL_ALL);
        //LogComponentEnable("Ipv6ListRouting", LOG_LEVEL_ALL);
        LogComponentEnable("Icmpv6L4Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("Ping6Application", LOG_LEVEL_ALL);
    }

    NS_LOG_INFO("Creating IoT bubbles.");
    NodeContainer iot[node_head];
    NodeContainer br;
    NodeContainer master;
    NodeContainer csmam[node_head];
    br.Create(node_head);
    master.Create(1);

    for (int jdx = 0; jdx < node_head; jdx++) {
        csmam[jdx] = br[jdx];
        csmam[jdx].Add(master.Get(0));
    }

    //Netdevice aanmaken voor masternode?
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> routerPositionAlloc = CreateObject<ListPositionAllocator> ();
    Ptr<ListPositionAllocator> masterPositionAlloc = CreateObject<ListPositionAllocator> ();

    NetDeviceContainer LrWpanDevCon[node_head];
    NetDeviceContainer six[node_head];
    NetDeviceContainer master_csma[node_head];

    for (int jdx = 0; jdx < node_head; jdx++) {
        iot[jdx].Create(node_periph);
        iot[jdx].Add(br.Get(jdx));



        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                "MinX", DoubleValue(jdx * dist),
                "MinY", DoubleValue(0.0),
                "DeltaX", DoubleValue(5),
                "DeltaY", DoubleValue(5),
                "GridWidth", UintegerValue(3),
                "LayoutType", StringValue("RowFirst"));
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

        mobility.Install(iot[jdx]);

        routerPositionAlloc->Add(Vector(5.0 + jdx * dist, -5.0, 0.0));
        masterPositionAlloc->Add(Vector(500, -200, 0.0));
    }

    mobility.SetPositionAllocator(routerPositionAlloc);
    mobility.Install(br);
    mobility.SetPositionAllocator(masterPositionAlloc);
    mobility.Install(master);

    NS_LOG_INFO("Create channels.");

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    for (int jdx = 0; jdx < node_head; jdx++) {
        master_csma[jdx] = csma.Install(csmam[jdx]);
    }


    LrWpanHelper lrWpanHelper;
    for (int jdx = 0; jdx < node_head; jdx++) {
        LrWpanDevCon[jdx] = lrWpanHelper.Install(iot[jdx]);
        lrWpanHelper.AssociateToPan(LrWpanDevCon[jdx], jdx + 10);

    }

    /* Install IPv4/IPv6 stack */
    NS_LOG_INFO("Install Internet stack.");
    InternetStackHelper internetv6;
    internetv6.SetIpv4StackInstall(false);
    internetv6.InstallAll();

    // Install 6LowPan layer
    NS_LOG_INFO("Install 6LoWPAN.");
    SixLowPanHelper sixlowpan;
    Ipv6AddressHelper ipv6;
    Ipv6InterfaceContainer i[node_head];
    Ipv6InterfaceContainer i_csma[node_head];

    ipv6.SetBase(Ipv6Address("2001:99::"), Ipv6Prefix(64));
    for (int jdx = 0; jdx < node_head; jdx++) {
        i_csma[jdx] = ipv6.Assign(master_csma[jdx]);
    }

    for (int jdx = 0; jdx < node_head; jdx++) {

        //Assign IPv6 addresses
        std::string addresss;
        six[jdx] = sixlowpan.Install(LrWpanDevCon[jdx]);
        NS_LOG_INFO("Assign addresses.");
        addresss = "2001:" + std::to_string(jdx + 1) + "::";
        const char * c = addresss.c_str();
        std::cout << c << std::endl;
        ipv6.SetBase(Ipv6Address(c), Ipv6Prefix(64));
        i[jdx] = ipv6.Assign(six[jdx]);
        //Set forwarding and routing rules.
/*
        i_csma[jdx].SetForwarding(jdx, true);
        i[jdx].SetDefaultRouteInAllNodes(node_periph);
        i[jdx].SetForwarding(node_periph, true);
        i_csma[jdx].SetDefaultRouteInAllNodes(jdx);
*/
    }

    NS_LOG_INFO("Create Applications.");

    /* Create a Ping6 application to send ICMPv6 echo request from node zero to
     * all-nodes (ff02::1).
     */
    uint32_t packetSize = 10;
    uint32_t maxPacketCount = 1000;
    Time interPacketInterval = Seconds(0.5);
    Ping6Helper ping6;

    ping6.SetLocal(i[0].GetAddress(0, 1));
    ping6.SetRemote(i[1].GetAddress(1, 1));
    // ping6.SetRemote (Ipv6Address::GetAllNodesMulticast ());

    ping6.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    ping6.SetAttribute("Interval", TimeValue(interPacketInterval));
    ping6.SetAttribute("PacketSize", UintegerValue(packetSize));
    ApplicationContainer apps = ping6.Install(iot[0].Get(0));
    apps.Start(Seconds(2.0));
    apps.Stop(Seconds(10.0));

    AsciiTraceHelper ascii;
    //lrWpanHelper.EnableAsciiAll(ascii.CreateFileStream("ping6wsn.tr"));
    lrWpanHelper.EnablePcapAll(std::string("./traces/ping6wsn"), true);

    NS_LOG_INFO("Run Simulation.");
    AnimationInterface anim("AWSNanimation.xml");
    anim.EnablePacketMetadata(true);
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");
}

