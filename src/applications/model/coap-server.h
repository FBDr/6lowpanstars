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

#ifndef COAP_SERVER_H
#define COAP_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include <vector>
#include "ns3/callback.h"

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
    class CoapServer : public Application {
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

        void AddSeq(uint32_t newSeq);


        CoapServer();
        virtual ~CoapServer();

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
        uint32_t FilterReqNum(void);
        /**
         * \brief Check whether requested data is available at this server.
         */
        bool CheckReqAv(uint32_t);
        /**
         * \brief Fill data buffers for response packet with std::string.
         * \param string A string containing the packet content.
         */
        void CreateResponsePkt(std::string fill);

        uint16_t m_port; //!< Port on which we listen for incoming packets.
        Ptr<Socket> m_socket; //!< IPv4 Socket
        Ptr<Socket> m_socket6; //!< IPv6 Socket
        Address m_local; //!< local multicast address
        uint32_t m_dataSize; //!< packet payload size (must be equal to m_size)
        uint8_t *m_data; //!< packet payload data
        /// Callbacks for tracing the packet Tx events Waarvoor nuttig?
        //TracedCallback<Ptr<const Packet> > m_txTrace;
        uint32_t m_RdataSize; //!< packet payload size (must be equal to m_size)
        uint8_t *m_Rdata; //!< packet payload data
        //        uint32_t *m_regSeq; //!< Available sequence numbers.
        //        uint32_t m_regNum; //!< Available sequence numbers.
        std::set<uint32_t> m_regSeqSet;
    };

} // namespace ns3

#endif /* COAP_SERVER_H */

