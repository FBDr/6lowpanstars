/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <bits/signum.h>

#include "best-route-strategy2.hpp"
#include "algorithm.hpp"
#include "core/logger.hpp"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/core-module.h"
#include "ns3/object.h"
#include "ns3/node-list.h"
#include  "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "src/ndnSIM/ndn-cxx/src/util/face-uri.hpp"
#include "src/ndnSIM/ndn-cxx/src/name.hpp"

namespace nfd {
    namespace fw {

        NFD_LOG_INIT("BestRouteStrategy2");

        const Name BestRouteStrategy2::STRATEGY_NAME("ndn:/localhost/nfd/strategy/best-route/%FD%04");
        NFD_REGISTER_STRATEGY(BestRouteStrategy2);

        const time::milliseconds BestRouteStrategy2::RETX_SUPPRESSION_INITIAL(10);
        const time::milliseconds BestRouteStrategy2::RETX_SUPPRESSION_MAX(250);

        BestRouteStrategy2::BestRouteStrategy2(Forwarder& forwarder, const Name& name)
        : Strategy(forwarder, name)
        , m_retxSuppression(RETX_SUPPRESSION_INITIAL,
        RetxSuppressionExponential::DEFAULT_MULTIPLIER,
        RETX_SUPPRESSION_MAX)
        , m_node(NULL) {

        }

//        void BestRouteStrategy2::setNode() {
//            ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
//            ns3::Ptr<ns3::ndn::L3Protocol> L3Prot = m_node->GetObject<ns3::ndn::L3Protocol>();
//
//            m_node = thisNode;
//            m_is_GTW = L3Prot->getGTW();
//        }
//
//        ns3::Ptr<ns3::Node> BestRouteStrategy2::getNode() {
//            if (m_node == NULL) { //initialize once function is called for first time.
//                setNode();
//            }
//            return m_node;
//        }

        void
        BestRouteStrategy2::CustomSendInterest(const shared_ptr<pit::Entry>& pitEntry, Face& outFace, const Interest& interest) {

//            Interest interestcopy = interest;
//            shared_ptr<Name> nameWithSequence;
//            std::string extra = "ovrhd";
//            int size = 5;
//            uint8_t * buff = new uint8_t [size];
//
//            memcpy(buff, extra.c_str(), size);
//
//            if (m_is_GTW || 1) {
//                FaceUri oerie = outFace.getLocalUri();
//                if ((oerie.getScheme() != "AppFace") && (outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL)) {
//                    std::cout << "Sending special interest2" << std::endl;
//                    nameWithSequence = make_shared<Name>(interestcopy.getName());
//                    // std::cout<< (nameWithSequence->getSubName(0,nameWithSequence->size()-1 )).toUri() <<std::endl; If we want to remove
//                    nameWithSequence->append(buff, size);
//                    interestcopy.setName(*nameWithSequence);
//                    std::cout << interestcopy.getName() << std::endl;
//                    this->sendInterest(pitEntry, outFace, interestcopy);
//                    
//                    return;
//                }
//            }
            this->sendInterest(pitEntry, outFace, interest);




        }

        /** \brief determines whether a NextHop is eligible
         *  \param inFace incoming face of current Interest
         *  \param interest incoming Interest
         *  \param nexthop next hop
         *  \param pitEntry PIT entry
         *  \param wantUnused if true, NextHop must not have unexpired out-record
         *  \param now time::steady_clock::now(), ignored if !wantUnused
         */
        static inline bool
        isNextHopEligible(const Face& inFace, const Interest& interest,
                const fib::NextHop& nexthop,
                const shared_ptr<pit::Entry>& pitEntry,
                bool wantUnused = false,
                time::steady_clock::TimePoint now = time::steady_clock::TimePoint::min()) {
            const Face& outFace = nexthop.getFace();

            // do not forward back to the same face
            if (&outFace == &inFace) {
                NFD_LOG_DEBUG("&outFace == &inFace ==> Reject");
                return false;

            }


            // forwarding would violate scope
            if (wouldViolateScope(inFace, interest, outFace)) {
                NFD_LOG_DEBUG("Violates scope! ==> Reject");
                return false;
            }

            if (wantUnused) {
                // nexthop must not have unexpired out-record
                pit::OutRecordCollection::iterator outRecord = pitEntry->getOutRecord(outFace);
                if (outRecord != pitEntry->out_end() && outRecord->getExpiry() > now) {
                    NFD_LOG_DEBUG("PIT expired.");
                    return false;
                }
            }

            return true;
        }

        /** \brief pick an eligible NextHop with earliest out-record
         *  \note It is assumed that every nexthop has an out-record.
         */
        static inline fib::NextHopList::const_iterator
        findEligibleNextHopWithEarliestOutRecord(const Face& inFace, const Interest& interest,
                const fib::NextHopList& nexthops,
                const shared_ptr<pit::Entry>& pitEntry) {
            fib::NextHopList::const_iterator found = nexthops.end();
            time::steady_clock::TimePoint earliestRenewed = time::steady_clock::TimePoint::max();
            for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
                if (!isNextHopEligible(inFace, interest, *it, pitEntry))
                    continue;
                pit::OutRecordCollection::iterator outRecord = pitEntry->getOutRecord(it->getFace());
                BOOST_ASSERT(outRecord != pitEntry->out_end());
                if (outRecord->getLastRenewed() < earliestRenewed) {
                    found = it;
                    earliestRenewed = outRecord->getLastRenewed();
                }
            }
            return found;
        }

        void
        BestRouteStrategy2::afterReceiveInterest(const Face& inFace, const Interest& interest,
                const shared_ptr<pit::Entry>& pitEntry) {
            RetxSuppression::Result suppression = m_retxSuppression.decide(inFace, interest, *pitEntry);
            if (suppression == RetxSuppression::SUPPRESS) {
                NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                        << " suppressed");
                return;
            }

            const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
            const fib::NextHopList& nexthops = fibEntry.getNextHops();
            fib::NextHopList::const_iterator it = nexthops.end();

            if (suppression == RetxSuppression::NEW) {
                // forward to nexthop with lowest cost except downstream
                it = std::find_if(nexthops.begin(), nexthops.end(),
                        bind(&isNextHopEligible, cref(inFace), interest, _1, pitEntry,
                        false, time::steady_clock::TimePoint::min()));

                if (it == nexthops.end()) {
                    NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " noNextHop");

                    lp::NackHeader nackHeader;
                    nackHeader.setReason(lp::NackReason::NO_ROUTE);
                    this->sendNack(pitEntry, inFace, nackHeader);

                    this->rejectPendingInterest(pitEntry);
                    return;
                }

                Face& outFace = it->getFace();
                //this->sendInterest(pitEntry, outFace, interest);
                CustomSendInterest(pitEntry, outFace, interest);
                NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                        << " newPitEntry-to=" << outFace.getId());
                return;
            }

            // find an unused upstream with lowest cost except downstream
            it = std::find_if(nexthops.begin(), nexthops.end(),
                    bind(&isNextHopEligible, cref(inFace), interest, _1, pitEntry,
                    true, time::steady_clock::now()));
            if (it != nexthops.end()) {
                Face& outFace = it->getFace();
                //this->sendInterest(pitEntry, outFace, interest);
                CustomSendInterest(pitEntry, outFace, interest);
                NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                        << " retransmit-unused-to=" << outFace.getId());
                return;
            }

            // find an eligible upstream that is used earliest
            it = findEligibleNextHopWithEarliestOutRecord(inFace, interest, nexthops, pitEntry);
            if (it == nexthops.end()) {
                NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " retransmitNoNextHop");
            } else {
                Face& outFace = it->getFace();
                //this->sendInterest(pitEntry, outFace, interest);
                CustomSendInterest(pitEntry, outFace, interest);
                NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                        << " retransmit-retry-to=" << outFace.getId());
            }
        }

        /** \return less severe NackReason between x and y
         *
         *  lp::NackReason::NONE is treated as most severe
         */
        inline lp::NackReason
        compareLessSevere(lp::NackReason x, lp::NackReason y) {
            if (x == lp::NackReason::NONE) {
                return y;
            }
            if (y == lp::NackReason::NONE) {
                return x;
            }
            return static_cast<lp::NackReason> (std::min(static_cast<int> (x), static_cast<int> (y)));
        }

        void
        BestRouteStrategy2::afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                const shared_ptr<pit::Entry>& pitEntry) {
            int nOutRecordsNotNacked = 0;
            Face* lastFaceNotNacked = nullptr;
            lp::NackReason leastSevereReason = lp::NackReason::NONE;
            for (const pit::OutRecord& outR : pitEntry->getOutRecords()) {
                const lp::NackHeader* inNack = outR.getIncomingNack();
                if (inNack == nullptr) {
                    ++nOutRecordsNotNacked;
                    lastFaceNotNacked = &outR.getFace();
                    continue;
                }

                leastSevereReason = compareLessSevere(leastSevereReason, inNack->getReason());
            }

            lp::NackHeader outNack;
            outNack.setReason(leastSevereReason);

            if (nOutRecordsNotNacked == 1) {
                BOOST_ASSERT(lastFaceNotNacked != nullptr);
                pit::InRecordCollection::iterator inR = pitEntry->getInRecord(*lastFaceNotNacked);
                if (inR != pitEntry->in_end()) {
                    // one out-record not Nacked, which is also a downstream
                    NFD_LOG_DEBUG(nack.getInterest() << " nack-from=" << inFace.getId() <<
                            " nack=" << nack.getReason() <<
                            " nack-to(bidirectional)=" << lastFaceNotNacked->getId() <<
                            " out-nack=" << outNack.getReason());
                    this->sendNack(pitEntry, *lastFaceNotNacked, outNack);
                    return;
                }
            }

            if (nOutRecordsNotNacked > 0) {
                NFD_LOG_DEBUG(nack.getInterest() << " nack-from=" << inFace.getId() <<
                        " nack=" << nack.getReason() <<
                        " waiting=" << nOutRecordsNotNacked);
                // continue waiting
                return;
            }

            NFD_LOG_DEBUG(nack.getInterest() << " nack-from=" << inFace.getId() <<
                    " nack=" << nack.getReason() <<
                    " nack-to=all out-nack=" << outNack.getReason());
            this->sendNacks(pitEntry, outNack);
        }

    } // namespace fw
} // namespace nfd
