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

    void ReduceRouteFreq(NodeContainer routers) {
        std::cout << "BLIEP BLIEP" << std::endl;
        for (int idx = 0; idx < ((int) routers.GetN()); idx++) {
            Ptr<RipNg> ripcur = routers.Get(idx)->GetObject<RipNg>();
            ripcur->SetAttribute("UnsolicitedRoutingUpdate", TimeValue(Seconds(50000)));
            ripcur->SetAttribute("TimeoutDelay", TimeValue(Seconds(50600)));
            ripcur->SetAttribute("GarbageCollectionDelay", TimeValue(Seconds(50700)));
        }

    }

    Ptr<Node> SelectRandomLeafNodeConsumer(BriteTopologyHelper & briteth, Ptr<UniformRandomVariable> Rnode) {
        int max_tries = briteth.GetNLeafNodes();
        Ptr<Node> sel_leaf;

        for (int idx = 0; idx <= max_tries; max_tries++) {
            NS_ASSERT_MSG(idx<max_tries, "Tried max_tries to find consumer(!) leafnode. No luck.");
            sel_leaf = SelectRandomNodeFromContainer(briteth.GetLeafNodeContainer(), Rnode);
            if (briteth.IsConnectedLeaf(sel_leaf) == false) {
                break;
            }
        }
        std::cout << "Selected leafnode is (consumer): " << sel_leaf->GetId() << std::endl;
        return sel_leaf;
    }

    Ptr<Node> SelectRandomLeafNode(BriteTopologyHelper & briteth, Ptr<UniformRandomVariable> Rnode) {
        Ptr<Node> sel_leaf = SelectRandomNodeFromContainer(briteth.GetLeafNodeContainer(), Rnode);
        std::cout << "Selected leafnode is (general): " << sel_leaf->GetId() << std::endl;
        return sel_leaf;
    }

    Ptr<Node> SelectRandomNodeFromContainer(NodeContainer container, Ptr<UniformRandomVariable> Rnode) {
        Ptr<Node> selNode;
        double rn = Rnode->GetValue((double) (0), (double) (container.GetN() - 1));
        int node = round(rn);
        std::cout << "Chosen node: " << node << " From max of: " << (double) (container.GetN() - 1) << " from container " << container.GetN() << " " << rn << std::endl;
        selNode = container.Get(node);
        NS_LOG_DEBUG("Selected: " << selNode->GetId());
        return selNode;
    }

    void shuffle_array(std::vector<int>& arrayf, Ptr<UniformRandomVariable> shuffles) {
        for (int idx = 0; idx < ((int) arrayf.size()); idx++) {
            std::cout << arrayf[idx] << std::endl;
        }

        //Shuffles std::vector array a random number of times.
        double max = shuffles->GetValue(2, 20);
        std::cout << "NDN: Max is: " << max << std::endl;
        for (int cnt = 0; cnt < (int) max; cnt++) {

            random_shuffle_ns3(arrayf.begin(), arrayf.end(), shuffles);
        }
        std::cout << std::endl;
        std::cout << std::endl;
        for (int idx = 0; idx < ((int) arrayf.size()); idx++) {
            std::cout << arrayf[idx] << std::endl;
        }
    };

    void shuffle_array_ip(std::vector< std::vector<Ipv6Address> >& arrayf, Ptr<UniformRandomVariable> shuffles, int64_t stream, int size) {
        //Shuffles std::vector array a random number of times.
        std::vector< std::vector<Ipv6Address> > array_;
        array_ = arrayf;
        std::vector<int> content_chunks(size);
        std::iota(std::begin(content_chunks), std::end(content_chunks), 1);
        Ptr<UniformRandomVariable> Rpro = CreateObject<UniformRandomVariable> ();
        Rpro->SetStream(stream);
        shuffle_array(content_chunks, Rpro);
        for (int jdx = 0; jdx < ((int) arrayf.size()); jdx++) {
            for (int idx = 0; idx < ((int) arrayf[jdx].size()); idx++) {
                arrayf[jdx][content_chunks[idx] - 1] = array_[jdx][idx];
            }
        }
    };

    std::vector< std::vector<Ipv6Address> > CreateAddrResBucket(std::vector< std::vector<Ipv6Address> > &arrayf, int numContentsPerDomain) {

        // Function to generate an address resolution vector, which maps content number (array iterator) to a specific producer.
        // (!) This function assumes that the total number of contents is larger than the total number of nodes.

        // For the smart home case we use one content per producer node.
        return arrayf;
    }

    //This function is copied form std::random_shuffle.

    template<typename _RandomAccessIterator>
    inline void
    random_shuffle_ns3(_RandomAccessIterator __first, _RandomAccessIterator __last, Ptr<UniformRandomVariable> shuffles) {
        // concept requirements
        __glibcxx_function_requires(_Mutable_RandomAccessIteratorConcept<
                _RandomAccessIterator>)
                __glibcxx_requires_valid_range(__first, __last);

        if (__first != __last)
            for (_RandomAccessIterator __i = __first + 1; __i != __last; ++__i) {
                // XXX rand() % N is not uniformly distributed
                _RandomAccessIterator __j = __first
                        + ((int) shuffles->GetValue(0, 2147483647)) % ((__i - __first) + 1);
                if (__i != __j)
                    std::iter_swap(__i, __j);
            }
    }
}


