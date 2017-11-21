/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <fstream>
#include "ns3/core-module.h"
#include "coap-cache-gtw.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "src/network/model/node.h"
#include "ns3/node.h"
#include "src/core/model/log.h"
#include <string>
#include "ns3/coap-packet-tag.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("CoapCacheGtwApplication");

    NS_OBJECT_ENSURE_REGISTERED(CoapCacheGtw);

    TypeId
    CoapCacheGtw::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::CoapCacheGtw")
                .SetParent<Application> ()
                .SetGroupName("Applications")
                .AddConstructor<CoapCacheGtw> ()
                .AddAttribute("Port", "Port on which we listen for incoming packets.",
                UintegerValue(9),
                MakeUintegerAccessor(&CoapCacheGtw::m_port),
                MakeUintegerChecker<uint16_t> ())
                .AddAttribute("Freshness", "Freshness setting cache in seconds.",
                UintegerValue(1),
                MakeUintegerAccessor(&CoapCacheGtw::m_fresh),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("CacheSize", "Size of cache defined in number of items.",
                UintegerValue(3),
                MakeUintegerAccessor(&CoapCacheGtw::m_cache_size),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("Payload", "Response data packet payload size.",
                UintegerValue(100),
                MakeUintegerAccessor(&CoapCacheGtw::m_packet_payload_size),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("ReportTime",
                "Time at which CS statistics report must be written",
                StringValue("100"), MakeIntegerAccessor(&CoapCacheGtw::GetReportTime, 
                &CoapCacheGtw::SetReportTime),
                MakeIntegerChecker<int>())

                .AddTraceSource("Tx", "A new packet is created and is sent",
                MakeTraceSourceAccessor(&CoapCacheGtw::m_txTrace),
                "ns3::Packet::TracedCallback")

                /*
                                .AddAttribute("DataNum", "Number of data pieces available at producer ", 
                                StringValue("0.7"),
                                MakeUintegerAccessor(&CoapCacheGtw::SetDataNum,
                                &CoapCacheGtw::GetDataNum),
                                MakeUintegerChecker<uint32_t> ())
                 */
                ;
        return tid;
    }

    CoapCacheGtw::CoapCacheGtw()
    : m_cur_max(0)
    , m_mov_av(0)
    , m_count(1)
    , m_w_flag(0) {
        NS_LOG_FUNCTION(this);
    }

    CoapCacheGtw::~CoapCacheGtw() {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
        m_socket6 = 0;
    }

    void
    CoapCacheGtw::SetIPv6Bucket(std::vector<Ipv6Address> bucket) {
        m_IPv6Bucket = std::vector<Ipv6Address> (bucket.size() + 1);
        m_IPv6Bucket = bucket;

    }

    void
    CoapCacheGtw::SetReportTime(int time) {
        m_report_time = time;
        Time temptime(std::to_string(time) + "s");
        m_report_time_T = temptime;
    }

    int
    CoapCacheGtw::GetReportTime() const {
        return m_report_time;
    }

    void
    CoapCacheGtw::DoDispose(void) {
        NS_LOG_FUNCTION(this);
        Application::DoDispose();
    }

    uint32_t
    CoapCacheGtw::FilterReqNum(uint32_t size) {

        // This function filters the request number from the received payload.

        NS_ASSERT_MSG(m_Rdata, "Udp message is empty.");
        std::string Rdatastr(reinterpret_cast<char*> (m_Rdata), size);
        if (Rdatastr.find("POST") != std::string::npos) {
            return std::numeric_limits<uint32_t>::max();
        } else {
            std::size_t pos = Rdatastr.find("/"); // Find position of "/" leading the sequence number.
            std::string str3 = Rdatastr.substr(pos + 1); // filter this sequence number to the end.
            return std::stoi(str3);
        }
    }

    void
    CoapCacheGtw::UpdateCache() {
        Time fresh(std::to_string(m_fresh) + "s");

        std::vector<std::pair<uint32_t, Time>>::iterator itr = m_cache.begin();

        while (itr != m_cache.end()) {
            Time lifetime = Simulator::Now() - itr->second;
            //NS_LOG_DEBUG("Lifetime of current cached item: " << itr->first << " " << lifetime.GetMilliSeconds());
            if (Simulator::Now() - itr->second >= fresh) {
                itr = m_cache.erase(itr);
                //NS_LOG_DEBUG("Deleting entry");
            } else {
                itr++;
                //NS_LOG_DEBUG("Valid entry");
            }

        }
    }

    bool
    CoapCacheGtw::InCache(uint32_t in_seq) {
        //Simnple function to check is sequence is currently in cache.
        UpdateCache();
        for (auto itr = m_cache.begin(); itr != m_cache.end(); itr++) {
            if (itr->first == in_seq) {
                std::rotate(itr, itr + 1, m_cache.end());
                for (auto itr2 = m_cache.begin(); itr2 != m_cache.end(); itr2++) {
                    //NS_LOG_INFO("Cache reordered: " << itr2->first);
                }
                return true;
            }
        }
        return false;
    }

    void
    CoapCacheGtw::AddToCache(uint32_t cache_seq) {
        if ((int) m_cache.size() >= (int) m_cache_size) {
            //The cache is full.
            //NS_LOG_DEBUG("Cache full, deleting first entry.");
            m_cache.erase(m_cache.begin()); //Delete first entry
        }
        m_cache.push_back(std::make_pair(cache_seq, Simulator::Now()));
        for (auto itr2 = m_cache.begin(); itr2 != m_cache.end(); itr2++) {
            //NS_LOG_INFO("Entry in cache: " << itr2->first);
        }

        std::ofstream outfile;
        long unsigned int cur_size = m_cache.size();
        outfile.open("cu_ip.txt", std::ios_base::app);

        if (cur_size > m_cur_max) {
            m_cur_max = cur_size;
        }
        m_mov_av = m_mov_av + ((double) cur_size - m_mov_av) / m_count;
        m_count++;

        if ((Simulator::Now() >= m_report_time_T) && m_w_flag == false) {
            outfile << Simulator::GetContext() << " " << m_mov_av << " " << m_cur_max << std::endl;
            m_w_flag = true;
        }
    }

    void
    CoapCacheGtw::CreateResponsePkt(std::string fill, uint32_t length) {

        //When a Coap request is correctly received, this function will generate a response packet.
        NS_LOG_FUNCTION(this << fill);
        NS_ASSERT_MSG(length >= fill.size(), "Length of data packet smaller than string size.");
        uint32_t dataSize = length;
        m_data = new uint8_t [dataSize];
        m_dataSize = dataSize;
        memcpy(m_data, fill.c_str(), dataSize);
    }

    void
    CoapCacheGtw::StartApplication(void) {
        NS_LOG_FUNCTION(this);
        Ptr <Node> PtrNode = this->GetNode();
        Ptr<Ipv6> ipv6 = PtrNode->GetObject<Ipv6> ();
        Ipv6InterfaceAddress ownaddr = ipv6->GetAddress(1, 1);
        m_ownip = ownaddr.GetAddress();

        if (m_socket == 0) {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
            m_socket = Socket::CreateSocket(GetNode(), tid);
            InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
            m_socket->Bind(local);
            if (addressUtils::IsMulticast(m_local)) {
                Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
                if (udpSocket) {
                    // equivalent to setsockopt (MCAST_JOIN_GROUP)
                    udpSocket->MulticastJoinGroup(0, m_local);
                } else {
                    NS_FATAL_ERROR("Error: Failed to join multicast group");
                }
            }
        }

        if (m_socket6 == 0) {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
            m_socket6 = Socket::CreateSocket(GetNode(), tid);
            Inet6SocketAddress local6 = Inet6SocketAddress(Ipv6Address::GetAny(), m_port);
            m_socket6->Bind(local6);
            if (addressUtils::IsMulticast(local6)) {
                Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket6);
                if (udpSocket) {
                    // equivalent to setsockopt (MCAST_JOIN_GROUP)
                    udpSocket->MulticastJoinGroup(0, local6);
                } else {
                    NS_FATAL_ERROR("Error: Failed to join multicast group");
                }
            }
        }

        m_socket->SetRecvCallback(MakeCallback(&CoapCacheGtw::HandleRead, this));
        m_socket6->SetRecvCallback(MakeCallback(&CoapCacheGtw::HandleRead, this));

    }

    void
    CoapCacheGtw::StopApplication() {
        NS_LOG_FUNCTION(this);

        if (m_socket != 0) {
            m_socket->Close();
            m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> > ());
        }
        if (m_socket6 != 0) {
            m_socket6->Close();
            m_socket6->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> > ());
        }
    }

    void
    CoapCacheGtw::HandleRead(Ptr<Socket> socket) {
        NS_LOG_FUNCTION(this << socket);

        Ptr<Packet> received_packet;
        Ptr<Packet> response_packet;
        Address from;

        while ((received_packet = socket->RecvFrom(from))&& (Inet6SocketAddress::ConvertFrom(from).GetIpv6() != m_ownip)) {

            if (InetSocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s server received " << received_packet->GetSize() << " bytes from " <<
                        InetSocketAddress::ConvertFrom(from).GetIpv4() << " port " <<
                        InetSocketAddress::ConvertFrom(from).GetPort());
            } else if (Inet6SocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s server received " << received_packet->GetSize() << " bytes from " <<
                        Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port " <<
                        Inet6SocketAddress::ConvertFrom(from).GetPort());
            }

            CoapPacketTag coaptag;

            received_packet->RemovePacketTag(coaptag);
            received_packet->RemoveAllPacketTags();
            received_packet->RemoveAllByteTags();

            Time e2edelay = Simulator::Now() - coaptag.GetTs();
            int64_t delay = e2edelay.GetMilliSeconds();
            NS_LOG_INFO("Currently received packet delay " << delay);
            m_Rdata = new uint8_t [received_packet->GetSize()];
            received_packet->CopyData(m_Rdata, received_packet->GetSize());
            uint32_t received_Req = FilterReqNum(received_packet->GetSize());

            // Check if in cache and message type
            if (received_Req == std::numeric_limits<uint32_t>::max()) {

                //Its a returning data packet

                //Add data seq to cache
                for (int idx = 0; idx < ((int) m_pendingreqs.size()); idx++) {
                    if (std::get<1>(m_pendingreqs[idx]) == coaptag.GetT()) {
                        //Found entry
                        received_packet->AddPacketTag(coaptag);
                        NS_LOG_INFO("Found return entry transmission Succes?: " << socket->SendTo(received_packet, 0, std::get<0>(m_pendingreqs[idx])));
                        m_pendingreqs.erase(m_pendingreqs.begin() + idx);

                        AddToCache(std::get<2>(m_pendingreqs[idx]));
                        NS_LOG_INFO("Cache insert! " << std::get<2>(m_pendingreqs[idx]) << " Size: " << m_cache.size());
                        break;
                    }
                }
            } else if (InCache(received_Req)) {
                CacheHit(socket, received_packet, received_Req, coaptag, from);
            } else {
                CacheMiss(socket, received_packet, received_Req, coaptag, from);
            }
        }
    }

    void
    CoapCacheGtw::CacheHit(Ptr<Socket> socket, Ptr<Packet> received_packet, uint32_t & sq, CoapPacketTag & coaptag, Address & from) {
        NS_LOG_INFO("Cache hit!");
        Ptr<Packet> response_packet;

        NS_LOG_INFO("Transmitting data from gateway cache: " << sq);
        CreateResponsePkt("POST", m_packet_payload_size);
        response_packet = Create<Packet> (m_data, m_dataSize);
        response_packet->AddPacketTag(coaptag);
        PrintToFile();
        NS_LOG_LOGIC("Sending packet: Succes?" << socket->SendTo(response_packet, 0, from));


    }

    void
    CoapCacheGtw::CacheMiss(Ptr<Socket> socket, Ptr<Packet> received_packet, uint32_t & sq, CoapPacketTag & coaptag, Address & from) {
        NS_LOG_INFO("Cache miss! Transmitting to: " << m_IPv6Bucket[sq] << " SEQ: " << sq);
        m_pendingreqs.push_back(std::make_tuple(from, coaptag.GetT(), sq));
        Packet repsonsep(*received_packet);
        repsonsep.AddPacketTag(coaptag);
        NS_LOG_INFO("Succes?: " << socket->SendTo(&repsonsep, 0, Inet6SocketAddress(m_IPv6Bucket[sq], m_port)));
    }

    void
    CoapCacheGtw::PrintToFile() {
        Ptr <Node> PtrNode = this->GetNode();
        std::ofstream outfile;
        outfile.open("cache_hits.txt", std::ios_base::app);
        outfile << PtrNode->GetId() << " " << ns3::Simulator::Now().GetSeconds() << std::endl;
    }



} // Namespace ns3
