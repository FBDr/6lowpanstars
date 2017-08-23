#include "stacks_header.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("ndn-stack");

    void NDN_stack(int &node_head, int &node_periph, NodeContainer iot[], NodeContainer & backhaul, NodeContainer &endnodes,
            BriteTopologyHelper &bth, int &simtime, int &con_leaf, int &con_inside, int &con_gtw,
            int &cache, double &freshness, bool &ipbackhaul, int &payloadsize, std::string zm_q, std::string zm_s,
            double &min_freq, double &max_freq) {

        //This function installs NDN stack on nodes if ndn is selected as networking protocol.

        //Variables declaration
        std::string prefix = "/SensorData";
        double interval_sel;
        ndn::StackHelper ndnHelper;
        ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
        ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrotV2");
        ndn::AppHelper producerHelper("ns3::ndn::Producer");
        Ptr<UniformRandomVariable> Rinterval = CreateObject<UniformRandomVariable> (); //Random variable for transmission interval
        ApplicationContainer apps;

        //Set default route / in FIBs
        ndnHelper.SetDefaultRoutes(true);

        //Scenario specific functions
        if (!cache) {
            ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache");
        }
        if (freshness) {
            ndnHelper.SetOldContentStore("ns3::ndn::cs::Freshness::Lru", "MaxSize", std::to_string(cache));
        } else {
            //Then the default CS is being used.
            ndnHelper.setCsSize(cache);
        }

        // Install NDN stack on iot endnodes
        for (int jdx = 0; jdx < node_head; jdx++) {
            ndnHelper.Install(iot[jdx]);
        }

        //Install NDN stack on backhaul nodes
        if (ipbackhaul) {
            ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache"); //We don't want caching in an IP based backhaul network.  
        }
        ndnHelper.Install(backhaul);

        if (ipbackhaul) {
            for (int idx = 0; idx < node_head; idx++) {
                Ptr<ndn::L3Protocol> L3Prot = iot[idx].Get(node_periph)->GetObject<ns3::ndn::L3Protocol>();
                std::cout << "Default value: " << L3Prot->getGTW() << std::endl;
                L3Prot->setGTW(true);
                std::cout << "Installed IP GTW= " << L3Prot->getGTW() << " on node: " << iot[idx].Get(node_periph)->GetId() << std::endl;
            }
        }

        //Install strategy and routing objects on all nodes.
        ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/bestroute2");
        ndnGlobalRoutingHelper.InstallAll();

        //Producer install
        std::vector<int> content_chunks(node_periph);
        std::iota(std::begin(content_chunks), std::end(content_chunks), 1);
        shuffle_array(content_chunks);
        shuffle_array(content_chunks);

        for (int idx = 0; idx < node_head; idx++) {
            for (int jdx = 0; jdx < node_periph; jdx++) {
                std::string cur_prefix;
                cur_prefix = "/Home_" + std::to_string(idx) + prefix + "/" + std::to_string(content_chunks[jdx]);
                producerHelper.SetPrefix(cur_prefix);
                producerHelper.SetAttribute("PayloadSize", StringValue(boost::lexical_cast<std::string>(payloadsize))); //Should we make this random?
                producerHelper.SetAttribute("Freshness", TimeValue(Seconds(freshness)));
                producerHelper.Install(iot[idx].Get(jdx)); // All producers have at least one data packet.
                ndnGlobalRoutingHelper.AddOrigin(cur_prefix, iot[idx].Get(jdx)); //Install routing entry for every node in FIB
            }
        }

        //Consumer leafnode install
        consumerHelper.SetAttribute("q", StringValue(zm_q));
        consumerHelper.SetAttribute("s", StringValue(zm_s));
        consumerHelper.SetAttribute("NumberOfContents", StringValue(std::to_string(node_periph)));

        for (int idx = 0; idx < node_head; idx++) {
            for (int jdx = 0; jdx < con_leaf; jdx++) {
                std::string cur_prefix;
                cur_prefix = "/Home_" + std::to_string(idx) + prefix;
                consumerHelper.SetPrefix(cur_prefix);
                interval_sel = Rinterval->GetValue(min_freq, max_freq); //Constant frequency ranging from 5 requests per second to 1 request per minute.
                consumerHelper.SetAttribute("Frequency", StringValue(boost::lexical_cast<std::string>(interval_sel)));
                apps = consumerHelper.Install(SelectRandomLeafNode(bth)); //Consumers are at leaf nodes.
                apps.Start(Seconds(120.0 + interval_sel));
                apps.Stop(Seconds(simtime - 5));
            }
        }
        //Consumer inside install
        for (int idx = 0; idx < node_head; idx++) {
            for (int jdx = 0; jdx < con_inside; jdx++) {

                std::string cur_prefix;
                cur_prefix = "/Home_" + std::to_string(idx) + prefix;
                consumerHelper.SetPrefix(cur_prefix);
                interval_sel = Rinterval->GetValue(min_freq, max_freq); //Constant frequency ranging from 5 requests per second to 1 request per minute.
                consumerHelper.SetAttribute("Frequency", StringValue(boost::lexical_cast<std::string>(interval_sel)));
                apps = consumerHelper.Install(SelectRandomNodeFromContainer(iot[idx])); //Consumers are at leaf nodes.
                apps.Start(Seconds(120.0 + interval_sel));
                apps.Stop(Seconds(simtime - 5));
            }
        }

        //Consumer gtw install
        for (int idx = 0; idx < node_head; idx++) {
            for (int jdx = 0; jdx < con_gtw; jdx++) {

                std::string cur_prefix;
                cur_prefix = "/Home_" + std::to_string(idx) + prefix;
                NS_LOG_INFO("Setting prefix to: " << cur_prefix);
                consumerHelper.SetPrefix(cur_prefix);
                interval_sel = Rinterval->GetValue(min_freq, max_freq); //Constant frequency ranging from 5 requests per second to 1 request per minute.
                consumerHelper.SetAttribute("Frequency", StringValue(boost::lexical_cast<std::string>(interval_sel)));
                apps = consumerHelper.Install((iot[idx].Get(node_periph))); //Consumers are at leaf nodes.
                apps.Start(Seconds(120.0 + interval_sel));
                apps.Stop(Seconds(simtime - 5));
            }
        }



        std::cout << "Filling routing tables..." << std::endl;
        ndn::GlobalRoutingHelper::CalculateRoutes();
        std::cout << "Done, now starting simulator..." << std::endl;
    }
}