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
#include "ns3/object.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("extra-functions");
    
    void ReduceRouteFreq(NodeContainer routers){
        std::cout<<"BLIEP BLIEP" <<std::endl;
        for(int idx=0; idx< ( (int) routers.GetN()); idx++)
        {
            Ptr<RipNg> ripcur = routers.Get(idx)->GetObject<RipNg>();
            ripcur->SetAttribute("UnsolicitedRoutingUpdate", TimeValue(Seconds(500)));
            ripcur->SetAttribute("TimeoutDelay", TimeValue(Seconds(600)));
            ripcur->SetAttribute("GarbageCollectionDelay", TimeValue(Seconds(700)));
        }
        
    }

    Ptr<Node> SelectRandomLeafNodeConsumer(BriteTopologyHelper & briteth) {
        int max_tries = briteth.GetNLeafNodes();
        Ptr<Node> sel_leaf;
        
        for (int idx = 0; idx <= max_tries; max_tries++) {
            NS_ASSERT_MSG(idx<max_tries, "Tried max_tries to find consumer(!) leafnode. No luck.");
            sel_leaf = SelectRandomNodeFromContainer(briteth.GetLeafNodeContainer());
            if(briteth.IsConnectedLeaf(sel_leaf) == false)
            {
                break;
            }
        }
        std::cout << "Selected leafnode is (consumer): " << sel_leaf->GetId() << std::endl;
        return sel_leaf;
    }

    Ptr<Node> SelectRandomLeafNode(BriteTopologyHelper & briteth) {
        Ptr<Node> sel_leaf = SelectRandomNodeFromContainer(briteth.GetLeafNodeContainer());
        std::cout << "Selected leafnode is (general): " << sel_leaf->GetId() << std::endl;
        return sel_leaf;
    }

    Ptr<Node> SelectRandomNodeFromContainer(NodeContainer container) {
        Ptr<Node> selNode;
        Ptr<UniformRandomVariable> Rnode = CreateObject<UniformRandomVariable> ();
        std::cout<<(double) (container.GetN() - 1)<<std::endl;
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


