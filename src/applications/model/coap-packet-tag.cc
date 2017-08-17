/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ns3/tag.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include <iostream>
#include "ns3/coap-packet-tag.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("CoapPacketTag");

    CoapPacketTag::CoapPacketTag()
    : m_seq(0), m_req(0),
    m_ts(Simulator::Now().GetTimeStep()) {
        NS_LOG_FUNCTION(this);
    }

    TypeId
    CoapPacketTag::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::CoapPacketTag")
                .SetParent<Tag> ()
                .AddConstructor<CoapPacketTag> ()
                ;
        return tid;
    }

    TypeId
    CoapPacketTag::GetInstanceTypeId(void) const {
        return GetTypeId();
    }

    uint32_t
    CoapPacketTag::GetSerializedSize(void) const {
        NS_LOG_FUNCTION(this);
        return 4 + 8 + 4;
    }

    void
    CoapPacketTag::Serialize(TagBuffer i) const {
        i.WriteU32(m_seq);
        i.WriteU32(m_req);
        i.WriteU64(m_ts);
    }

    void
    CoapPacketTag::Deserialize(TagBuffer i) {
        m_seq = i.ReadU32();
        m_req = i.ReadU32();
        m_ts = i.ReadU64();
    }

    void
    CoapPacketTag::Print(std::ostream & os) const {
        NS_LOG_FUNCTION(this << &os);
        os << "(seq=" << m_seq << " time=" << TimeStep(m_ts).GetSeconds() << ")";
    }

    void
    CoapPacketTag::SetSeq(uint32_t seq) {
        NS_LOG_FUNCTION(this << seq);
        m_seq = seq;
    }

    uint32_t
    CoapPacketTag::GetSeq(void) const {
        NS_LOG_FUNCTION(this);
        return m_seq;
    }

    void
    CoapPacketTag::SetReq(uint32_t seq) {
        NS_LOG_FUNCTION(this << seq);
        m_req = seq;
    }

    uint32_t
    CoapPacketTag::GetReq(void) const {
        NS_LOG_FUNCTION(this);
        return m_req;
    }

    Time
    CoapPacketTag::GetTs(void) const {
        NS_LOG_FUNCTION(this);
        return TimeStep(m_ts);
    }

    uint64_t
    CoapPacketTag::GetT(void) const {
        NS_LOG_FUNCTION(this);
        return m_ts;
    }

}