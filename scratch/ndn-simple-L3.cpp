// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {

    int
    main(int argc, char* argv[]) {
        // setting default parameters for PointToPoint links and channels
        Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
        Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
        Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

        // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
        CommandLine cmd;
        cmd.Parse(argc, argv);

        // Creating nodes
        NodeContainer nodes;
        nodes.Create(3);
        NetDeviceContainer netcon;
        netcon.Add(nodes);
        // Connecting nodes using two links
        PointToPointHelper p2p;
        p2p.Install(nodes.Get(0), nodes.Get(1));
        p2p.Install(nodes.Get(1), nodes.Get(2));

        InternetStackHelper internetv6;
        internetv6.SetIpv4StackInstall(false);
        internetv6.Install(nodes);
        Ipv6InterfaceContainer i_faces;
        Ipv6AddressHelper ipv6;
        std::string addresss;
        addresss = "2001:0:" + std::to_string(1337) + "::";
        const char * c = addresss.c_str();
        ipv6.SetBase(Ipv6Address(c), Ipv6Prefix(64));
        i_faces = ipv6.Assign(netcon);


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
        producerHelper.Install(nodes.Get(2)); // last node

        Simulator::Stop(Seconds(20.0));

        Simulator::Run();
        Simulator::Destroy();

        return 0;
    }

} // namespace ns3

int
main(int argc, char* argv[]) {
    return ns3::main(argc, argv);
}