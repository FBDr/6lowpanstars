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

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("extra-functions");

    Ptr<Node> SelectRandomLeafNode(BriteTopologyHelper & briteth) {
        Ptr<Node> sel_leaf = SelectRandomNodeFromContainer(briteth.GetLeafNodeContainer());
        std::cout<<"Selected leafnode is: "<< sel_leaf->GetId()<<std::endl;
        return sel_leaf;
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


