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
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/coap-client.h"
#include "ns3/coap-packet-tag.h"
#include "ns3/ipv6-packet-info-tag.h"
#include "src/network/model/node.h"
#include <fstream>
#include "ns3/node.h"
#include "ns3/address-utils.h"
#include "ns3/udp-socket.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include <string>






namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("CoapClientApplication");

    NS_OBJECT_ENSURE_REGISTERED(CoapClient);

    TypeId
    CoapClient::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::CoapClient")
                .SetParent<Application> ()
                .SetGroupName("Applications")
                .AddConstructor<CoapClient> ()
                .AddAttribute("MaxPackets",
                "The maximum number of packets the application will send",
                UintegerValue(100),
                MakeUintegerAccessor(&CoapClient::m_count),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("Interval",
                "The time to wait between packets",
                TimeValue(Seconds(1.0)),
                MakeTimeAccessor(&CoapClient::m_interval),
                MakeTimeChecker())
                .AddAttribute("RemotePort",
                "The destination port of the outbound packets",
                UintegerValue(0),
                MakeUintegerAccessor(&CoapClient::m_peerPort),
                MakeUintegerChecker<uint16_t> ())
                .AddAttribute("PacketSize", "Size of echo data in outbound packets",
                UintegerValue(100),
                MakeUintegerAccessor(&CoapClient::SetDataSize,
                &CoapClient::GetDataSize),
                MakeUintegerChecker<uint32_t> ())
                .AddTraceSource("Tx", "A new packet is created and is sent",
                MakeTraceSourceAccessor(&CoapClient::m_txTrace),
                "ns3::Packet::TracedCallback")
                .AddAttribute("useIPCache",
                "Must be set if IP caching is used",
                BooleanValue(false),
                MakeBooleanAccessor(&CoapClient::m_useIPcache),
                MakeBooleanChecker())

                //ZM
                .AddAttribute("q", "parameter of improve rank", StringValue("0.7"),
                MakeDoubleAccessor(&CoapClient::SetQ,
                &CoapClient::GetQ),
                MakeDoubleChecker<double>())

                .AddAttribute("s", "parameter of power", StringValue("0.7"),
                MakeDoubleAccessor(&CoapClient::SetS,
                &CoapClient::GetS),
                MakeDoubleChecker<double>())
                .AddAttribute("NumberOfContents", "Number of the Contents in total", StringValue("100"),
                MakeUintegerAccessor(&CoapClient::SetNumberOfContents,
                &CoapClient::GetNumberOfContents),
                MakeUintegerChecker<uint32_t>())
                .AddAttribute("RngStream", "Set Rng stream.", StringValue("-1"),
                MakeUintegerAccessor(&CoapClient::SetRngStream,
                &CoapClient::GetRngStream),
                MakeUintegerChecker<uint32_t>());

        ;
        return tid;
    }

    CoapClient::CoapClient()
    : m_N(100) // needed here to make sure when SetQ/SetS are called, there is a valid value of N
    , m_q(0.7)
    , m_s(0.7)
    , m_seqRng(CreateObject<UniformRandomVariable>()) {
        NS_LOG_FUNCTION(this);
        m_sent = 0;
        m_received = 0;
        m_socket = 0;
        m_sendEvent = EventId();
        m_data = 0;
        m_dataSize = 0;
    }

    CoapClient::~CoapClient() {
        NS_LOG_FUNCTION(this);
        m_socket = 0;

        delete [] m_data;
        m_data = 0;
        m_dataSize = 0;
    }

    void
    CoapClient::DoDispose(void) {
        NS_LOG_FUNCTION(this);
        Application::DoDispose();
    }

    void
    CoapClient::SetNumberOfContents(uint32_t numOfContents) {
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
        }
    }

    void
    CoapClient::SetRngStream(uint32_t stream) {
        m_seqRng->SetStream((int64_t) stream);

    }

    uint32_t
    CoapClient::GetRngStream() const {
        return (uint32_t) m_seqRng->GetStream();
    }

    void
    CoapClient::SetIPv6Bucket(std::vector<Ipv6Address> bucket) {
        m_IPv6Bucket = std::vector<Ipv6Address> (bucket.size() + 1);
        m_IPv6Bucket = bucket;
        // SetNumberOfContents((uint32_t)bucket.size());

    }

    void
    CoapClient::SetNodeToGtwMap(std::map<Ipv6Address, Ipv6Address> gtw_to_node) {

        m_gtw_to_node = gtw_to_node;
    }

    uint32_t
    CoapClient::GetNumberOfContents() const {
        return m_N;
    }

    void
    CoapClient::SetQ(double q) {
        m_q = q;
        NS_LOG_INFO("Setting q to: " << q);
    }

    double
    CoapClient::GetQ() const {
        return m_q;
    }

    void
    CoapClient::SetS(double s) {
        m_s = s;
        NS_LOG_INFO("Setting s to: " << s);
    }

    double
    CoapClient::GetS() const {
        return m_s;
    }

    uint32_t
    CoapClient::GetNextSeq() {
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
    CoapClient::StartApplication(void) {
        NS_LOG_FUNCTION(this);
        Ptr <Node> PtrNode = this->GetNode();
        Ptr<Ipv6> ipv6 = PtrNode->GetObject<Ipv6> ();
        Ipv6InterfaceAddress ownaddr = ipv6->GetAddress(1, 1);
        m_ownip = ownaddr.GetAddress();
        m_ownGTW = m_gtw_to_node[m_ownip];
        if (m_socket == 0) {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
            m_socket = Socket::CreateSocket(GetNode(), tid);
            m_socket->Bind6();

        }

        m_socket->SetRecvCallback(MakeCallback(&CoapClient::HandleRead, this));
        m_socket->SetIpv6RecvHopLimit(true);
        ScheduleTransmit(Seconds(0.));
    }

    void
    CoapClient::StopApplication() {
        NS_LOG_FUNCTION(this);
        float pktloss = 100.0 - (float) m_received / (float) m_sent * 100.0;
        std::ofstream outfile;
        outfile.open("pktloss.txt", std::ios_base::app);
        outfile << GetNode()->GetId() << " " << m_sent << " " << m_received << " " << pktloss << std::endl;

        NS_LOG_INFO("Total transmitted packets: " << m_sent);
        NS_LOG_INFO("Total received packets: " << m_received);
        NS_LOG_INFO("Packet loss: " << pktloss);

        if (m_socket != 0) {
            m_socket->Close();
            m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> > ());
            m_socket = 0;
        }

        Simulator::Cancel(m_sendEvent);
    }

    void
    CoapClient::SetDataSize(uint32_t dataSize) {
        NS_LOG_FUNCTION(this << dataSize);

        //
        // If the client is setting the echo packet data size this way, we infer
        // that she doesn't care about the contents of the packet at all, so 
        // neither will we.
        //
        delete [] m_data;
        m_data = 0;
        m_dataSize = 0;
        m_size = dataSize;
    }

    uint32_t
    CoapClient::GetDataSize(void) const {
        NS_LOG_FUNCTION(this);
        return m_size;
    }

    void
    CoapClient::SetFill(std::string fill) {
        NS_LOG_FUNCTION(this << fill);

        uint32_t dataSize = 8 + fill.size(); //CoAp GET message header is 8 bytes long.

        if (dataSize != m_dataSize) {
            delete [] m_data;
            m_data = new uint8_t [dataSize];
            m_dataSize = dataSize;
        }

        memcpy(m_data, fill.c_str(), dataSize);

        //
        // Overwrite packet size attribute.
        //
        m_size = dataSize;
    }

    void
    CoapClient::SetFill(uint8_t fill, uint32_t dataSize) {
        NS_LOG_FUNCTION(this << fill << dataSize);
        if (dataSize != m_dataSize) {
            delete [] m_data;
            m_data = new uint8_t [dataSize];
            m_dataSize = dataSize;
        }

        memset(m_data, fill, dataSize);

        //
        // Overwrite packet size attribute.
        //
        m_size = dataSize;
    }

    void
    CoapClient::SetFill(uint8_t *fill, uint32_t fillSize, uint32_t dataSize) {
        NS_LOG_FUNCTION(this << fill << fillSize << dataSize);
        if (dataSize != m_dataSize) {
            delete [] m_data;
            m_data = new uint8_t [dataSize];
            m_dataSize = dataSize;
        }

        if (fillSize >= dataSize) {
            memcpy(m_data, fill, dataSize);
            m_size = dataSize;
            return;
        }

        //
        // Do all but the final fill.
        //
        uint32_t filled = 0;
        while (filled + fillSize < dataSize) {
            memcpy(&m_data[filled], fill, fillSize);
            filled += fillSize;
        }

        //
        // Last fill may be partial
        //
        memcpy(&m_data[filled], fill, dataSize - filled);

        //
        // Overwrite packet size attribute.
        //
        m_size = dataSize;
    }

    void
    CoapClient::ScheduleTransmit(Time dt) {
        NS_LOG_FUNCTION(this << dt);
        m_sendEvent = Simulator::Schedule(dt, &CoapClient::Send, this);
    }

    void
    CoapClient::Send(void) {
        NS_LOG_FUNCTION(this);
        CoapPacketTag coaptag;
        uint32_t nxtsq;
        NS_ASSERT(m_sendEvent.IsExpired());
        do {
            nxtsq = GetNextSeq() - 1; //Next sequence spans from [1, N];
            NS_LOG_DEBUG("IP from bucket:" << m_IPv6Bucket[nxtsq] << " My own ip: " << m_ownip);
        } while (m_IPv6Bucket[nxtsq] == m_ownip);
        coaptag.SetReq(nxtsq);
        coaptag.SetSeq(m_sent);
        coaptag.SetHop(64);
        SetFill("Sensordata/" + std::to_string(nxtsq));
        NS_LOG_INFO("Added REQ to label: " << coaptag.GetReq());
        Ptr<Packet> p;
        if (m_dataSize) {
            //
            // If m_dataSize is non-zero, we have a data buffer of the same size that we
            // are expected to copy and send.  This state of affairs is created if one of
            // the Fill functions is called.  In this case, m_size must have been set
            // to agree with m_dataSize
            //
            NS_ASSERT_MSG(m_dataSize == m_size, "CoapClient::Send(): m_size and m_dataSize inconsistent");
            NS_ASSERT_MSG(m_data, "CoapClient::Send(): m_dataSize but no m_data");
            p = Create<Packet> (m_data, m_dataSize); // To Do should be changed later.
            p->AddPacketTag(coaptag);
        } else {
            //
            // If m_dataSize is zero, the client has indicated that it doesn't care
            // about the data itself either by specifying the data size by setting
            // the corresponding attribute or by not calling a SetFill function.  In
            // this case, we don't worry about it either.  But we do allow m_size
            // to have a value different from the (zero) m_dataSize.
            //
            p = Create<Packet> (m_size);
        }
        // call to the trace sinks before the packet is actually sent,
        // so that tags added to the packet can be sent as well
        m_txTrace(p);

        if (m_gtw_to_node[m_IPv6Bucket[nxtsq]] == m_ownGTW || !m_useIPcache) {
            NS_LOG_INFO("Transmitting directly/internal node.");
            if (m_socket->SendTo(p, 0, Inet6SocketAddress(m_IPv6Bucket[nxtsq], m_peerPort)) == -1) {
                NS_LOG_INFO("SendTo ERROR! Trying to send to: " << m_IPv6Bucket[nxtsq]);
            };
        } else {
            NS_LOG_INFO("Node is at different gateway, transmitting to own GTW.");
            if (m_socket->SendTo(p, 0, Inet6SocketAddress(m_ownGTW, m_peerPort)) == -1) {
                NS_LOG_INFO("SendTo ERROR! Trying to send to ownGTW:" << m_ownGTW);
            };
        }


        ++m_sent;
        //std::cout<<"s: "<< m_sent<<std::endl;
        m_PenSeqSet.insert(nxtsq);
        if (Ipv6Address::IsMatchingType(m_IPv6Bucket[nxtsq])) {
            NS_LOG_INFO("At time " << Simulator::Now().GetNanoSeconds() << "s client sent " << m_size << " bytes to " <<
                    m_IPv6Bucket[nxtsq] << " port " << m_peerPort);
        }
        if (m_sent < m_count) {
            ScheduleTransmit(m_interval);
        }
    }

    void
    CoapClient::HandleRead(Ptr<Socket> socket) {
        NS_LOG_FUNCTION(this << socket);
        Ptr<Packet> packet;
        Address from;
        CoapPacketTag coaptag;
        SocketIpv6HopLimitTag hoplimitTag;

        while ((packet = socket->RecvFrom(from))) {
            if (InetSocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO("At time " << Simulator::Now().GetNanoSeconds() << "s client received " << packet->GetSize() << " bytes from " <<
                        InetSocketAddress::ConvertFrom(from).GetIpv4() << " port " <<
                        InetSocketAddress::ConvertFrom(from).GetPort());
                m_received++;
            } else if (Inet6SocketAddress::IsMatchingType(from)) {
                packet->RemovePacketTag(coaptag);
                packet->RemovePacketTag(hoplimitTag);
                NS_LOG_INFO("Pending sequence list contains " << m_PenSeqSet.size() << " entries before Rx.");
                uint32_t cur_req = coaptag.GetReq();
                NS_LOG_INFO("Req Tag: " << (int) cur_req);
                NS_LOG_INFO("Seq Tag: " << coaptag.GetSeq());
                // NS_LOG_INFO("IT: " << m_PenSeqSet[m_PenSeqSet.find(coaptag.GetReq())]);

                if (m_PenSeqSet.find(coaptag.GetReq()) != m_PenSeqSet.end()) { //Check whether this packet was requested by this client application.
                    Time e2edelay = Simulator::Now() - coaptag.GetTs();
                    int64_t delay = e2edelay.GetMilliSeconds();
                    int hops = (int) (64 - coaptag.GetHop()) + (int) (64 - hoplimitTag.GetHopLimit());
                    NS_LOG_INFO("At time " << Simulator::Now().GetNanoSeconds() << "s client received " << packet->GetSize() << " bytes from " <<
                            Inet6SocketAddress::ConvertFrom(from).GetIpv6() << ", port " <<
                            Inet6SocketAddress::ConvertFrom(from).GetPort() << ", E2E Delay:" << e2edelay.GetMilliSeconds() << " ms, Hopcount (64): "
                            << hops);

                    PrintToFile(coaptag.GetTs().GetSeconds(), hops, delay);

                    m_received++;
                    m_PenSeqSet.erase(m_PenSeqSet.find(coaptag.GetReq()));
                }
                NS_LOG_INFO("Pending sequence list contains " << m_PenSeqSet.size() << " entries after Rx.");
            }
        }
    }

    void
    CoapClient::PrintToFile(double txtime, int &hops, int64_t & delay) {
        Ptr <Node> PtrNode = this->GetNode();
        uint32_t nodenum = PtrNode->GetId();
        std::ofstream outfile;
        outfile.open("hopdelay.txt", std::ios_base::app);
        outfile << nodenum << " " << txtime << " " << Simulator::Now().GetSeconds() << " " << hops << " " << delay << std::endl;
    }


} // Namespace ns3
