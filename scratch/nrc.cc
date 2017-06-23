/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/brite-module.h"
//#include "ns3/log.h"

namespace ns3 {

    void
    shuffle_array(std::vector<int>& arrayf) {
        Ptr<UniformRandomVariable> shuffles = CreateObject<UniformRandomVariable> ();
        shuffles->SetAttribute("Min", DoubleValue(2));
        shuffles->SetAttribute("Max", DoubleValue(20));

        for (int cnt = 0; cnt < (int) (shuffles->GetValue()); cnt++) {

            std::random_shuffle(arrayf.begin(), arrayf.end());
        }
    };
}




namespace ns3 {
//
//    class PcapWriter {
//    public:
//
//        PcapWriter(const std::string& file) {
//            PcapHelper helper;
//            m_pcap = helper.CreateFile(file, std::ios::out, PcapHelper::DLT_PPP);
//        }
//
//        void
//        TracePacket(Ptr<const Packet> packet) {
//            static PppHeader pppHeader;
//            pppHeader.SetProtocol(0x0077);
//
//            m_pcap->Write(Simulator::Now(), pppHeader, packet);
//        }
//
//    private:
//        Ptr<PcapFileWrapper> m_pcap;
//    };

    int
    main(int argc, char* argv[]) {
        // setting default parameters for PointToPoint links and channels
        Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("10Mbps"));
        Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("5ms"));
        Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("1000"));

        //Variable initialization
        int numPro = 1;
        int numCon = 1;
        int curr = 0;
        int range_input = 0;
        int ad_num_contents = 0;
        bool ba = true;
        int pktsize = 1024;
        int aval = 50;
        int avalangle = 0;
        int nrc_mix = 50;
        int nrc_num = 0;
        bool sillyseq = false;
        int stoptime= 10;
        int time_ran =1;
        std::string preamble_prefix = "/NL/TNO/NRCTestbed/VROrientation";
        std::string cur_prefix;


        //Read-in commandline parameters.
        CommandLine cmd;
        cmd.AddValue("seq", "For RNC seq = 0 , for sequential guessing seq = 1", sillyseq);
        cmd.AddValue("nrcmix", "Percentage of NRC consumers", nrc_mix);
        cmd.AddValue("numproducer", "The number of producers", numPro);
        cmd.AddValue("numconsumer", "The number of consumers", numCon);
        cmd.AddValue("range", "The number of consumers", range_input);
        cmd.AddValue("topo", "Set as '1' for Barabasi–Albert or '0' for simple grid", ba);
        cmd.AddValue("datasize", "Set the size of content in data message. In bytes.", pktsize);
        cmd.AddValue("availability", "Set the availability of data. 100% ==> 360 degrees available.", aval);
        cmd.AddValue("stoptime", "Set the simulation duration", stoptime);
        cmd.AddValue("rng", "Set the rng feed.", time_ran);
        cmd.Parse(argc, argv);

        //Print command-line parameters
        std::cout << "The following parameters will be used:" << std::endl;
        std::cout << "Use RNC or sequential interests: " << ((sillyseq) ? "Sequential interest guessing" : "RNC!") << std::endl;
        std::cout << "Percentage of range requesting consumers: " << nrc_mix << std::endl;
        std::cout << "Number of consumers: " << numCon << std::endl;
        std::cout << "Number of producers: " << numPro << std::endl;
        std::cout << "Data packet size: " << pktsize << " bytes" << std::endl;
        std::cout << "Topology: " << ((ba) ? "Barabasi–Albert" : "Grid") << std::endl;
        std::cout << "Availability: " << aval << "%" << std::endl;
        std::cout << "Range: " << range_input << std::endl;
        std::cout << "Stoptime: " << stoptime << std::endl;
        std::cout << "RNG feed: " << time_ran << std::endl << std::endl;
        

        //Seeding RNG
        //time_t time_ran;
        //time(&time_ran);
        RngSeedManager::SetSeed(1);
        RngSeedManager::SetRun(time_ran);

        //Create random variable for interest frequency
        Ptr<UniformRandomVariable> freqvar = CreateObject<UniformRandomVariable> ();
        freqvar->SetAttribute("Min", DoubleValue(10));
        freqvar->SetAttribute("Max", DoubleValue(100));

        //BRITE
        std::string confFile = "./RTBarabasi20.conf";
        BriteTopologyHelper bth(confFile);
        bth.AssignStreams(34);


        // Connecting nodes using two links
        PointToPointHelper p2p;

        // Build BRITE topology and return nodecontainer.
        NodeContainer nodeset = bth.BuildBriteTopologyNDN();

        //Random variables
        //Create random variable for selecting consumer and producer nodes randomly.
        std::vector<int> ProConArray(bth.GetNNodesTopology());
        std::iota(std::begin(ProConArray), std::end(ProConArray), 0);
        shuffle_array(ProConArray);

        std::vector<int> AvaArray(360);
        std::iota(std::begin(AvaArray), std::end(AvaArray), 1);
        shuffle_array(AvaArray);


        //Install NDN-stack
        ndn::StackHelper ndnHelper;
        ndnHelper.Install(nodeset);
        ndn::StrategyChoiceHelper::Install(nodeset, "/", "/localhost/nfd/strategy/bestroute");
        ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
        ndnGlobalRoutingHelper.Install(nodeset);

        // Installing applications
        //Consumer Baseline 
        ad_num_contents = 360 - range_input;
        std::cout << "ad_num_contents: " << ad_num_contents << std::endl;
        std::cout << "Number of nodes in topology: " << bth.GetNNodesTopology() << std::endl;
        nrc_num = (int) (numCon * (float) nrc_mix / 100);
        std::cout << "Number of NRC nodes: " << nrc_num << std::endl;

        //Standard interests + NRC interests
        ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrotNativeNrc");

        //Sequential interests
        ndn::AppHelper consumerHelperR("ns3::ndn::ConsumerZipfMandelbrotFloodNrc");

        //Assign consumer applications to random nodes.
        for (int idx = 0; idx < numCon; idx++) {
            curr = ProConArray[idx];
            if (idx < nrc_num) {
                if (sillyseq) {
                    consumerHelperR.SetAttribute("Frequency", StringValue(boost::lexical_cast<std::string>(freqvar->GetValue()))); // 10 interests a second
                    consumerHelperR.SetPrefix(preamble_prefix);
                    consumerHelperR.SetAttribute("NumberOfContents", StringValue(std::to_string(ad_num_contents))); // 10 different contents
                    consumerHelperR.SetAttribute("range", UintegerValue(range_input));
                    consumerHelperR.Install(bth.GetNodeForAs(0, curr)); // first node
                    std::cout << "Installed silly sequential range consumer at node:" << curr << std::endl;
                } else {
                    consumerHelper.SetAttribute("Frequency", StringValue(boost::lexical_cast<std::string>(freqvar->GetValue()))); // 10 interests a second
                    consumerHelper.SetPrefix(preamble_prefix);
                    consumerHelper.SetAttribute("NumberOfContents", StringValue(std::to_string(ad_num_contents))); // 10 different contents
                    consumerHelper.SetAttribute("range", UintegerValue(range_input));
                    consumerHelper.Install(bth.GetNodeForAs(0, curr)); // first node
                    std::cout << "Installed NRC consumer at node: " << curr << std::endl;
                }

            } else {
                consumerHelper.SetAttribute("Frequency", StringValue(boost::lexical_cast<std::string>(freqvar->GetValue()))); // 10 interests a second
                consumerHelper.SetPrefix(preamble_prefix);
                consumerHelper.SetAttribute("NumberOfContents", StringValue(std::to_string(ad_num_contents))); // 10 different contents
                consumerHelper.SetAttribute("range", UintegerValue(0));
                consumerHelper.Install(bth.GetNodeForAs(0, curr)); // first node
                std::cout << "Installed normal consumer at node: " << curr << std::endl;
            }
        }

        // Producer
        ndn::AppHelper producerHelper("ns3::ndn::Producer");
        producerHelper.SetAttribute("PayloadSize", StringValue(std::to_string(pktsize)));
        ProConArray.erase(ProConArray.begin(), ProConArray.begin()+numCon);

        //Availability
        avalangle = (int) (360 * (float) aval / 100);
        std::cout << "Available angles:" << avalangle << std::endl;
        //Assign producer applications to random nodes.


        if (numPro >= avalangle) {
            //Number of producers outnumber the available chunks
            for (int idx = 0; idx < numPro; idx++) {
                curr = ProConArray[idx];
                cur_prefix = preamble_prefix + "/" + std::to_string(AvaArray[idx % avalangle]);
                producerHelper.SetPrefix(cur_prefix);
                producerHelper.Install(bth.GetNodeForAs(0, curr));
                std::cout << "Producer generated at node:" << curr << std::endl;
                ndnGlobalRoutingHelper.AddOrigins(cur_prefix, bth.GetNodeForAs(0, curr));
                std::cout << "Prefix for this producer: " << cur_prefix << std::endl;
            }
        } else {
            //More chunks than producers
            for (int idx = 0; idx < avalangle; idx++) {
                curr = ProConArray[idx % numPro];
                cur_prefix = preamble_prefix + "/" + std::to_string(AvaArray[idx]);
                producerHelper.SetPrefix(cur_prefix);
                producerHelper.Install(bth.GetNodeForAs(0, curr));
                std::cout << "*Producer generated at node:" << curr << std::endl;
                ndnGlobalRoutingHelper.AddOrigins(cur_prefix, bth.GetNodeForAs(0, curr));
                std::cout << "*Prefix for this producer: " << cur_prefix << std::endl;

            }
        }

        //Routing

        std::cout << "Filling routing tables..." << std::endl;
        ndn::GlobalRoutingHelper::CalculateRoutes();
        std::cout << "Done, now starting simulator..." << std::endl;

        Simulator::Stop(Seconds(stoptime));
        ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");
        //L2RateTracer::InstallAll("drop-trace.txt", Seconds(0.5));
        //PcapWriter trace("ndn-simple-trace.pcap");
        //AsciiTraceHelper ascii;
        //p2p.EnableAsciiAll (ascii.CreateFileStream ("myfirst.tr"));
        //Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacTx",
        //        MakeCallback(&PcapWriter::TracePacket, &trace));

        //p2p.EnablePcapAll(std::string("rnc"), true);
        //ndn::L3RateTracer::InstallAll("rate-trace.txt", Seconds(0.5));
        Simulator::Run();
        Simulator::Destroy();

        return 0;
    }

} // namespace ns3

int
main(int argc, char* argv[]) {
    //LogComponentEnable("ndn.Producer", LOG_LEVEL_FUNCTION);
    return ns3::main(argc, argv);
}




