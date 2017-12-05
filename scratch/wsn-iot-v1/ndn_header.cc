#include "stacks_header.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("ndn-stack");

    void NDN_stack(int &node_head, int &node_periph, NodeContainer iot[], NodeContainer & backhaul, NodeContainer &endnodes,
            BriteTopologyHelper &bth, int &simtime, int &report_time_cu, int &con_leaf, int &con_inside, int &con_gtw,
            int &cache, double &freshness, bool &ipbackhaul, int &payloadsize, std::string zm_q, std::string zm_s,
            double &min_freq, double &max_freq) {

        //This function installs NDN stack on nodes if ndn is selected as networking protocol.

        //Variables declaration
        std::string prefix = "/SensorData";
        double interval_sel;
        double start_delay; //Prevent nodes from starting at the same time.
        ndn::StackHelper ndnHelper;
        ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
        ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrotV2");
        ndn::AppHelper producerHelper("ns3::ndn::Producer");
        Ptr<UniformRandomVariable> Rinterval = CreateObject<UniformRandomVariable> (); //Random variable for transmission interval
        Rinterval->SetStream(2);
        Ptr<UniformRandomVariable> Rstartdelay = CreateObject<UniformRandomVariable> (); //Random variable for transmission interval
        Rstartdelay->SetStream(5);
        ApplicationContainer apps;

        //Set default route / in FIBs
        ndnHelper.SetDefaultRoutes(true);

        //Scenario specific functions
        if (freshness) {
            ndnHelper.SetOldContentStore("ns3::ndn::cs::Freshness::Lru", "MaxSize", std::to_string(cache), "ReportTime", std::to_string(report_time_cu));
        } else {
            //Then the default CS is being used.
            ndnHelper.setCsSize(cache);
        }

        if (!cache) {
            ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache");
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
            for (int idx = 0; idx < (int) backhaul.GetN(); idx++) {
                Ptr<ndn::L3Protocol> L3Prot = backhaul.Get(idx)->GetObject<ns3::ndn::L3Protocol>();
                L3Prot->setRole(1);
            }

            for (int idx = 0; idx < node_head; idx++) {
                Ptr<ndn::L3Protocol> L3Prot = iot[idx].Get(node_periph)->GetObject<ns3::ndn::L3Protocol>();
                L3Prot->setRole(2);

            }
        }

        //Install strategy and routing objects on all nodes.
        ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/bestroute2");
        ndnGlobalRoutingHelper.InstallAll();

        //Producer install
        std::vector<int> content_chunks(node_periph);
        std::iota(std::begin(content_chunks), std::end(content_chunks), 1);
        Ptr<UniformRandomVariable> Rpro = CreateObject<UniformRandomVariable> ();
        Rpro->SetStream(6);
        shuffle_array(content_chunks, Rpro);

        for (int idx = 0; idx < node_head; idx++) {
            for (int jdx = 0; jdx < node_periph; jdx++) {
                std::string cur_prefix;

                cur_prefix = "/Home_" + std::to_string(idx) + prefix + "/" + std::to_string(content_chunks[jdx] - 1); //-1
                NS_LOG_INFO("Im node: " << iot[idx].Get(jdx)->GetId() << "with prefix: " << content_chunks[jdx] << " " << content_chunks[jdx] - 1);
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
        Ptr<UniformRandomVariable> Rleafnodecon = CreateObject<UniformRandomVariable> ();
        Rleafnodecon->SetStream(3);

        for (int idx = 0; idx < node_head; idx++) {
            for (int jdx = 0; jdx < con_leaf; jdx++) {
                std::string cur_prefix;
                cur_prefix = "/Home_" + std::to_string(idx) + prefix;
                consumerHelper.SetPrefix(cur_prefix);
                interval_sel = Rinterval->GetValue(min_freq, max_freq); //Constant frequency ranging from 5 requests per second to 1 request per minute.
                start_delay = Rstartdelay->GetValue(0.1, 1 / max_freq);
                consumerHelper.SetAttribute("Frequency", StringValue(boost::lexical_cast<std::string>(interval_sel)));
                Ptr<Node> sel_node = SelectRandomLeafNodeConsumer(bth, Rleafnodecon);
                consumerHelper.SetAttribute("RngStream", StringValue(std::to_string(sel_node->GetId()))); //Select for every node an unique stream.
                std::cout << "sel_node_leaf " << sel_node->GetId() << std::endl;
                apps = consumerHelper.Install(sel_node); //Consumers are at leaf nodes.
                if (ipbackhaul) {
                    Ptr<ndn::L3Protocol> L3Prot = sel_node->GetObject<ns3::ndn::L3Protocol>();
                    L3Prot->setRole(2);
                }
                apps.Start(Seconds(120.0 + start_delay));
                apps.Stop(Seconds(simtime - 5));
            }
        }
        Ptr<UniformRandomVariable> Rinsidenodecon = CreateObject<UniformRandomVariable> ();
        Rinsidenodecon->SetStream(4);

        //Consumer inside install
        for (int idx = 0; idx < node_head; idx++) {
            for (int jdx = 0; jdx < con_inside; jdx++) {

                std::string cur_prefix;
                cur_prefix = "/Home_" + std::to_string(idx) + prefix;
                consumerHelper.SetPrefix(cur_prefix);
                interval_sel = Rinterval->GetValue(min_freq, max_freq); //Constant frequency ranging from 5 requests per second to 1 request per minute.
                start_delay = Rstartdelay->GetValue(0.1, 1 / max_freq);
                consumerHelper.SetAttribute("Frequency", StringValue(boost::lexical_cast<std::string>(interval_sel)));
                Ptr<Node> sel_node = SelectRandomNodeFromContainer(iot[idx], Rinsidenodecon);
                consumerHelper.SetAttribute("RngStream", StringValue(std::to_string(sel_node->GetId())));
                apps = consumerHelper.Install(sel_node); //Consumers are at leaf nodes.
                std::cout << "sel_node_inside " << sel_node->GetId() << std::endl;
                apps.Start(Seconds(120.0 + start_delay));
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
                Ptr<Node> sel_node = iot[idx].Get(node_periph);
                interval_sel = Rinterval->GetValue(min_freq, max_freq); //Constant frequency ranging from 5 requests per second to 1 request per minute.
                start_delay = Rstartdelay->GetValue(0.1, 1 / max_freq);
                consumerHelper.SetAttribute("Frequency", StringValue(boost::lexical_cast<std::string>(interval_sel)));
                consumerHelper.SetAttribute("RngStream", StringValue(std::to_string(sel_node->GetId())));
                apps = consumerHelper.Install(sel_node); //Consumers are at leaf nodes.
                apps.Start(Seconds(120.0 + start_delay));
                apps.Stop(Seconds(simtime - 5));
            }
        }



        std::cout << "Filling routing tables..." << std::endl;
        ndn::GlobalRoutingHelper::CalculateRoutes();
        std::cout << "Done, now starting simulator..." << std::endl;
    }
}