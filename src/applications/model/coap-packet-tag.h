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
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/nstime.h"
#include <iostream>

#ifndef COAP_TAG_HEADER_H
#define COAP_TAG_HEADER_H


namespace ns3 {

    // define this class in a public header

    class CoapPacketTag : public Tag {
    public:
        static TypeId GetTypeId(void);
        virtual TypeId GetInstanceTypeId(void) const;
        virtual uint32_t GetSerializedSize(void) const;
        virtual void Serialize(TagBuffer i) const;
        virtual void Deserialize(TagBuffer i);
        virtual void Print(std::ostream &os) const;

        // these are our accessors to our tag structure
        CoapPacketTag();
        void SetSeq(uint32_t);
        uint32_t GetSeq(void) const;
        void SetReq(uint32_t);
        uint32_t GetReq(void) const;
        Time GetTs(void) const;
        uint64_t GetT(void) const;
    private:
        uint32_t m_seq; //!< Sequence number
        uint32_t m_req; //!< Request number
        uint64_t m_ts; //!< Timestamp
    };

}

#endif