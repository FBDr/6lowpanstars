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

#include "g_function_header.h"

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("extra-functions");

    Ptr<Node> SelectRandomLeafNode(BriteTopologyHelper & briteth) {

        //Select a random leafnode at the edges of the generated brite network. 
        //This function is used for assigning consumer roles to clients.

        uint32_t totAS = briteth.GetNAs();
        NS_LOG_INFO("The total number of AS is: " << totAS);
        uint32_t leafnum = 0;
        uint32_t selAS = 0;
        uint32_t selLN = 0;
        int max_tries = 5;
        Ptr<UniformRandomVariable> Ras = CreateObject<UniformRandomVariable> ();
        Ptr<UniformRandomVariable> Rleaf = CreateObject<UniformRandomVariable> ();

        for (int tries = 0; tries <= max_tries; tries++) {

            NS_ASSERT_MSG(tries<max_tries, "Tried max_tries to find leafnode. No luck. Check Brite config.");

            selAS = round(Ras->GetValue((double) (0), (double) (totAS - 1)));
            NS_LOG_INFO("Selected AS num: " << selAS);
            leafnum = briteth.GetNLeafNodesForAs(selAS);
            NS_LOG_INFO("Total number of leafnodes in this AS is: " << leafnum);

            if (leafnum) {
                break;
            }
            NS_LOG_WARN("No leafnodes in this AS! Check config! ");
        }

        selLN = round(Rleaf->GetValue((double) (0), (double) (leafnum - 1)));
        NS_LOG_INFO("Selected leafnode num: " << selLN);

        return briteth.GetLeafNodeForAs(selAS, selLN);
    }

    Ptr<Node> SelectRandomNodeFromContainer(NodeContainer container) {
        Ptr<Node> selNode;
        Ptr<UniformRandomVariable> Rnode = CreateObject<UniformRandomVariable> ();
        selNode = container.Get(round(Rnode->GetValue((double) (0), (double) (container.GetN() - 1))));

        return selNode;
    }

    void shuffle_array(std::vector<int>& arrayf) {

        //Shuffles std::vector array a random number of times.

        Ptr<UniformRandomVariable> shuffles = CreateObject<UniformRandomVariable> ();
        shuffles->SetAttribute("Min", DoubleValue(2));
        shuffles->SetAttribute("Max", DoubleValue(20));

        for (int cnt = 0; cnt < (int) (shuffles->GetValue()); cnt++) {

            std::random_shuffle(arrayf.begin(), arrayf.end());
        }
    };

    std::vector< std::vector<Ipv6Address> > CreateAddrResBucket(std::vector< std::vector<Ipv6Address> > &arrayf, int numContentsPerDomain) {

        // Function to generate an address resolution vector, which maps content number (array iterator) to a specific producer.
        // (!) This function assumes that the total number of contents is larger than the total number of nodes.

        // For the smart home case we use one content per producer node.
        return arrayf;
    }
}


