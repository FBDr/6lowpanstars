/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ip4iot.h
 * Author: floris
 *
 * Created on August 7, 2017, 11:08 AM
 */

#include "stacks_header.h"
#include "src/network/utils/ipv6-address.h"
#include "src/applications/helper/coap-helper.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("ip-stack");

    void sixlowpan_stack(int &node_periph, int &node_head, int &totnumcontents, BriteTopologyHelper &bth,
            NetDeviceContainer LrWpanDevice[], NetDeviceContainer SixLowpanDevice[], NetDeviceContainer CSMADevice[],
            Ipv6InterfaceContainer i_6lowpan[], Ipv6InterfaceContainer i_csma[],
            std::vector<Ipv6Address> &IPv6Bucket, std::vector<Ipv6Address> &AddrResBucket,
            NodeContainer &endnodes, NodeContainer &br, NodeContainer & backhaul, std::map<Ipv6Address, Ipv6Address> &gtw_to_node_map) {

        //This function installs 6LowPAN stack on nodes if IP is selected as networking protocol.

        int subn = 0;
        RipNgHelper ripNgRouting;
        ripNgRouting.Set("UnsolicitedRoutingUpdate", TimeValue(Seconds(15)));
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
        // Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("Routing table.txt", ios_base::trunc);
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
            i_csma[jdx].SetForwarding(0, true);
            i_csma[jdx].SetForwarding(1, true);

        }

        //Create IPv6AddrResBucket.
        for (int idx = 0; idx < node_head; idx++) {
            for (int jdx = 0; jdx < node_periph; jdx++) {
                IPv6Bucket.push_back(i_6lowpan[idx].GetAddress(jdx, 1));
                gtw_to_node_map.insert(std::make_pair(i_6lowpan[idx].GetAddress(jdx, 1), i_6lowpan[idx].GetAddress(node_periph, 1)));
                NS_LOG_INFO("Node: " << i_6lowpan[idx].GetAddress(jdx, 1) << " at GTW: " << gtw_to_node_map[i_6lowpan[idx].GetAddress(jdx, 1)]);
                NS_LOG_INFO("Map currently is of size: " << gtw_to_node_map.size());
            }
        }
        //AddrResBucket = CreateAddrResBucket(IPv6Bucket, node_periph);
        AddrResBucket = IPv6Bucket;
        //Shuffelen
        Ptr<UniformRandomVariable> Rpro = CreateObject<UniformRandomVariable> ();
        Rpro->SetStream(6);
        shuffle_array_ip(AddrResBucket, Rpro, 6, node_head * node_periph);
    }

    void sixlowpan_apps(int &node_periph, int &node_head, NodeContainer iot[], NodeContainer all, NodeContainer endnodes,
            std::vector<Ipv6Address> &AddrResBucket, ApplicationContainer &apps,
            Ipv6InterfaceContainer i_6lowpan[], int &simtime, BriteTopologyHelper & briteth, int &payloadsize,
            std::string zm_q, std::string zm_s, int &con_leaf, int &con_inside, int &con_gtw,
            double &min_freq, double &max_freq, bool useIPCache, double freshness, int cache, std::map<Ipv6Address, Ipv6Address> &gtw_to_node_map) {

        //This function installs the specific IP applications. 

        uint32_t maxPacketCount = 99999999; //Optional
        uint16_t port = 9;
        double interval_sel;
        double start_delay; //Prevent nodes from starting at the same time.
        Ptr<UniformRandomVariable> Rinterval = CreateObject<UniformRandomVariable> ();
        Rinterval->SetStream(2);
        Ptr<UniformRandomVariable> Rstartdelay = CreateObject<UniformRandomVariable> (); //Random variable for transmission interval
        Rstartdelay->SetStream(5);
        /*
                Ptr<UniformRandomVariable> Rcon = CreateObject<UniformRandomVariable> ();
                Rcon->SetStream(4);
         */
        CoapClientHelper client(port);
        CoapServerHelper server(port);
        CoapCacheGtwHelper gtw_cache(port);

        //Server
        for (int itr = 0; itr < node_head; itr++) {
            //Install cache gtw application on the gateway.

            if (useIPCache) {
                gtw_cache.SetAttribute("Payload", UintegerValue((uint32_t) payloadsize));
                gtw_cache.SetAttribute("Freshness", UintegerValue((uint32_t) freshness));
                gtw_cache.SetAttribute("CacheSize", UintegerValue((uint32_t) cache));
                apps = gtw_cache.Install(iot[itr].Get(node_periph));
                gtw_cache.SetIPv6Bucket(apps.Get(0), AddrResBucket);
                NS_LOG_INFO("Map currently is of size: " << gtw_to_node_map.size());
                gtw_cache.SetNodeToGtwMap(apps.Get(0), gtw_to_node_map);
                apps.Start(Seconds(1.0));
                apps.Stop(Seconds(simtime));
            }


            for (int jdx = 0; jdx < node_periph; jdx++) {
                //Install server application on every node, in every IoT domain.
                server.SetAttribute("Payload", UintegerValue((uint16_t) payloadsize));
                apps = server.Install(iot[itr].Get(jdx));
                server.SetIPv6Bucket(apps.Get(0), AddrResBucket);
                server.SetNodeToGtwMap(apps.Get(0), gtw_to_node_map);
                //interval_sel = Rstartdelay->GetValue(0.1, 5);
                //apps.Start(Seconds(1.0 + interval_sel));
                apps.Start(Seconds(1.0));
                apps.Stop(Seconds(simtime));

            }
        }

        client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        client.SetAttribute("q", StringValue(zm_q)); // 10 different contents
        client.SetAttribute("s", StringValue(zm_s)); // 10 different contents

        Ptr<UniformRandomVariable> Rinsidenodecon = CreateObject<UniformRandomVariable> ();
        Rinsidenodecon->SetStream(4);


        //Client inside nodes
        for (int jdx = 0; jdx < con_inside; jdx++) {
            interval_sel = Rinterval->GetValue(min_freq, max_freq);
            start_delay = Rstartdelay->GetValue(0.1, 1 / max_freq);
            client.SetAttribute("Interval", TimeValue(Seconds(1.0 / interval_sel))); //Constant frequency ranging from 5 requests per second to 1 request per minute.
            client.SetAttribute("NumberOfContents", UintegerValue(AddrResBucket.size()));
            if (useIPCache) {
                client.SetAttribute("useIPCache", BooleanValue(true));
            }
            Ptr<Node> sel_node = SelectRandomNodeFromContainer(endnodes, Rinsidenodecon);
            client.SetAttribute("RngStream", StringValue(std::to_string(sel_node->GetId())));
            apps = client.Install(sel_node);
            std::cout << "sel_node_inside " << sel_node->GetId() << std::endl;
            client.SetIPv6Bucket(apps.Get(0), AddrResBucket);
            client.SetNodeToGtwMap(apps.Get(0), gtw_to_node_map);
            apps.Start(Seconds(120.0 + start_delay));
            apps.Stop(Seconds(simtime - 5));
        }
    }

}


