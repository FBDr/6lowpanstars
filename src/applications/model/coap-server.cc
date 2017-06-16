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
#include "coap-server.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "src/network/model/node.h"
#include <string>

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("CoapServerApplication");

    NS_OBJECT_ENSURE_REGISTERED(CoapServer);

    TypeId
    CoapServer::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::CoapServer")
                .SetParent<Application> ()
                .SetGroupName("Applications")
                .AddConstructor<CoapServer> ()
                .AddAttribute("Port", "Port on which we listen for incoming packets.",
                UintegerValue(9),
                MakeUintegerAccessor(&CoapServer::m_port),
                MakeUintegerChecker<uint16_t> ())
                /*
                                .AddAttribute("DataNum", "Number of data pieces available at producer ", 
                                StringValue("0.7"),
                                MakeUintegerAccessor(&CoapServer::SetDataNum,
                                &CoapServer::GetDataNum),
                                MakeUintegerChecker<uint32_t> ())
                 */
                ;
        return tid;
    }

    CoapServer::CoapServer() {
        NS_LOG_FUNCTION(this);
    }

    CoapServer::~CoapServer() {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
        m_socket6 = 0;
    }

    void
    CoapServer::DoDispose(void) {
        NS_LOG_FUNCTION(this);
        Application::DoDispose();
    }

    void
    CoapServer::AddSeq(uint32_t newSeq) {
        m_regSeqVector.push_back(newSeq);
    }

    uint32_t
    CoapServer::FilterReqNum(void) {
        NS_ASSERT_MSG(m_Rdata, "Udp message is empty.");
        std::string Rdatastr(reinterpret_cast<char*> (m_Rdata), sizeof (m_Rdata));
        std::size_t pos = Rdatastr.find("/"); // position of "live" in str
        std::string str3 = Rdatastr.substr(pos + 1); // get from "live" to the end
        std::cout << str3 << std::endl;
        return std::stoi(str3);
    }

    /*
    void
    CoapServer::SetDataNum(uint32_t datanum) {
        NS_LOG_FUNCTION(this);
        m_regNum = datanum;
        m_regSeq = new uint32_t[datanum];
    }
    uint32_t
    CoapServer::GetDataNum(void) const {
        NS_LOG_FUNCTION(this);
        return m_regNum;
        std::vector test;
        test.

    }
     */


    void
    CoapServer::StartApplication(void) {
        NS_LOG_FUNCTION(this);

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

        m_socket->SetRecvCallback(MakeCallback(&CoapServer::HandleRead, this));
        m_socket6->SetRecvCallback(MakeCallback(&CoapServer::HandleRead, this));
    }

    void
    CoapServer::StopApplication() {
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
    CoapServer::HandleRead(Ptr<Socket> socket) {
        NS_LOG_FUNCTION(this << socket);

        Ptr<Packet> packet;
        uint32_t received_Req;
        Address from;


        while ((packet = socket->RecvFrom(from))) {
            if (InetSocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s server received " << packet->GetSize() << " bytes from " <<
                        InetSocketAddress::ConvertFrom(from).GetIpv4() << " port " <<
                        InetSocketAddress::ConvertFrom(from).GetPort());
            } else if (Inet6SocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s server received " << packet->GetSize() << " bytes from " <<
                        Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port " <<
                        Inet6SocketAddress::ConvertFrom(from).GetPort());
            }

            packet->RemoveAllPacketTags();
            packet->RemoveAllByteTags();


            // Copy data from current received packet into buffer. 
            delete [] m_Rdata; //nodig?
            m_Rdata = new uint8_t [packet->GetSize()];
            packet->CopyData(m_Rdata, packet->GetSize());
            received_Req = FilterReqNum()



            NS_LOG_LOGIC("Echoing packet");
            socket->SendTo(packet, 0, from);

            if (InetSocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s server sent " << packet->GetSize() << " bytes to " <<
                        InetSocketAddress::ConvertFrom(from).GetIpv4() << " port " <<
                        InetSocketAddress::ConvertFrom(from).GetPort());
            } else if (Inet6SocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s server sent " << packet->GetSize() << " bytes to " <<
                        Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port " <<
                        Inet6SocketAddress::ConvertFrom(from).GetPort());
            }
        }
    }

} // Namespace ns3
