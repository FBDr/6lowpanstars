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

#include "forwarder.hpp"
#include "algorithm.hpp"
#include "core/logger.hpp"
#include "strategy.hpp"
#include "table/cleanup.hpp"
#include <ndn-cxx/lp/tags.hpp>
#include "face/null-face.hpp"
#include <boost/random/uniform_int_distribution.hpp>
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/core-module.h"
#include "ns3/object.h"
#include  "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "src/ndnSIM/ndn-cxx/src/interest.hpp"
#include "ns3/random-variable-stream.h"
#include "logger.hpp"
#include "src/ndnSIM/ndn-cxx/src/util/face-uri.hpp"
#include <utility>
namespace nfd {

    NFD_LOG_INIT("Forwarder");

    Forwarder::Forwarder()
    : m_unsolicitedDataPolicy(new fw::DefaultUnsolicitedDataPolicy())
    , m_fib(m_nameTree)
    , m_pit(m_nameTree)
    , m_measurements(m_nameTree)
    , m_strategyChoice(m_nameTree, fw::makeDefaultStrategy(*this))
    , m_tx_data_bytes(0)
    , m_tx_interest_bytes(0)
    , m_csFace(face::makeNullFace(FaceUri("contentstore://"))) {
        m_node = NULL;
        fw::installStrategies(*this);
        getFaceTable().addReserved(m_csFace, face::FACEID_CONTENT_STORE);

        m_faceTable.afterAdd.connect([this] (Face & face) {
            face.afterReceiveInterest.connect(
                    [this, &face] (const Interest & interest) {
                        this->startProcessInterest(face, interest);
                    });
            face.afterReceiveData.connect(
                    [this, &face] (const Data & data) {
                        this->startProcessData(face, data);
                    });
            face.afterReceiveNack.connect(
                    [this, &face] (const lp::Nack & nack) {
                        this->startProcessNack(face, nack);
                    });
        });

        m_faceTable.beforeRemove.connect([this] (Face & face) {
            cleanupOnFaceRemoval(m_nameTree, m_fib, m_pit, face);
        });
    }

    Forwarder::~Forwarder() = default;

    void
    Forwarder::setNode(ns3::Ptr<ns3::Node> node) {
        m_node = node;
    }

    ns3::Ptr<ns3::Node>
    Forwarder::getNode() {
        return m_node;
    }

    uint64_t
    Forwarder::getTx_data_bytes() {
        return m_tx_data_bytes;
    }

    uint64_t
    Forwarder::getTx_interest_bytes() {
        return m_tx_interest_bytes;
    }

    uint8_t
    Forwarder::iamGTW() {
        ns3::Ptr<ns3::ndn::L3Protocol> L3Prot = m_node->GetObject<ns3::ndn::L3Protocol>();
        return L3Prot->getRole();
    }

    std::string
    Forwarder::genRandomString(int len) {
        std::string returnString;
        ns3::Ptr<ns3::UniformRandomVariable> Rnum = ns3::CreateObject<ns3::UniformRandomVariable> ();
        for (int idx = 0; idx < len; idx++) {
            int curnum = round(Rnum->GetValue(0, 9));
            returnString.append(std::to_string(curnum));
        }
        return returnString;
    }

    void
    Forwarder::startProcessInterest(Face& face, const Interest& interest) {
        // check fields used by forwarding are well-formed
        try {
            if (interest.hasLink()) {
                interest.getLink();
            }
        } catch (const tlv::Error&) {
            NFD_LOG_DEBUG("startProcessInterest face=" << face.getId() <<
                    " interest=" << interest.getName() << " malformed");
            // It's safe to call interest.getName() because Name has been fully parsed
            return;
        }

        this->onIncomingInterest(face, interest);
    }

    void
    Forwarder::startProcessData(Face& face, const Data& data) {
        // check fields used by forwarding are well-formed
        // (none needed)

        this->onIncomingData(face, data);
    }

    void
    Forwarder::startProcessNack(Face& face, const lp::Nack& nack) {
        // check fields used by forwarding are well-formed
        try {
            if (nack.getInterest().hasLink()) {
                nack.getInterest().getLink();
            }
        } catch (const tlv::Error&) {
            NFD_LOG_DEBUG("startProcessNack face=" << face.getId() <<
                    " nack=" << nack.getInterest().getName() <<
                    "~" << nack.getReason() << " malformed");
            return;
        }

        this->onIncomingNack(face, nack);
    }

    void
    Forwarder::onIncomingInterest(Face& inFace, const Interest& interest) {

        // receive Interest
        NFD_LOG_DEBUG("onIncomingInterest face=" << inFace.getId() <<
                " interest=" << interest.getName());
        interest.setTag(make_shared<lp::IncomingFaceIdTag>(inFace.getId()));
        ++m_counters.nInInterests;

        //**New part for IP backhaul functionality. 

        // Make copy of interest in order to be able to change it.
        auto outInterest = make_shared<Interest>(interest);

        //If interest name contains "ovrhd" keyword, set m_conOvrhd flag.
        if (interest.getName().at(-1).toUri().find("ovrhd") != std::string::npos) {
            m_conOvrhd_int = true;
            NFD_LOG_DEBUG("Contains overhead");
        } else {
            m_conOvrhd_int = false;
        }

        // If Name contains overhead component and this node is configured as gateway,
        // then remove this component.
        if (m_conOvrhd_int && (iamGTW() == 2)) {
            std::string ovrhd = interest.getName().at(-1).toUri();
            Name subname = interest.getName().getSubName(0, interest.getName().size() - 1);
            //Place ovrhd label into m_trnsoverhead translation vector.
            m_trnsoverhead.push_back(std::make_pair(subname.toUri(), ovrhd));
            outInterest->setName(subname);
            NFD_LOG_DEBUG("Removed overhead component, name is now: " << outInterest->getName());
        }

        //**End of new part.

        // /localhost scope control
        bool isViolatingLocalhost = inFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL &&
                scope_prefix::LOCALHOST.isPrefixOf(interest.getName());
        if (isViolatingLocalhost) {
            NFD_LOG_DEBUG("onIncomingInterest face=" << inFace.getId() <<
                    " interest=" << interest.getName() << " violates /localhost");
            // (drop)
            return;
        }


        // detect duplicate Nonce with Dead Nonce List
        bool hasDuplicateNonceInDnl = m_deadNonceList.has(outInterest->getName(), outInterest->getNonce());
        if (hasDuplicateNonceInDnl) {
            // goto Interest loop pipeline
            this->onInterestLoop(inFace, *outInterest);
            return;
        }

        // PIT insert
        shared_ptr<pit::Entry> pitEntry = m_pit.insert(*outInterest).first;

        // detect duplicate Nonce in PIT entry
        bool hasDuplicateNonceInPit = fw::findDuplicateNonce(*pitEntry, outInterest->getNonce(), inFace) !=
                fw::DUPLICATE_NONCE_NONE;
        if (hasDuplicateNonceInPit) {
            // goto Interest loop pipeline
            this->onInterestLoop(inFace, *outInterest);
            return;
        }

        // cancel unsatisfy & straggler timer
        this->cancelUnsatisfyAndStragglerTimer(*pitEntry);

        const pit::InRecordCollection& inRecords = pitEntry->getInRecords();
        bool isPending = inRecords.begin() != inRecords.end();
        if (!isPending) {
            if (m_csFromNdnSim == nullptr) {
                m_cs.find(*outInterest,
                        bind(&Forwarder::onContentStoreHit, this, ref(inFace), pitEntry, _1, _2),
                        bind(&Forwarder::onContentStoreMiss, this, ref(inFace), pitEntry, _1));
            } else {
                shared_ptr<Data> match = m_csFromNdnSim->Lookup(outInterest->shared_from_this());
                if (match != nullptr) {
                    this->onContentStoreHit(inFace, pitEntry, *outInterest, *match);
                } else {
                    this->onContentStoreMiss(inFace, pitEntry, *outInterest);
                }
            }
        } else {
            this->onContentStoreMiss(inFace, pitEntry, *outInterest);
        }
    }

    void
    Forwarder::onInterestLoop(Face& inFace, const Interest& interest) {
        // if multi-access face, drop
        if (inFace.getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS) {
            NFD_LOG_DEBUG("onInterestLoop face=" << inFace.getId() <<
                    " interest=" << interest.getName() <<
                    " drop");
            return;
        }

        NFD_LOG_DEBUG("onInterestLoop face=" << inFace.getId() <<
                " interest=" << interest.getName() <<
                " send-Nack-duplicate");

        // send Nack with reason=DUPLICATE
        // note: Don't enter outgoing Nack pipeline because it needs an in-record.
        lp::Nack nack(interest);
        nack.setReason(lp::NackReason::DUPLICATE);
        inFace.sendNack(nack);
    }

    void
    Forwarder::onContentStoreMiss(const Face& inFace, const shared_ptr<pit::Entry>& pitEntry,
            const Interest& interest) {
        NFD_LOG_DEBUG("onContentStoreMiss interest=" << interest.getName());

        // insert in-record
        pitEntry->insertOrUpdateInRecord(const_cast<Face&> (inFace), interest);

        // set PIT unsatisfy timer
        this->setUnsatisfyTimer(pitEntry);

        // has NextHopFaceId?
        shared_ptr<lp::NextHopFaceIdTag> nextHopTag = interest.getTag<lp::NextHopFaceIdTag>();
        if (nextHopTag != nullptr) {
            // chosen NextHop face exists?
            Face* nextHopFace = m_faceTable.get(*nextHopTag);
            if (nextHopFace != nullptr) {
                NFD_LOG_DEBUG("onContentStoreMiss interest=" << interest.getName() << " nexthop-faceid=" << nextHopFace->getId());
                // go to outgoing Interest pipeline
                // scope control is unnecessary, because privileged app explicitly wants to forward
                this->onOutgoingInterest(pitEntry, *nextHopFace, interest);
            }
            return;
        }

        // dispatch to strategy: after incoming Interest
        this->dispatchToStrategy(*pitEntry,
                [&] (fw::Strategy & strategy) {
                    strategy.afterReceiveInterest(inFace, interest, pitEntry); });
    }

    void
    Forwarder::onContentStoreHit(const Face& inFace, const shared_ptr<pit::Entry>& pitEntry,
            const Interest& interest, const Data& data) {
        NFD_LOG_DEBUG("onContentStoreHit interest=" << interest.getName());

        beforeSatisfyInterest(*pitEntry, *m_csFace, data);
        this->dispatchToStrategy(*pitEntry,
                [&] (fw::Strategy & strategy) {
                    strategy.beforeSatisfyInterest(pitEntry, *m_csFace, data); });

        data.setTag(make_shared<lp::IncomingFaceIdTag>(face::FACEID_CONTENT_STORE));
        // XXX should we lookup PIT for other Interests that also match csMatch?

        // set PIT straggler timer
        this->setStragglerTimer(pitEntry, true, data.getFreshnessPeriod());

        // goto outgoing Data pipeline
        this->onOutgoingData(data, *const_pointer_cast<Face>(inFace.shared_from_this()));
    }

    void
    Forwarder::onOutgoingInterest(const shared_ptr<pit::Entry>& pitEntry, Face& outFace, const Interest& interest) {
        NFD_LOG_DEBUG("onOutgoingInterest face=" << outFace.getId() <<
                " interest=" << pitEntry->getName());

        //Debug
        NFD_LOG_DEBUG("Link local: " << outFace.getLocalUri());
        NFD_LOG_DEBUG("Link remote: " << outFace.getRemoteUri());
        NFD_LOG_DEBUG("Link: " << outFace.getTransport()->getRemoteUri());

        // insert out-record
        pitEntry->insertOrUpdateOutRecord(outFace, interest);

        // send Interest
        //**New part for backhaul modeling
        auto outInterest = make_shared<Interest>(interest);
        // Size is the total size of the extra overhead component.
        int size = 48 - 2;
        uint8_t * buff = new uint8_t [size];
        shared_ptr<Name> nameWithSequence;
        std::string extra = "ovrhd";
        extra.append(genRandomString(size - 5));
        FaceUri oerie = outFace.getLocalUri();
        memcpy(buff, extra.c_str(), size);

        // If this node is either a gateway or a backhaul node, add overhead name. 
        // This should only be done if the incoming interest already had an overhead component.
        // Also check that outface is not a broadcast domain.
        if ((iamGTW() == 1 || (iamGTW() == 2)) && (m_conOvrhd_int == 0) && ((outFace.getRemoteUri().toString() == "netdev://[ff:ff:ff:ff:ff:ff]")
                && (outFace.getLocalUri().toString() == "netdev://[ff:ff:ff:ff:ff:ff]")) != 1) {
            if ((oerie.getScheme() != "AppFace") && (outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL)) {
                if (iamGTW() == 1) {
                    NFD_LOG_DEBUG("Node: " << m_node->GetId() << " is configured as a backhaulnode. " << "Adding overhead component.");
                } else if (iamGTW() == 2) {
                    NFD_LOG_DEBUG("Node: " << m_node->GetId() << " is configured as a gateway node." << "Adding overhead component.");
                }
                nameWithSequence = make_shared<Name>(outInterest->getName());
                nameWithSequence->append(buff, size);
                outInterest->setName(*nameWithSequence);
                NFD_LOG_DEBUG("Sending overhead interest: " << outInterest->getName());
            }
        }

        if ((oerie.getScheme() != "AppFace") && (outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL)) {
            m_tx_interest_bytes += (uint64_t) (outInterest->wireEncode().size()) + 7;
        }

        //**End of part for backhaul modeling.
        outFace.sendInterest(*outInterest);
        ++m_counters.nOutInterests;
    }

    void
    Forwarder::onInterestReject(const shared_ptr<pit::Entry>& pitEntry) {
        if (fw::hasPendingOutRecords(*pitEntry)) {
            NFD_LOG_ERROR("onInterestReject interest=" << pitEntry->getName() <<
                    " cannot reject forwarded Interest");
            return;
        }
        NFD_LOG_DEBUG("onInterestReject interest=" << pitEntry->getName());

        // cancel unsatisfy & straggler timer
        this->cancelUnsatisfyAndStragglerTimer(*pitEntry);

        // set PIT straggler timer
        this->setStragglerTimer(pitEntry, false);
    }

    void
    Forwarder::onInterestUnsatisfied(const shared_ptr<pit::Entry>& pitEntry) {
        NFD_LOG_DEBUG("onInterestUnsatisfied interest=" << pitEntry->getName());

        std::string pitName = pitEntry->getName().toUri();
        for (int idx = 0; idx < ((int) m_trnsoverhead.size()); idx++) {
            if (m_trnsoverhead[idx].first == pitName) {
                NFD_LOG_DEBUG("Found trans-entry for Pit: " << idx);
                m_trnsoverhead.erase(m_trnsoverhead.begin() + idx);
                break;
            }
        }

        // invoke PIT unsatisfied callback
        beforeExpirePendingInterest(*pitEntry);
        this->dispatchToStrategy(*pitEntry,
                [&] (fw::Strategy & strategy) {
                    strategy.beforeExpirePendingInterest(pitEntry); });

        // goto Interest Finalize pipeline
        this->onInterestFinalize(pitEntry, false);
    }

    void
    Forwarder::onInterestFinalize(const shared_ptr<pit::Entry>& pitEntry, bool isSatisfied,
            time::milliseconds dataFreshnessPeriod) {
        NFD_LOG_DEBUG("onInterestFinalize interest=" << pitEntry->getName() <<
                (isSatisfied ? " satisfied" : " unsatisfied"));

        // Dead Nonce List insert if necessary
        this->insertDeadNonceList(*pitEntry, isSatisfied, dataFreshnessPeriod, 0);

        // PIT delete
        this->cancelUnsatisfyAndStragglerTimer(*pitEntry);
        m_pit.erase(pitEntry.get());
    }

    void
    Forwarder::onIncomingData(Face& inFace, const Data& data) {
        // receive Data
        NFD_LOG_DEBUG("onIncomingData face=" << inFace.getId() << " data=" << data.getName());
        data.setTag(make_shared<lp::IncomingFaceIdTag>(inFace.getId()));
        ++m_counters.nInData;

        //**New part for backhaul modeling.

        auto outData = make_shared<Data>(data);

        // /localhost scope control
        bool isViolatingLocalhost = inFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL &&
                scope_prefix::LOCALHOST.isPrefixOf(data.getName());
        if (isViolatingLocalhost) {
            NFD_LOG_DEBUG("onIncomingData face=" << inFace.getId() <<
                    " data=" << data.getName() << " violates /localhost");
            // (drop)
            return;
        }

        // Set flag if data pkt already contains overhead.
        if (data.getName().at(-1).toUri().find("ovrhd") != std::string::npos) {
            m_conOvrhd_data = true;
            NFD_LOG_DEBUG("Data contains overhead");
        } else {
            m_conOvrhd_data = false;
        }

        // If data packet arrives with overhead component and this node is gateway, then
        // remove overhead component.
        if (m_conOvrhd_data && (iamGTW() == 2)) {
            //Remove last segment
            std::string ovrhd = data.getName().at(-1).toUri();
            Name subname = data.getName().getSubName(0, data.getName().size() - 1);
            outData->setName(subname);
            NFD_LOG_DEBUG("Removed overhead component, name is now: " << outData->getName());
        }

        //**End of new part



        // PIT match
        pit::DataMatchResult pitMatches = m_pit.findAllDataMatches(*outData);
        if (pitMatches.begin() == pitMatches.end()) {
            // goto Data unsolicited pipeline
            this->onDataUnsolicited(inFace, *outData);
            return;
        }
        shared_ptr<Data> dataCopyWithoutTag = make_shared<Data>(*outData);

        dataCopyWithoutTag->removeTag<lp::HopCountTag>();
        // CS insert
        if (m_csFromNdnSim == nullptr)
            m_cs.insert(*dataCopyWithoutTag);
        else
            m_csFromNdnSim->Add(dataCopyWithoutTag);

        std::set<Face*> pendingDownstreams;
        // foreach PitEntry
        auto now = time::steady_clock::now();
        for (const shared_ptr<pit::Entry>& pitEntry : pitMatches) {
            NFD_LOG_DEBUG("onIncomingData matching=" << pitEntry->getName());

            // cancel unsatisfy & straggler timer
            this->cancelUnsatisfyAndStragglerTimer(*pitEntry);

            // remember pending downstreams
            for (const pit::InRecord& inRecord : pitEntry->getInRecords()) {
                NFD_LOG_DEBUG("Found inrecord! " << inRecord.getFace().getId());
                if (inRecord.getExpiry() > now) {
                    NFD_LOG_DEBUG("Is valid");
                    pendingDownstreams.insert(&inRecord.getFace());
                }
            }

            // invoke PIT satisfy callback
            beforeSatisfyInterest(*pitEntry, inFace, *outData);
            this->dispatchToStrategy(*pitEntry,
                    [&] (fw::Strategy & strategy) {
                        strategy.beforeSatisfyInterest(pitEntry, inFace, data); });

            // Dead Nonce List insert if necessary (for out-record of inFace)
            this->insertDeadNonceList(*pitEntry, true, outData->getFreshnessPeriod(), &inFace);

            // mark PIT satisfied
            pitEntry->clearInRecords();
            pitEntry->deleteOutRecord(inFace);

            // set PIT straggler timer
            this->setStragglerTimer(pitEntry, true, outData->getFreshnessPeriod());
        }

        // foreach pending downstream
        for (Face* pendingDownstream : pendingDownstreams) {
            if (pendingDownstream == &inFace) {
                continue;
            }
            // goto outgoing Data pipeline
            this->onOutgoingData(*outData, *pendingDownstream);
        }
    }

    void
    Forwarder::onDataUnsolicited(Face& inFace, const Data& data) {
        // accept to cache?
        fw::UnsolicitedDataDecision decision = m_unsolicitedDataPolicy->decide(inFace, data);
        if (decision == fw::UnsolicitedDataDecision::CACHE) {
            // CS insert
            if (m_csFromNdnSim == nullptr)
                m_cs.insert(data, true);
            else
                m_csFromNdnSim->Add(data.shared_from_this());
        }

        NFD_LOG_DEBUG("onDataUnsolicited face=" << inFace.getId() <<
                " data=" << data.getName() <<
                " decision=" << decision);
    }

    //    void
    //    Forwarder::onOutgoingData(const Data& data, Face& outFace) {
    //        if (outFace.getId() == face::INVALID_FACEID) {
    //            NFD_LOG_WARN("onOutgoingData face=invalid data=" << data.getName());
    //            return;
    //        }
    //        NFD_LOG_DEBUG("onOutgoingData face=" << outFace.getId() << " data=" << data.getName());
    //        // /localhost scope control
    //        bool isViolatingLocalhost = outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL &&
    //                scope_prefix::LOCALHOST.isPrefixOf(data.getName());
    //        if (isViolatingLocalhost) {
    //            NFD_LOG_DEBUG("onOutgoingData face=" << outFace.getId() <<
    //                    " data=" << data.getName() << " violates /localhost");
    //            // (drop)
    //            return;
    //        }
    //        //**New part for backhaul modeling.
    //        shared_ptr<Data> outData = make_shared<Data>(data);
    //        FaceUri oerie = outFace.getLocalUri();
    //        std::string dataName = data.getName().toUri();
    //        shared_ptr<Name> nameWithOvrhd = make_shared<Name>(data.getName());
    //
    //        //If this node is gateway, add overhead component to data name.
    //        if ((iamGTW() == 2)) {
    //            if ((oerie.getScheme() != "AppFace") && (outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL)) {
    //                //Find overhead sequence corresponding to current data name.
    //                for (int idx = 0; idx < ((int) m_trnsoverhead.size()); idx++) {
    //                    NFD_LOG_DEBUG("Looking for correct overhead flag: " << dataName << " " << m_trnsoverhead[idx].first << " " << m_trnsoverhead[idx].second);
    //                }
    //
    //                for (int idx = 0; idx < ((int) m_trnsoverhead.size()); idx++) {
    //                    if (m_trnsoverhead[idx].first == dataName) {
    //                        NFD_LOG_DEBUG("Found entry for data package: " << idx);
    //                        nameWithOvrhd->append(m_trnsoverhead[idx].second);
    //                        outData->setName(*nameWithOvrhd);
    //                        NFD_LOG_DEBUG(nameWithOvrhd->toUri());
    //                        NFD_LOG_DEBUG("Size of overhead container before : " << m_trnsoverhead.size());
    //                        m_trnsoverhead.erase(m_trnsoverhead.begin() + idx);
    //                        NFD_LOG_DEBUG("Size of overhead container after: " << m_trnsoverhead.size());
    //                        break;
    //                    }
    //                    //Throw assertion if no entry is found.
    //                    assert(idx < ((int) m_trnsoverhead.size()));
    //                }
    //            }
    //
    //        }
    //        //**End new part
    //
    //        if ((oerie.getScheme() != "AppFace") && (outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL)) {
    //            m_tx_data_bytes += (uint64_t) (outData->wireEncode().size()) + 7;
    //        }
    //        // send Data
    //        outFace.sendData(*outData);
    //        ++m_counters.nOutData;
    //    }

    void
    Forwarder::onOutgoingData(const Data& data, Face& outFace) {
        if (outFace.getId() == face::INVALID_FACEID) {
            NFD_LOG_WARN("onOutgoingData face=invalid data=" << data.getName());
            return;
        }
        NFD_LOG_DEBUG("onOutgoingData face=" << outFace.getId() << " data=" << data.getName());
        // /localhost scope control
        bool isViolatingLocalhost = outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL &&
                scope_prefix::LOCALHOST.isPrefixOf(data.getName());
        if (isViolatingLocalhost) {
            NFD_LOG_DEBUG("onOutgoingData face=" << outFace.getId() <<
                    " data=" << data.getName() << " violates /localhost");
            // (drop)
            return;
        }
        //**New part for backhaul modeling.
        shared_ptr<Data> outData = make_shared<Data>(data);
        FaceUri oerie = outFace.getLocalUri();
        std::string dataName = data.getName().toUri();


        //If this node is gateway, add overhead component to data name. Data comes from other domain
        if ((iamGTW() == 2) && !m_conOvrhd_data) {
            if ((oerie.getScheme() != "AppFace") && (outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL)) {
                //Find overhead sequence corresponding to current data name.

                for (int idx = 0; idx < ((int) m_trnsoverhead.size()); idx++) {
                    NFD_LOG_DEBUG("Looking for correct overhead flag: " << dataName << " " << m_trnsoverhead[idx].first << " " << m_trnsoverhead[idx].second);
                }

                std::vector<std::pair < std::string, std::string>>::iterator itr = m_trnsoverhead.begin();
                while (itr != m_trnsoverhead.end()) {
                    if (itr->first == dataName) {
                        shared_ptr<Name> nameWithOvrhd = make_shared<Name>(data.getName());
                        NFD_LOG_DEBUG("Found entry for data package: " << itr->first << " " << itr->second);
                        nameWithOvrhd->append(itr->second);
                        outData->setName(*nameWithOvrhd);
                        NFD_LOG_DEBUG(nameWithOvrhd->toUri());
                        itr = m_trnsoverhead.erase(itr);

                        if ((oerie.getScheme() != "AppFace") && (outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL)) {
                            m_tx_data_bytes += (uint64_t) (outData->wireEncode().size()) + 7;
                        }
                        // send Data
                        outFace.sendData(*outData);
                        ++m_counters.nOutData;
                    } else
                        ++itr;
                }
                return;

            }

        }
        //**End new part

        if ((oerie.getScheme() != "AppFace") && (outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL)) {
            m_tx_data_bytes += (uint64_t) (outData->wireEncode().size()) + 7;
        }
        // send Data
        outFace.sendData(*outData);
        ++m_counters.nOutData;
        m_conOvrhd_data = 0;
    }

    void
    Forwarder::onIncomingNack(Face& inFace, const lp::Nack & nack) {
        // receive Nack
        nack.setTag(make_shared<lp::IncomingFaceIdTag>(inFace.getId()));
        ++m_counters.nInNacks;

        // if multi-access face, drop
        if (inFace.getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS) {
            NFD_LOG_DEBUG("onIncomingNack face=" << inFace.getId() <<
                    " nack=" << nack.getInterest().getName() <<
                    "~" << nack.getReason() << " face-is-multi-access");
            return;
        }

        // PIT match
        shared_ptr<pit::Entry> pitEntry = m_pit.find(nack.getInterest());
        // if no PIT entry found, drop
        if (pitEntry == nullptr) {
            NFD_LOG_DEBUG("onIncomingNack face=" << inFace.getId() <<
                    " nack=" << nack.getInterest().getName() <<
                    "~" << nack.getReason() << " no-PIT-entry");
            return;
        }

        // has out-record?
        pit::OutRecordCollection::iterator outRecord = pitEntry->getOutRecord(inFace);
        // if no out-record found, drop
        if (outRecord == pitEntry->out_end()) {
            NFD_LOG_DEBUG("onIncomingNack face=" << inFace.getId() <<
                    " nack=" << nack.getInterest().getName() <<
                    "~" << nack.getReason() << " no-out-record");
            return;
        }

        // if out-record has different Nonce, drop
        if (nack.getInterest().getNonce() != outRecord->getLastNonce()) {
            NFD_LOG_DEBUG("onIncomingNack face=" << inFace.getId() <<
                    " nack=" << nack.getInterest().getName() <<
                    "~" << nack.getReason() << " wrong-Nonce " <<
                    nack.getInterest().getNonce() << "!=" << outRecord->getLastNonce());
            return;
        }

        NFD_LOG_DEBUG("onIncomingNack face=" << inFace.getId() <<
                " nack=" << nack.getInterest().getName() <<
                "~" << nack.getReason() << " OK");

        // record Nack on out-record
        outRecord->setIncomingNack(nack);

        // trigger strategy: after receive NACK
        this->dispatchToStrategy(*pitEntry,
                [&] (fw::Strategy & strategy) {

                    strategy.afterReceiveNack(inFace, nack, pitEntry); });
    }

    void
    Forwarder::onOutgoingNack(const shared_ptr<pit::Entry>& pitEntry, const Face& outFace,
            const lp::NackHeader & nack) {
        if (outFace.getId() == face::INVALID_FACEID) {
            NFD_LOG_WARN("onOutgoingNack face=invalid" <<
                    " nack=" << pitEntry->getInterest().getName() <<
                    "~" << nack.getReason() << " no-in-record");
            return;
        }

        // has in-record?
        pit::InRecordCollection::iterator inRecord = pitEntry->getInRecord(outFace);

        // if no in-record found, drop
        if (inRecord == pitEntry->in_end()) {
            NFD_LOG_DEBUG("onOutgoingNack face=" << outFace.getId() <<
                    " nack=" << pitEntry->getInterest().getName() <<
                    "~" << nack.getReason() << " no-in-record");
            return;
        }

        // if multi-access face, drop
        if ((outFace.getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS) || true) { //No nacks for Wireless domains.
            NFD_LOG_DEBUG("onOutgoingNack face=" << outFace.getId() <<
                    " nack=" << pitEntry->getInterest().getName() <<
                    "~" << nack.getReason() << " face-is-multi-access");

            return;
        }



        NFD_LOG_DEBUG("onOutgoingNack face=" << outFace.getId() <<
                " nack=" << pitEntry->getInterest().getName() <<
                "~" << nack.getReason() << " OK");

        // create Nack packet with the Interest from in-record
        lp::Nack nackPkt(inRecord->getInterest());
        nackPkt.setHeader(nack);

        // erase in-record
        pitEntry->deleteInRecord(outFace);

        // send Nack on face
        const_cast<Face&> (outFace).sendNack(nackPkt);
        ++m_counters.nOutNacks;
    }

    static inline bool
    compare_InRecord_expiry(const pit::InRecord& a, const pit::InRecord & b) {

        return a.getExpiry() < b.getExpiry();
    }

    void
    Forwarder::setUnsatisfyTimer(const shared_ptr<pit::Entry>& pitEntry) {
        pit::InRecordCollection::iterator lastExpiring =
                std::max_element(pitEntry->in_begin(), pitEntry->in_end(), &compare_InRecord_expiry);

        time::steady_clock::TimePoint lastExpiry = lastExpiring->getExpiry();
        time::nanoseconds lastExpiryFromNow = lastExpiry - time::steady_clock::now();
        if (lastExpiryFromNow <= time::seconds::zero()) {
            // TODO all in-records are already expired; will this happen?
        }

        scheduler::cancel(pitEntry->m_unsatisfyTimer);
        pitEntry->m_unsatisfyTimer = scheduler::schedule(lastExpiryFromNow,
                bind(&Forwarder::onInterestUnsatisfied, this, pitEntry));
    }

    void
    Forwarder::setStragglerTimer(const shared_ptr<pit::Entry>& pitEntry, bool isSatisfied,
            time::milliseconds dataFreshnessPeriod) {

        time::nanoseconds stragglerTime = time::milliseconds(100);

        scheduler::cancel(pitEntry->m_stragglerTimer);
        pitEntry->m_stragglerTimer = scheduler::schedule(stragglerTime,
                bind(&Forwarder::onInterestFinalize, this, pitEntry, isSatisfied, dataFreshnessPeriod));
    }

    void
    Forwarder::cancelUnsatisfyAndStragglerTimer(pit::Entry & pitEntry) {

        scheduler::cancel(pitEntry.m_unsatisfyTimer);
        scheduler::cancel(pitEntry.m_stragglerTimer);
    }

    static inline void
    insertNonceToDnl(DeadNonceList& dnl, const pit::Entry& pitEntry,
            const pit::OutRecord & outRecord) {

        dnl.add(pitEntry.getName(), outRecord.getLastNonce());
    }

    void
    Forwarder::insertDeadNonceList(pit::Entry& pitEntry, bool isSatisfied,
            time::milliseconds dataFreshnessPeriod, Face * upstream) {
        // need Dead Nonce List insert?
        bool needDnl = false;
        if (isSatisfied) {
            bool hasFreshnessPeriod = dataFreshnessPeriod >= time::milliseconds::zero();
            // Data never becomes stale if it doesn't have FreshnessPeriod field
            needDnl = static_cast<bool> (pitEntry.getInterest().getMustBeFresh()) &&
                    (hasFreshnessPeriod && dataFreshnessPeriod < m_deadNonceList.getLifetime());
        } else {
            needDnl = true;
        }

        if (!needDnl) {
            return;
        }

        // Dead Nonce List insert
        if (upstream == 0) {
            // insert all outgoing Nonces
            const pit::OutRecordCollection& outRecords = pitEntry.getOutRecords();
            std::for_each(outRecords.begin(), outRecords.end(),
                    bind(&insertNonceToDnl, ref(m_deadNonceList), cref(pitEntry), _1));
        } else {
            // insert outgoing Nonce of a specific face
            pit::OutRecordCollection::iterator outRecord = pitEntry.getOutRecord(*upstream);
            if (outRecord != pitEntry.getOutRecords().end()) {
                m_deadNonceList.add(pitEntry.getName(), outRecord->getLastNonce());
            }
        }
    }

} // namespace nfd
