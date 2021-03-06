/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Tsinghua University, P.R.China.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Xiaoke Jiang <shock.jiang@gmail.com>
 **/

#include "ndn-consumer-zipf-mandelbrot-v2.hpp"
#include "src/core/model/integer.h"
#include "ndn-producer.hpp"
#include <math.h>

NS_LOG_COMPONENT_DEFINE("ndn.ConsumerZipfMandelbrotV2");

namespace ns3 {
    namespace ndn {

        NS_OBJECT_ENSURE_REGISTERED(ConsumerZipfMandelbrotV2);

        TypeId
        ConsumerZipfMandelbrotV2::GetTypeId(void) {
            static TypeId tid =
                    TypeId("ns3::ndn::ConsumerZipfMandelbrotV2")
                    .SetGroupName("Ndn")
                    .SetParent<ConsumerCbr>()
                    .AddConstructor<ConsumerZipfMandelbrotV2>()

                    .AddAttribute("NumberOfContents", "Number of the Contents in total", StringValue("100"),
                    MakeUintegerAccessor(&ConsumerZipfMandelbrotV2::SetNumberOfContents,
                    &ConsumerZipfMandelbrotV2::GetNumberOfContents),
                    MakeUintegerChecker<uint32_t>())

                    .AddAttribute("RngStream", "Set Rng stream.", StringValue("-1"),
                    MakeUintegerAccessor(&ConsumerZipfMandelbrotV2::SetRngStream,
                    &ConsumerZipfMandelbrotV2::GetRngStream),
                    MakeUintegerChecker<uint32_t>())

                    .AddAttribute("q", "parameter of improve rank", StringValue("0.7"),
                    MakeDoubleAccessor(&ConsumerZipfMandelbrotV2::SetQ,
                    &ConsumerZipfMandelbrotV2::GetQ),
                    MakeDoubleChecker<double>())

                    .AddAttribute("s", "parameter of power", StringValue("0.7"),
                    MakeDoubleAccessor(&ConsumerZipfMandelbrotV2::SetS,
                    &ConsumerZipfMandelbrotV2::GetS),
                    MakeDoubleChecker<double>());

            return tid;
        }

        ConsumerZipfMandelbrotV2::ConsumerZipfMandelbrotV2()
        : m_N(100) // needed here to make sure when SetQ/SetS are called, there is a valid value of N
        , m_q(0.7)
        , m_s(0.7)
        , m_own_seq(std::numeric_limits<uint32_t>::max())
        , m_seqRng(CreateObject<UniformRandomVariable>()) {
            // SetNumberOfContents is called by NS-3 object system during the initialization
        }

        ConsumerZipfMandelbrotV2::~ConsumerZipfMandelbrotV2() {
        }

        void
        ConsumerZipfMandelbrotV2::SetNumberOfContents(uint32_t numOfContents) {
            m_N = numOfContents;

            NS_LOG_DEBUG(m_q << " and " << m_s << " and " << m_N);

            m_Pcum = std::vector<double>(m_N + 1);

            m_Pcum[0] = 0.0;
            for (uint32_t i = 1; i <= m_N; i++) {
                m_Pcum[i] = m_Pcum[i - 1] + 1.0 / std::pow(i + m_q, m_s);
            }

            for (uint32_t i = 1; i <= m_N; i++) {
                m_Pcum[i] = m_Pcum[i] / m_Pcum[m_N];
                NS_LOG_LOGIC("Cumulative probability [" << i << "]=" << m_Pcum[i]);
                //std::cout << m_Pcum[i] << std::endl;
            }
        }

        uint32_t
        ConsumerZipfMandelbrotV2::GetNumberOfContents() const {
            return m_N;
        }

        void
        ConsumerZipfMandelbrotV2::SetRngStream(uint32_t stream) {
            m_seqRng->SetStream((int64_t) stream);

        }

        uint32_t
        ConsumerZipfMandelbrotV2::GetRngStream() const {
            return (uint32_t) m_seqRng->GetStream();
        }

        void
        ConsumerZipfMandelbrotV2::SetQ(double q) {
            m_q = q;
            SetNumberOfContents(m_N);
        }

        double
        ConsumerZipfMandelbrotV2::GetQ() const {
            return m_q;
        }

        void
        ConsumerZipfMandelbrotV2::SetS(double s) {
            m_s = s;
            SetNumberOfContents(m_N);
        }

        double
        ConsumerZipfMandelbrotV2::GetS() const {
            return m_s;
        }

        void
        ConsumerZipfMandelbrotV2::SendPacket() {
            if (!m_active)
                return;
            NS_LOG_FUNCTION_NOARGS();

            //Check if this node has a producer application running also.

            if (m_own_seq == std::numeric_limits<uint32_t>::max()) //First time
            {
                for (uint32_t idx = 0; idx < (GetNode()->GetNApplications()); idx++) {
                    if (GetNode()->GetApplication(idx)->GetInstanceTypeId().GetName() == "ns3::ndn::Producer") {
                        Name own_name_pro = GetNode()->GetApplication(idx)->GetObject<Producer>()->GetPrefix();
                        int curseq = atoi(own_name_pro.at(-1).toUri().c_str());
                        m_own_seq = (uint32_t) curseq;
                        break;
                    }
                    m_own_seq = std::numeric_limits<uint32_t>::max() - 1337;
                }
            }

            uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

            // std::cout << Simulator::Now ().ToDouble (Time::S) << "s max -> " << m_seqMax << "\n";

            while (m_retxSeqs.size()) {
                seq = *m_retxSeqs.begin();
                m_retxSeqs.erase(m_retxSeqs.begin());

                // NS_ASSERT (m_seqLifetimes.find (seq) != m_seqLifetimes.end ());
                // if (m_seqLifetimes.find (seq)->time <= Simulator::Now ())
                //   {

                //     NS_LOG_DEBUG ("Expire " << seq);
                //     m_seqLifetimes.erase (seq); // lifetime expired. Trying to find another unexpired
                //     sequence number
                //     continue;
                //   }
                NS_LOG_DEBUG("=interest seq " << seq << " from m_retxSeqs");
                break;
            }

            if (seq == std::numeric_limits<uint32_t>::max()) // no retransmission
            {
                if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
                    if (m_seq >= m_seqMax) {
                        return; // we are totally done
                    }
                }
                NS_LOG_INFO(m_own_seq);
                if (m_own_seq != (std::numeric_limits<uint32_t>::max() - 1337)) {
                    do {
                        NS_LOG_INFO("Next sequence...");
                        seq = ConsumerZipfMandelbrotV2::GetNextSeq() - 1; //-1 because of IP implm.
                    } while (seq == m_own_seq);
                    m_seq++;
                } else {
                    seq = ConsumerZipfMandelbrotV2::GetNextSeq() - 1; //-1 because of IP implm.
                    m_seq++;
                }

            }

            // std::cout << Simulator::Now ().ToDouble (Time::S) << "s -> " << seq << "\n";
            // From here we internchange seq
            //
            shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
            Name seqname(std::to_string(seq).c_str());
            nameWithSequence->append(seqname);

            nameWithSequence->appendSequenceNumber(seq);
            //

            shared_ptr<Interest> interest = make_shared<Interest>();
            interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
            interest->setName(*nameWithSequence);
            m_tx_bytes += (uint64_t) interest->wireEncode().size();
            m_tx_packets++;
            NS_LOG_INFO("Requesting Interest: \n" << *interest);
            NS_LOG_INFO("> Interest for " << seq << ", Total: " << m_seq << ", face: " << m_face->getId());
            NS_LOG_DEBUG("Trying to add " << seq << " with " << Simulator::Now() << ". already "
                    << m_seqTimeouts.size() << " items");

            m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
            m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));

            m_seqLastDelay.erase(seq);
            m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));

            m_seqRetxCounts[seq]++;

            m_rtt->SentSeq(SequenceNumber32(seq), 1);

            m_transmittedInterests(interest, this, m_face);
            m_appLink->onReceiveInterest(*interest);

            ConsumerZipfMandelbrotV2::ScheduleNextPacket();
        }

        uint32_t
        ConsumerZipfMandelbrotV2::GetNextSeq() {
            uint32_t content_index = 1; //[1, m_N]
            double p_sum = 0;

            double p_random = m_seqRng->GetValue();
            while (p_random == 0) {
                p_random = m_seqRng->GetValue();
            }
            // if (p_random == 0)
            NS_LOG_LOGIC("p_random=" << p_random);
            for (uint32_t i = 1; i <= m_N; i++) {
                p_sum = m_Pcum[i]; // m_Pcum[i] = m_Pcum[i-1] + p[i], p[0] = 0;   e.g.: p_cum[1] = p[1],
                // p_cum[2] = p[1] + p[2]
                if (p_random <= p_sum) {
                    content_index = i;
                    break;
                } // if
            } // for
            // content_index = 1;
            NS_LOG_DEBUG("RandomNumber=" << content_index);
            return content_index;
        }

        void
        ConsumerZipfMandelbrotV2::ScheduleNextPacket() {

            if (m_firstTime) {
                m_sendEvent = Simulator::Schedule(Seconds(0.0), &ConsumerZipfMandelbrotV2::SendPacket, this);
                m_firstTime = false;
            } else if (!m_sendEvent.IsRunning())
                m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                    : Seconds(m_random->GetValue()),
                    &ConsumerZipfMandelbrotV2::SendPacket, this);
        }

    } /* namespace ndn */
} /* namespace ns3 */
