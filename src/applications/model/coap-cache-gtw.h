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

#ifndef COAP_CACHE_GTW_H
#define COAP_CACHE_GTW_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include <vector>
#include "ns3/callback.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-callback.h"
#include "ns3/coap-packet-tag.h"

namespace ns3 {

    class Socket;
    class Packet;

    /**
     * \ingroup applications 
     * \defgroup coap Coap
     */

    /**
     * \ingroup coap
     * \brief A Udp Echo server
     *
     * Every packet received is sent back.
     */
    class CoapCacheGtw : public Application {
    public:
        /**
         * \brief Get the type ID.
         * \return the object TypeId
         */
        static TypeId GetTypeId(void);
        /**
         * \brief Set the total number of data chunks available at this server.
         * 
         */
        //void SetDataNum(uint32_t datanum);
        /**
         * \brief Get the total number of data chunks available at this server.
         * 
         */
        //uint32_t GetDataNum() const;


        void SetIPv6Bucket(std::vector<Ipv6Address> bucket);
        void SetReportTime(int time);
        int GetReportTime(void) const;
        void CacheHit(Ptr<Socket> socket, Ptr<Packet> received_packet, uint32_t & sq, CoapPacketTag & coaptag, Address &from);
        void CacheMiss(Ptr<Socket> socket, Ptr<Packet> received_packet, uint32_t &sq, CoapPacketTag &coaptag, Address &from);
        void PrintToFile(void);
        CoapCacheGtw();
        bool InCache(uint32_t in_seq);
        void AddToCache(uint32_t cache_seq);
        virtual ~CoapCacheGtw();

    protected:
        virtual void DoDispose(void);

    private:

        virtual void StartApplication(void);
        virtual void StopApplication(void);


        /**
         * \brief Handle a packet reception.
         *
         * This function is called by lower layers.
         *
         * \param socket the socket the packet was received to.
         */
        void HandleRead(Ptr<Socket> socket);
        /**
         * \brief Filter request number out of received payload.
         */
        uint32_t FilterReqNum(uint32_t size);
        /**
         * \brief Update cache contents by checking for expired freshness.
         */
        void UpdateCache(void);
        /**
         * \brief Save CU metric to file.
         */        
        void SaveToFile(uint32_t context);
        /**
         * \brief Fill data buffers for response packet with std::string.
         * \param string A string containing the packet content.
         */
        void CreateResponsePkt(std::string fill, uint32_t);


        uint16_t m_port; //!< Port on which we listen for incoming packets.
        uint32_t m_fresh;
        uint32_t m_cache_size;
        Ptr<Socket> m_socket; //!< IPv4 Socket
        Ptr<Socket> m_socket6; //!< IPv6 Socket
        Address m_local; //!< local multicast address
        uint32_t m_dataSize; //!< packet payload size (must be equal to m_size)
        uint8_t *m_data; //!< packet payload data
        uint32_t m_packet_payload_size; //!< packet payload size (must be equal to m_size)
        /// Callbacks for tracing the packet Tx events Waarvoor nuttig?
        TracedCallback<Ptr<const Packet> > m_txTrace;
        uint32_t m_RdataSize; //!< packet payload size (must be equal to m_size)
        uint8_t *m_Rdata; //!< packet payload data
        uint32_t m_cur_max;
        double m_mov_av;
        double m_count;
        int m_report_time;
        bool m_w_flag;
        Time m_report_time_T;
        EventId m_event_save;



        std::vector<std::pair<uint32_t, Time>> m_cache; //Sequence, time
        std::vector<std::tuple<Address, uint64_t, uint32_t>> m_pendingreqs; //Adress, time, sequence
        std::vector<Ipv6Address> m_IPv6Bucket;
        Ipv6Address m_ownip;
    };

} // namespace ns3

#endif /* COAP_CACHE_GTW_H */

