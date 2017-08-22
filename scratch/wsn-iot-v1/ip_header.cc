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

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("ip-stack");

    void sixlowpan_stack(int &node_periph, int &node_head, int &totnumcontents, BriteTopologyHelper &bth,
            NetDeviceContainer LrWpanDevice[], NetDeviceContainer SixLowpanDevice[], NetDeviceContainer CSMADevice[],
            Ipv6InterfaceContainer i_6lowpan[], Ipv6InterfaceContainer i_csma[],
            std::vector< std::vector<Ipv6Address> > &IPv6Bucket, std::vector< std::vector<Ipv6Address> > &AddrResBucket,
            NodeContainer &endnodes, NodeContainer &br, NodeContainer & backhaul) {

        //This function installs 6LowPAN stack on nodes if IP is selected as networking protocol.

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

                IPv6Bucket[idx].push_back(i_6lowpan[idx].GetAddress(jdx, 1));
                NS_LOG_INFO("Filling IPv6 bucket " << IPv6Bucket[idx][jdx]);
            }
        }
        AddrResBucket = CreateAddrResBucket(IPv6Bucket, node_periph);
    }

    void sixlowpan_apps(int &node_periph, int &node_head, NodeContainer iot[], NodeContainer all,
            std::vector< std::vector<Ipv6Address> > &AddrResBucket, ApplicationContainer &apps,
            Ipv6InterfaceContainer i_6lowpan[], int &simtime, BriteTopologyHelper & briteth, int &payloadsize,
            std::string zm_q, std::string zm_s, int &con_leaf, int &con_inside, int &con_gtw,
            double &min_freq, double &max_freq) {

        //This function installs the specific IP applications. 

        uint32_t maxPacketCount = 99999999; //Optional
        uint16_t port = 9;
        double interval_sel;
        double start_delay; //Prevent nodes from starting at the same time.
        Ptr<UniformRandomVariable> Rinterval = CreateObject<UniformRandomVariable> ();
        Ptr<UniformRandomVariable> Rcon = CreateObject<UniformRandomVariable> ();

        CoapClientHelper client(port);
        CoapServerHelper server(port);

        //Server
        for (int itr = 0; itr < node_head; itr++) {
            for (int jdx = 0; jdx < node_periph; jdx++) {
                //Install server application on every node, in every IoT domain.
                server.SetAttribute("Payload", UintegerValue((uint16_t) payloadsize));
                apps = server.Install(iot[itr].Get(jdx));
                server.SetIPv6Bucket(apps.Get(0), AddrResBucket[itr]);
                interval_sel = Rinterval->GetValue(0.1, 1);
                apps.Start(Seconds(1.0 + interval_sel));
                apps.Stop(Seconds(simtime));

            }
        }


        // Client leafnode
        client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        client.SetAttribute("q", StringValue(zm_q)); // 10 different contents
        client.SetAttribute("s", StringValue(zm_s)); // 10 different contents

        for (int idx = 0; idx < node_head; idx++) {
            for (int jdx = 0; jdx < con_leaf; jdx++) {
                interval_sel = Rinterval->GetValue(min_freq, max_freq);
                start_delay = Rinterval->GetValue(0.1, 5.0);
                client.SetAttribute("Interval", TimeValue(Seconds(1.0 / interval_sel))); //Constant frequency ranging from 5 requests per second to 1 request per minute.
                client.SetAttribute("NumberOfContents", UintegerValue(AddrResBucket[idx].size()));
                apps = client.Install(SelectRandomLeafNode(briteth));
                client.SetIPv6Bucket(apps.Get(0), AddrResBucket[idx]);
                NS_LOG_INFO("Size of generated bucket: " << AddrResBucket[idx].size());
                apps.Start(Seconds(120.0 + start_delay));
                apps.Stop(Seconds(simtime - 5));
            }
        }

        //Client inside nodes
        for (int idx = 0; idx < node_head; idx++) {
            for (int jdx = 0; jdx < con_inside; jdx++) {
                interval_sel = Rinterval->GetValue(min_freq, max_freq);
                client.SetAttribute("Interval", TimeValue(Seconds(1.0 / interval_sel))); //Constant frequency ranging from 5 requests per second to 1 request per minute.
                client.SetAttribute("NumberOfContents", UintegerValue(AddrResBucket[idx].size()));
                apps = client.Install(SelectRandomNodeFromContainer(iot[idx]));
                client.SetIPv6Bucket(apps.Get(0), AddrResBucket[idx]);
                apps.Start(Seconds(120.0 + interval_sel));
                apps.Stop(Seconds(simtime - 5));
            }
        }


        //Client gateway nodes
        for (int idx = 0; idx < node_head; idx++) {
            for (int jdx = 0; jdx < con_gtw; jdx++) {
                interval_sel = Rinterval->GetValue(min_freq, max_freq);
                client.SetAttribute("Interval", TimeValue(Seconds(1.0 / interval_sel))); //Constant frequency ranging from 5 requests per second to 1 request per minute.
                client.SetAttribute("NumberOfContents", UintegerValue(AddrResBucket[idx].size()));
                apps = client.Install(iot[idx].Get(node_periph));
                client.SetIPv6Bucket(apps.Get(0), AddrResBucket[idx]);
                apps.Start(Seconds(120.0 + interval_sel));
                apps.Stop(Seconds(simtime - 5));
            }
        }
    }
}


