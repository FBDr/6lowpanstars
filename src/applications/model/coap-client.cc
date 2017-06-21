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
#include "coap-client.h"


namespace ns3
{

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
                .AddAttribute("RemoteAddress",
                "The destination Address of the outbound packets",
                AddressValue(),
                MakeAddressAccessor(&CoapClient::m_peerAddress),
                MakeAddressChecker())
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

                //ZM
                .AddAttribute("NumberOfContents", "Number of the Contents in total", StringValue("100"),
                MakeUintegerAccessor(&CoapClient::SetNumberOfContents,
                &CoapClient::GetNumberOfContents),
                MakeUintegerChecker<uint32_t>())
                .AddAttribute("q", "parameter of improve rank", StringValue("0.7"),
                MakeDoubleAccessor(&CoapClient::SetQ,
                &CoapClient::GetQ),
                MakeDoubleChecker<double>())

                .AddAttribute("s", "parameter of power", StringValue("0.7"),
                MakeDoubleAccessor(&CoapClient::SetS,
                &CoapClient::GetS),
                MakeDoubleChecker<double>());
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
    CoapClient::SetRemote(Address ip, uint16_t port) {
        NS_LOG_FUNCTION(this << ip << port);
        m_peerAddress = ip;
        m_peerPort = port;
    }

    void
    CoapClient::SetRemote(Ipv4Address ip, uint16_t port) {
        NS_LOG_FUNCTION(this << ip << port);
        m_peerAddress = Address(ip);
        m_peerPort = port;
    }

    void
    CoapClient::SetRemote(Ipv6Address ip, uint16_t port) {
        NS_LOG_FUNCTION(this << ip << port);
        m_peerAddress = Address(ip);
        m_peerPort = port;
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
    CoapClient::SetIPv6Bucket(std::vector<Ipv6Address>& bucket) {
        std::vector<Ipv6Address> m_IPv6Bucket(bucket.size()+1);
        m_IPv6Bucket = bucket;

    }

    uint32_t
    CoapClient::GetNumberOfContents() const {
        return m_N;
    }

    void
    CoapClient::SetQ(double q) {
        m_q = q;
        SetNumberOfContents(m_N);
    }

    double
    CoapClient::GetQ() const {
        return m_q;
    }

    void
    CoapClient::SetS(double s) {
        m_s = s;
        SetNumberOfContents(m_N);
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

        if (m_socket == 0) {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
            m_socket = Socket::CreateSocket(GetNode(), tid);
            if (Ipv4Address::IsMatchingType(m_peerAddress) == true) {
                m_socket->Bind();
                //m_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort)); SendTo ipv Send, connect onnodig.
            } else if (Ipv6Address::IsMatchingType(m_peerAddress) == true) {
                m_socket->Bind6();
                //m_socket->Connect(Inet6SocketAddress(Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort)); SendTo ipv Send, connect onnodig.
            }
        }

        m_socket->SetRecvCallback(MakeCallback(&CoapClient::HandleRead, this));

        ScheduleTransmit(Seconds(0.));
    }

    void
    CoapClient::StopApplication() {
        NS_LOG_FUNCTION(this);

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

        uint32_t dataSize = fill.size() + 1;

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

        NS_ASSERT(m_sendEvent.IsExpired());
        SetFill("GET/" + std::to_string(GetNextSeq()));

        Ptr<Packet> p;
        if (m_dataSize) {
            //
            // If m_dataSize is non-zerol, we have a data buffer of the same size that we
            // are expected to copy and send.  This state of affairs is created if one of
            // the Fill functions is called.  In this case, m_size must have been set
            // to agree with m_dataSize
            //
            NS_ASSERT_MSG(m_dataSize == m_size, "CoapClient::Send(): m_size and m_dataSize inconsistent");
            NS_ASSERT_MSG(m_data, "CoapClient::Send(): m_dataSize but no m_data");
            p = Create<Packet> (m_data, m_dataSize);

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
        //m_socket->Send(p); Vervangen door SendTo

        if (m_socket->SendTo(p, 0, Inet6SocketAddress(Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort)) == -1) {
            std::cout<<"SendTo ERROR!"<<std::endl;
        };

        ++m_sent;

        if (Ipv4Address::IsMatchingType(m_peerAddress)) {
            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client sent " << m_size << " bytes to " <<
                    Ipv4Address::ConvertFrom(m_peerAddress) << " port " << m_peerPort);
        } else if (Ipv6Address::IsMatchingType(m_peerAddress)) {
            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client sent " << m_size << " bytes to " <<
                    Ipv6Address::ConvertFrom(m_peerAddress) << " port " << m_peerPort);
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
        while ((packet = socket->RecvFrom(from))) {
            if (InetSocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client received " << packet->GetSize() << " bytes from " <<
                        InetSocketAddress::ConvertFrom(from).GetIpv4() << " port " <<
                        InetSocketAddress::ConvertFrom(from).GetPort());
            } else if (Inet6SocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client received " << packet->GetSize() << " bytes from " <<
                        Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port " <<
                        Inet6SocketAddress::ConvertFrom(from).GetPort());
            }
        }
    }

} // Namespace ns3
