/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Vishwesh Rege <vrege2012@gmail.com>
 */
#include "lr-wpan-contikimac.h"
#include "lr-wpan-csmaca.h"
#include "lr-wpan-mac-header.h"
#include "lr-wpan-mac-trailer.h"
#include <ns3/simulator.h>
#include <ns3/log.h>
#include <ns3/uinteger.h>
#include <ns3/boolean.h>
#include <ns3/node.h>
#include <ns3/packet.h>
#include <ns3/random-variable-stream.h>
#include <ns3/double.h>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                   \
  std::clog << "[address " << GetShortAddress () << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LrWpanContikiMac");

NS_OBJECT_ENSURE_REGISTERED (LrWpanContikiMac);

const Time LrWpanContikiMac::maxPacketTime = Seconds (0.004256);        // 2400 MHz PHY (915 MHz PHY has a maximum packet time of 26.6 ms)

TypeId
LrWpanContikiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LrWpanContikiMac")
    .SetParent<LrWpanMac> ()
    .SetGroupName ("LrWpan")
    .AddConstructor<LrWpanContikiMac> ()
    .AddAttribute ("SleepTime",
                   "Sleep interval",
                   DoubleValue (0.125),
                   MakeDoubleAccessor (&LrWpanContikiMac::m_sleepTime),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("CcaInterval",
                   "The interval between each CCA",
                   TimeValue (Seconds (0.0006)),
                   MakeTimeAccessor (&LrWpanContikiMac::m_ccaInterval),
                   MakeTimeChecker ())
    .AddAttribute ("PktInterval",
                   "The interval between each packet transmission",
                   TimeValue (Seconds (0.00055)),
                   MakeTimeAccessor (&LrWpanContikiMac::m_pktInterval),
                   MakeTimeChecker ())
    .AddAttribute ("RdcMaxRetries",
                   "The maximum number of retries for ContikiMAC RDC",
                   UintegerValue (4),    
                   MakeUintegerAccessor (&LrWpanContikiMac::m_rdcMaxFrameRetries),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("FastSleep",
                   "ContikiMAC fast sleep optimization",
                   BooleanValue (false),    
                   MakeBooleanAccessor (&LrWpanContikiMac::m_fastSleep),
                   MakeBooleanChecker ())
  ;
  return tid;
}

LrWpanContikiMac::LrWpanContikiMac ()
{
  m_ccaCount = 0;
  m_broadcast = false;
  m_bcStart = Seconds (0);
  m_nextWakeUp = Seconds (0);
  m_rdcRetries = 0;
  m_pastTxStart = Seconds (0);
  m_currentTxStart = Seconds (0);
}

LrWpanContikiMac::~LrWpanContikiMac ()
{
}

void
LrWpanContikiMac::DoInitialize ()
{
  //Sleep
  m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_TRX_OFF);

  LrWpanMac::DoInitialize ();
}

void
LrWpanContikiMac::DoDispose ()
{
  if (m_wakeUp.IsRunning ())
    {
      m_wakeUp.Cancel ();
    }

  LrWpanMac::DoDispose ();
}

void
LrWpanContikiMac::Sleep ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sleep.IsExpired ());
  if (m_lrWpanMacState == MAC_IDLE
      && (m_phy->GetTRXState () == IEEE_802_15_4_PHY_RX_ON || m_phy->GetTRXState () == IEEE_802_15_4_PHY_TX_ON))   //MAC_IDLE and TRX_ON
    {
      NS_LOG_DEBUG("Sleeping...");
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_TRX_OFF);
    }      
  else
    {
      Time sleepTime = maxPacketTime + m_pktInterval * 2;
      m_sleep = Simulator::Schedule (sleepTime, &LrWpanContikiMac::Sleep, this);
    }
}

void
LrWpanContikiMac::WakeUp ()
{
  NS_LOG_FUNCTION (this);

  if (m_phy->GetTRXState () == IEEE_802_15_4_PHY_TRX_OFF)
    {
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_RX_ON);
    }
}

Time
LrWpanContikiMac::CheckPlm (Address address)
{
  NS_ASSERT_MSG (Mac16Address::IsMatchingType (address) || Mac64Address::IsMatchingType (address), "Not a valid Address type");

  Neighbors::const_iterator it = m_nb.begin ();
  for (; it != m_nb.end (); it++)
  {
    if ((*it).m_neighborAddress == address)
      {
        return (*it).m_wakeupTime;
      }
  }
  
  return Seconds (0);
}

void
LrWpanContikiMac::NotifyPlm (Address address, Time txStart)
{
  NS_ASSERT_MSG (Mac16Address::IsMatchingType (address) || Mac64Address::IsMatchingType (address), "Not a valid Address type");

  Neighbors::const_iterator it = m_nb.begin ();
  for (; it != m_nb.end (); it++)       // Check if neighbor already present in list
  {
    if ((*it).m_neighborAddress == address)
      {
        return;
      }
  }

  Neighbor *nb = new Neighbor;
  (*nb).m_neighborAddress = address;
  (*nb).m_wakeupTime = txStart;
  (*nb).failedTransmissions = 0;
  m_nb.push_back (*nb); // Add new neighbor to phase-lock module
}

void
LrWpanContikiMac::McpsDataRequest (McpsDataRequestParams params, Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  NS_ASSERT_MSG (!GetCsmaCa ()->IsSlottedCsmaCa (), "ContikiMAC cannot be used in slotted mode");

  m_broadcast = false;
  if (params.m_dstAddrMode == SHORT_ADDR && params.m_dstAddr == Mac16Address ("ff:ff"))
    {
        m_broadcast = true;
    }

  LrWpanMac::McpsDataRequest (params, p);
}

bool
LrWpanContikiMac::CheckQueue ()
{
  NS_LOG_FUNCTION (this);

  // Pull a packet from the queue and start sending, if we are not already sending.
  if (m_lrWpanMacState == MAC_IDLE && !m_txQueue.empty () && m_txPkt == 0 && !m_setMacState.IsRunning ())
    {
      TxQueueElement *txQElement = m_txQueue.front ();
      m_txPkt = txQElement->txQPkt;
      Ptr<Packet> pkt = m_txPkt->Copy ();
      LrWpanMacHeader hdr;
      pkt->RemoveHeader (hdr);
      if (hdr.GetDstAddrMode () == SHORT_ADDR && hdr.GetShortDstAddr () == Mac16Address ("ff:ff"))      //Broadcast case
        {
          NS_LOG_DEBUG ("Broadcast, scheduling packet now");
          m_setMacState = Simulator::ScheduleNow (&LrWpanContikiMac::SetLrWpanMacState, this, MAC_CSMA);
        }
      else if (hdr.GetDstAddrMode () == SHORT_ADDR && CheckPlm (hdr.GetShortDstAddr ()) != Seconds (0))
        {
          NS_LOG_DEBUG ("Found neighbor in phase-lock module");
          Time diff = Simulator::Now () - CheckPlm (hdr.GetShortDstAddr ());
          double d = diff.GetDouble ()/m_sleepTime/(1000*1000*1000);
          uint32_t t = ceil (d);
          Time txTime = CheckPlm (hdr.GetShortDstAddr ()) + Seconds (t * m_sleepTime) - Simulator::Now ();
          NS_LOG_DEBUG ("t = " << t << " d = " << d << " diff = " << diff << " txTime: " << Simulator::Now () + txTime << " PLM: " << CheckPlm (hdr.GetShortDstAddr ()));
          m_setMacState = Simulator::Schedule (txTime, &LrWpanContikiMac::SetLrWpanMacState, this, MAC_CSMA);
        }
      else if (hdr.GetDstAddrMode () == EXT_ADDR && CheckPlm (hdr.GetExtDstAddr ()) != Seconds (0))
        {
          NS_LOG_DEBUG ("Found neighbor in phase-lock module");
          Time diff = Simulator::Now () - CheckPlm (hdr.GetExtDstAddr ());
          double d = diff.GetDouble ()/m_sleepTime/(1000*1000*1000);
          uint32_t t = ceil (d);
          Time txTime = CheckPlm (hdr.GetExtDstAddr ()) + Seconds (t * m_sleepTime) - Simulator::Now ();
          NS_LOG_DEBUG ("t = " << t << " d = " << d << " diff = " << diff << " txTime: " << Simulator::Now () + txTime << " PLM: " << CheckPlm (hdr.GetExtDstAddr ()));
          m_setMacState = Simulator::Schedule (txTime, &LrWpanContikiMac::SetLrWpanMacState, this, MAC_CSMA);
        }
      else      // Neighbor node not listed in phase-lock module
        {
          NS_LOG_DEBUG ("Neighbor node not listed, scheduling packet now");
          m_setMacState = Simulator::ScheduleNow (&LrWpanContikiMac::SetLrWpanMacState, this, MAC_CSMA);
        }
      return false;
    }

  return true;
}

void
LrWpanContikiMac::PdDataIndication (uint32_t psduLength, Ptr<Packet> p, uint8_t lqi)
{
  NS_ASSERT (m_lrWpanMacState == MAC_IDLE || m_lrWpanMacState == MAC_ACK_PENDING || m_lrWpanMacState == MAC_CSMA);

  NS_LOG_FUNCTION (this << psduLength << p << lqi);

  bool acceptFrame;

  // from sec 7.5.6.2 Reception and rejection, Std802.15.4-2006
  // level 1 filtering, test FCS field and reject if frame fails
  // level 2 filtering if promiscuous mode pass frame to higher layer otherwise perform level 3 filtering
  // level 3 filtering accept frame
  // if Frame type and version is not reserved, and
  // if there is a dstPanId then dstPanId=m_macPanId or broadcastPanI, and
  // if there is a shortDstAddr then shortDstAddr =shortMacAddr or broadcastAddr
  // if only srcAddr field in Data or Command frame,accept frame if srcPanId=m_macPanId

  Ptr<Packet> originalPkt = p->Copy (); // because we will strip headers

  m_promiscSnifferTrace (originalPkt);

  m_macPromiscRxTrace (originalPkt);
  // XXX no rejection tracing (to macRxDropTrace) being performed below

  LrWpanMacTrailer receivedMacTrailer;
  p->RemoveTrailer (receivedMacTrailer);
  if (Node::ChecksumEnabled ())
    {
      receivedMacTrailer.EnableFcs (true);
    }

  // level 1 filtering
  if (!receivedMacTrailer.CheckFcs (p))
    {
      m_macRxDropTrace (originalPkt);
    }
  else
    {
      LrWpanMacHeader receivedMacHdr;
      p->RemoveHeader (receivedMacHdr);

      McpsDataIndicationParams params;
      params.m_dsn = receivedMacHdr.GetSeqNum ();
      params.m_mpduLinkQuality = lqi;
      params.m_srcPanId = receivedMacHdr.GetSrcPanId ();
      params.m_srcAddrMode = receivedMacHdr.GetSrcAddrMode ();
      // TODO: Add field for EXT_ADDR source address.
      if (params.m_srcAddrMode == SHORT_ADDR)
        {
          params.m_srcAddr = receivedMacHdr.GetShortSrcAddr ();
        }
      params.m_dstPanId = receivedMacHdr.GetDstPanId ();
      params.m_dstAddrMode = receivedMacHdr.GetDstAddrMode ();
      // TODO: Add field for EXT_ADDR destination address.
      if (params.m_dstAddrMode == SHORT_ADDR)
        {
          params.m_dstAddr = receivedMacHdr.GetShortDstAddr ();
        }

      NS_LOG_DEBUG ("Packet from " << params.m_srcAddr << " to " << params.m_dstAddr);

      if (m_macPromiscuousMode)
        {
          //level 2 filtering
          if (!m_mcpsDataIndicationCallback.IsNull ())
            {
              NS_LOG_DEBUG ("promiscuous mode, forwarding up");
              m_mcpsDataIndicationCallback (params, p);
              if (m_lrWpanMacState == MAC_IDLE)
                {
                  NS_LOG_DEBUG("Fwd Up & Sleep");
                  m_sleep.Cancel ();
                  m_sleep = Simulator::ScheduleNow (&LrWpanContikiMac::Sleep, this);        //Sleep during promiscuous mode?
                }
            }
          else
            {
              NS_LOG_ERROR (this << " Data Indication Callback not initialised");
            }
        }
      else
        {
          //level 3 frame filtering
          acceptFrame = (receivedMacHdr.GetType () != LrWpanMacHeader::LRWPAN_MAC_RESERVED);

          if (acceptFrame)
            {
              acceptFrame = (receivedMacHdr.GetFrameVer () <= 1);
            }

          if (acceptFrame
              && (receivedMacHdr.GetDstAddrMode () > 1))
            {
              acceptFrame = receivedMacHdr.GetDstPanId () == m_macPanId
                || receivedMacHdr.GetDstPanId () == 0xffff;
            }

          if (acceptFrame
              && (receivedMacHdr.GetDstAddrMode () == 2))
            {
              acceptFrame = receivedMacHdr.GetShortDstAddr () == m_shortAddress
                || receivedMacHdr.GetShortDstAddr () == Mac16Address ("ff:ff");        // check for broadcast addrs
            }

          if (acceptFrame
              && (receivedMacHdr.GetDstAddrMode () == 3))
            {
              acceptFrame = (receivedMacHdr.GetExtDstAddr () == m_selfExt);
            }

          if (acceptFrame
              && ((receivedMacHdr.GetType () == LrWpanMacHeader::LRWPAN_MAC_DATA)
                  || (receivedMacHdr.GetType () == LrWpanMacHeader::LRWPAN_MAC_COMMAND))
              && (receivedMacHdr.GetSrcAddrMode () > 1))
            {
              acceptFrame = receivedMacHdr.GetSrcPanId () == m_macPanId; // \todo need to check if PAN coord
            }

          if (acceptFrame)
            {
              m_macRxTrace (originalPkt);
              // \todo: What should we do if we receive a frame while waiting for an ACK?
              //        Especially if this frame has the ACK request bit set, should we reply with an ACK, possibly missing the pending ACK?

              // If the received frame is a frame with the ACK request bit set, we immediately send back an ACK.
              // If we are currently waiting for a pending ACK, we assume the ACK was lost and trigger a retransmission after sending the ACK.
              if ((receivedMacHdr.IsData () || receivedMacHdr.IsCommand ()) && receivedMacHdr.IsAckReq ()
                  && !(receivedMacHdr.GetDstAddrMode () == SHORT_ADDR && receivedMacHdr.GetShortDstAddr () == "ff:ff"))
                {
                  // If this is a data or mac command frame, which is not a broadcast,
                  // with ack req set, generate and send an ack frame.
                  // If there is a CSMA medium access in progress we cancel the medium access
                  // for sending the ACK frame. A new transmission attempt will be started
                  // after the ACK was send.
                  if (m_lrWpanMacState == MAC_ACK_PENDING)
                    {
                      if (m_repeatPkt.IsRunning ()) 
                        {
                          NS_LOG_DEBUG("ContikiMAC pkt retransmission cancelled");
                          m_repeatPkt.Cancel ();
                          m_rdcRetries = 0;
                        }
                      m_ackWaitTimeout.Cancel ();       //assume the ACK was lost and 
                      PrepareRetransmission ();         //trigger a retransmission (after sending the ACK for the received frame)
                    }
                  else if (m_lrWpanMacState == MAC_CSMA)
                    {
                      // \todo: If we receive a packet while doing CSMA/CA, should  we drop the packet because of channel busy,
                      //        or should we restart CSMA/CA for the packet after sending the ACK?
                      // Currently we simply restart CSMA/CA after sending the ACK.
                      m_csmaCa->Cancel ();
                    }
                  // Cancel any pending MAC state change, ACKs have higher priority.
                  m_setMacState.Cancel ();
                  ChangeMacState (MAC_IDLE);
                  m_setMacState = Simulator::ScheduleNow (&LrWpanContikiMac::SendAck, this, receivedMacHdr.GetSeqNum ());
                }

              if (receivedMacHdr.IsData () && !m_mcpsDataIndicationCallback.IsNull ())
                {
                  // If it is a data frame, push it up the stack.
                  NS_LOG_DEBUG ("PdDataIndication():  Packet is for me; forwarding up");
                  m_mcpsDataIndicationCallback (params, p);
                  if (m_lrWpanMacState == MAC_IDLE)
                    {
                      if (!receivedMacHdr.IsAckReq () || receivedMacHdr.GetShortDstAddr () == "ff:ff")        //If Ack not required, go to sleep
                        {
                          NS_LOG_DEBUG("Fwd Up & Sleep");
                          m_sleep.Cancel ();
                          m_sleep = Simulator::ScheduleNow (&LrWpanContikiMac::Sleep, this);
                        }
                      else m_sleep.Cancel ();
                    }
                }
              else if (receivedMacHdr.IsAcknowledgment () && m_txPkt && m_lrWpanMacState == MAC_ACK_PENDING)
                {
                  NS_LOG_DEBUG("Acked");
                  LrWpanMacHeader macHdr;
                  m_txPkt->PeekHeader (macHdr);
                  if (receivedMacHdr.GetSeqNum () == macHdr.GetSeqNum ())
                    {
                      m_macTxOkTrace (m_txPkt);
                      // If it is an ACK with the expected sequence number, finish the transmission
                      // and notify the upper layer.
                      m_ackWaitTimeout.Cancel ();
                      if (m_repeatPkt.IsRunning ()) 
                        {
                          NS_LOG_DEBUG("ContikiMAC pkt retransmission cancelled");
                          m_repeatPkt.Cancel ();
                          m_rdcRetries = 0;
                          Time phase = m_pastTxStart;
                          m_pastTxStart = m_currentTxStart = Seconds (0);
                          Address addr;
                          if (macHdr.GetDstAddrMode () == SHORT_ADDR)
                            {
                              addr = macHdr.GetShortDstAddr ();
                            }
                          else
                            {
                              addr = macHdr.GetExtDstAddr ();
                            }
                          NotifyPlm (addr, phase);
                          NS_LOG_DEBUG ("Phase: " << phase);
                          if (CheckPlm (addr) != Seconds (0))
                            {
                              NS_LOG_DEBUG("Check Plm: " << addr);
                            }
                        }
                      if (!m_mcpsDataConfirmCallback.IsNull ())
                        {
                          TxQueueElement *txQElement = m_txQueue.front ();
                          McpsDataConfirmParams confirmParams;
                          confirmParams.m_msduHandle = txQElement->txQMsduHandle;
                          confirmParams.m_status = IEEE_802_15_4_SUCCESS;
                          m_mcpsDataConfirmCallback (confirmParams);
                        }
                      RemoveFirstTxQElement ();
                      m_setMacState.Cancel ();
                      m_setMacState = Simulator::ScheduleNow (&LrWpanContikiMac::SetLrWpanMacState, this, MAC_IDLE);
                    }
                  else
                    {
                      // If it is an ACK with an unexpected sequence number, mark the current transmission as failed and start a retransmit. (cf 7.5.6.4.3)
                      m_ackWaitTimeout.Cancel ();
                      if (m_repeatPkt.IsRunning ()) 
                        {
                          NS_LOG_DEBUG("ContikiMAC pkt retransmission cancelled");
                          m_repeatPkt.Cancel ();
                          m_rdcRetries = 0;
                        }
                      if (!PrepareRetransmission ())
                        {
                          m_setMacState.Cancel ();
                          m_setMacState = Simulator::ScheduleNow (&LrWpanContikiMac::SetLrWpanMacState, this, MAC_IDLE);
                        }
                      else
                        {
                          m_setMacState.Cancel ();
                          m_setMacState = Simulator::ScheduleNow (&LrWpanContikiMac::SetLrWpanMacState, this, MAC_CSMA);
                        }
                    }
                }   //IsAcknowledgment
            }       //acceptFrame
          else
            {
              m_macRxDropTrace (originalPkt);
              NS_LOG_DEBUG("Drop & Sleep");
              m_sleep.Cancel ();
              m_sleep = Simulator::ScheduleNow (&LrWpanContikiMac::Sleep, this);
            }
        }           //!m_macPromiscuousMode
    }
}

void
LrWpanContikiMac::PktRetransmissionTimeout (void)
{
  NS_LOG_FUNCTION (this);

  if (m_broadcast)
    {
      NS_ASSERT (m_lrWpanMacState == MAC_SENDING && m_phy->GetTRXState () == IEEE_802_15_4_PHY_TX_ON);
      NS_LOG_DEBUG("Broadcasting...");
      m_macTxTrace (m_txPkt);
      m_phy->PdDataRequest (m_txPkt->GetSize (), m_txPkt);
      return;
    }    

  if (m_rdcRetries < m_rdcMaxFrameRetries)
    {   
      m_rdcRetries++;
      ChangeMacState (MAC_SENDING);
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_TX_ON);
    }
  else
    {
      NS_LOG_DEBUG("Inform MAC");
      m_rdcRetries = 0;
      AckWaitTimeout ();
    }
}

void
LrWpanContikiMac::PdDataConfirm (LrWpanPhyEnumeration status)
{
  NS_ASSERT (m_lrWpanMacState == MAC_SENDING);

  NS_LOG_FUNCTION (this << status << m_txQueue.size ());

  LrWpanMacHeader macHdr;
  m_txPkt->PeekHeader (macHdr);
  if (status == IEEE_802_15_4_PHY_SUCCESS)
    {
      if (!macHdr.IsAcknowledgment ())
        {
          // We have just send a regular data packet, check if we have to wait
          // for an ACK.
          if (macHdr.IsAckReq ())
            {
              //No ackWaitTimeout, RDC informs MAC about failure (No ACK)
              m_setMacState.Cancel ();
              m_setMacState = Simulator::ScheduleNow (&LrWpanContikiMac::SetLrWpanMacState, this, MAC_ACK_PENDING);     //actually RDC Ack Pending

              //ContikiMAC: data packet transmissions are repeated
              NS_ASSERT (m_repeatPkt.IsExpired ());
              m_repeatPkt = Simulator::Schedule (m_pktInterval, &LrWpanContikiMac::PktRetransmissionTimeout, this);

              return;
            }
          else if (m_broadcast && ((Simulator::Now () - m_bcStart) < Seconds (m_sleepTime)))
            {
              NS_ASSERT (m_repeatPkt.IsExpired ());
              m_repeatPkt = Simulator::Schedule (m_pktInterval, &LrWpanContikiMac::PktRetransmissionTimeout, this);
            } 
          else
            {
              if (m_broadcast)
                {
                  NS_LOG_DEBUG("Stop broadcasting");
                  m_broadcast = false;
                  m_bcStart = Seconds (0);
                }
              m_macTxOkTrace (m_txPkt);
              // remove the copy of the packet that was just sent
              if (!m_mcpsDataConfirmCallback.IsNull ())
                {
                  McpsDataConfirmParams confirmParams;
                  NS_ASSERT_MSG (m_txQueue.size () > 0, "TxQsize = 0");
                  TxQueueElement *txQElement = m_txQueue.front ();
                  confirmParams.m_msduHandle = txQElement->txQMsduHandle;
                  confirmParams.m_status = IEEE_802_15_4_SUCCESS;
                  m_mcpsDataConfirmCallback (confirmParams);
                }
              RemoveFirstTxQElement ();
            }
        }
      else
        {
          // We have send an ACK. Clear the packet buffer.
          m_txPkt = 0;
        }
    }
  else if (status == IEEE_802_15_4_PHY_UNSPECIFIED)     //Not in standard, all cases not covered by ieee802.15.4
    {
      NS_LOG_DEBUG("UNSPECIFIED");
      if (!macHdr.IsAcknowledgment ())
        {
          NS_ASSERT_MSG (m_txQueue.size () > 0, "TxQsize = 0");
          TxQueueElement *txQElement = m_txQueue.front ();
          m_macTxDropTrace (txQElement->txQPkt);
          if (!m_mcpsDataConfirmCallback.IsNull ())
            {
              McpsDataConfirmParams confirmParams;
              confirmParams.m_msduHandle = txQElement->txQMsduHandle;
              confirmParams.m_status = IEEE_802_15_4_FRAME_TOO_LONG;
              m_mcpsDataConfirmCallback (confirmParams);
            }
          RemoveFirstTxQElement ();
        }
      else
        {
          NS_LOG_ERROR ("Unable to send ACK");
        }
    }
  else
    {
      // Something went really wrong. The PHY is not in the correct state for
      // data transmission.
      NS_FATAL_ERROR ("Transmission attempt failed with PHY status " << status);
    }

  if (!m_broadcast)
    {
      NS_LOG_DEBUG("Set MacState = IDLE");
      m_setMacState.Cancel ();
      m_setMacState = Simulator::ScheduleNow (&LrWpanContikiMac::SetLrWpanMacState, this, MAC_IDLE);
    }
}

void
LrWpanContikiMac::PlmeCcaConfirm (LrWpanPhyEnumeration status)
{
  NS_LOG_FUNCTION (this << status);

  if (m_lrWpanMacState == MAC_IDLE)
    {
      m_ccaCount++;
      NS_LOG_DEBUG("MAC_IDLE");                               
      if (status == IEEE_802_15_4_PHY_IDLE)
        {
          if (m_ccaCount == 2) 
            {
              NS_LOG_DEBUG("Phy Idle twice");
              m_ccaCount = 0;
              NS_ASSERT (m_sleep.IsExpired ());
              m_sleep = Simulator::ScheduleNow (&LrWpanContikiMac::Sleep, this);        //vs. Sleep ();
            }
          else
            {
              NS_LOG_DEBUG("Phy Idle");
              Simulator::Schedule (m_ccaInterval, &LrWpanPhy::PlmeCcaRequest, m_phy);
            }
        }
      else if (status == IEEE_802_15_4_PHY_BUSY)
        {
          NS_LOG_DEBUG("Phy Busy");
          Time sleepTime = maxPacketTime + m_pktInterval * 2;
          NS_ASSERT (m_sleep.IsExpired ());
          m_sleep = Simulator::Schedule (sleepTime, &LrWpanContikiMac::Sleep, this);
          m_ccaCount = 0;
          //Keep radio ON
        }  
                         
    }
        
  // Direct this call through the csmaCa object
  if (m_lrWpanMacState == MAC_CSMA)
    {
      NS_LOG_DEBUG("csmaCa");                               
      m_csmaCa->PlmeCcaConfirm (status);
    }
}

void
LrWpanContikiMac::PlmeSetTRXStateConfirm (LrWpanPhyEnumeration status)
{
  NS_LOG_FUNCTION (this << status);
  if (status == IEEE_802_15_4_PHY_SUCCESS)      //state change
    {          
      if (m_wakeUp.IsRunning ())        //If woke up to send packet, cancel pending wakeup
        {
          NS_ASSERT (m_phy->GetTRXState () == IEEE_802_15_4_PHY_TX_ON || m_phy->GetTRXState () == IEEE_802_15_4_PHY_RX_ON);
          NS_LOG_DEBUG("WakeUp Cancelled");
          m_wakeUp.Cancel ();
        }
    }
    
  if (m_lrWpanMacState == MAC_SENDING && (status == IEEE_802_15_4_PHY_TX_ON || status == IEEE_802_15_4_PHY_SUCCESS))
    {
      NS_ASSERT (m_txPkt);

      if (m_broadcast)
        {
          NS_LOG_DEBUG("Starting broadcast now");
          m_bcStart = Simulator::Now ();
        }
      else 
        {
          NS_LOG_DEBUG("Starting pkt tx now");
	  if (m_currentTxStart != Seconds (0))
            {
              m_pastTxStart = m_currentTxStart;
              m_currentTxStart = Simulator::Now ();
	    }
	  else 
	    {
	      m_currentTxStart = Simulator::Now ();
            }
        }
    }
  else if (m_lrWpanMacState == MAC_IDLE)
    {
      NS_ASSERT (status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS || status == IEEE_802_15_4_PHY_TRX_OFF);
      if (m_phy->GetTRXState () == IEEE_802_15_4_PHY_TRX_OFF)
        {
          Time upTime = Seconds (0);
          if (Simulator::Now () > m_nextWakeUp)
            {
              int m = 1;
              while (upTime <= Seconds (0))
                {
                  upTime = Seconds (m * m_sleepTime) + m_nextWakeUp - Simulator::Now ();    //Keep incr. 'm' if missed > 1 wake-ups
                  m++;
                }
              NS_LOG_DEBUG("WakeUp Missed! next wake-up: " << Simulator::Now () + upTime << " nWU: " << m_nextWakeUp);
            }
          else if (Simulator::Now () < m_nextWakeUp)
            {
              upTime = m_nextWakeUp - Simulator::Now ();
              NS_LOG_DEBUG("next WakeUp: " << Simulator::Now () + upTime);
            }
          else  //will be equal only at start
            {
              upTime = Seconds (m_sleepTime);
              NS_LOG_DEBUG("next WakeUp (init): " << Simulator::Now () + upTime);
            }
          m_wakeUp = Simulator::Schedule (upTime, &LrWpanContikiMac::WakeUp, this);
          m_nextWakeUp = Simulator::Now () + upTime;
          NS_LOG_DEBUG("WakeUp " << m_nextWakeUp);
        }
        //For ContikiMAC, perform CCA after wakeup
      else if (m_phy->GetTRXState () == IEEE_802_15_4_PHY_RX_ON)
        {
          NS_LOG_DEBUG("WakeUp-CCA");
          m_phy->PlmeCcaRequest ();
        }
	  return;
    }

  LrWpanMac::PlmeSetTRXStateConfirm (status);
}

void
LrWpanContikiMac::SetLrWpanMacState (LrWpanMacState macState)
{
  NS_LOG_FUNCTION (this << "mac state = " << macState);

  McpsDataConfirmParams confirmParams;

  if (macState == MAC_IDLE)
    {
      ChangeMacState (MAC_IDLE);
      bool qEmpty = CheckQueue ();
      if (qEmpty)
        {
          NS_LOG_DEBUG("MAC Idle, TxQueue Empty, Scheduling Sleep");
          m_sleep.Cancel ();
          m_sleep = Simulator::ScheduleNow (&LrWpanContikiMac::Sleep, this);
        }
    }
  else if (macState == MAC_ACK_PENDING)
    {
      ChangeMacState (MAC_ACK_PENDING);
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_RX_ON);
    }
  else if (macState == MAC_CSMA)
    {
      NS_ASSERT (m_lrWpanMacState == MAC_IDLE || m_lrWpanMacState == MAC_ACK_PENDING);
      NS_LOG_DEBUG("Mac_Csma");
      ChangeMacState (MAC_CSMA);
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_RX_ON);
    }
  else if (m_lrWpanMacState == MAC_CSMA && macState == CHANNEL_IDLE)
    {
      // Channel is idle, set transmitter to TX_ON

      ChangeMacState (MAC_SENDING);
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_TX_ON);
    }
  else if (m_lrWpanMacState == MAC_CSMA && macState == CHANNEL_ACCESS_FAILURE)
    {
      NS_ASSERT (m_txPkt);

      // cannot find a clear channel, drop the current packet.
      NS_LOG_DEBUG ( this << " cannot find clear channel");
      confirmParams.m_msduHandle = m_txQueue.front ()->txQMsduHandle;
      confirmParams.m_status = IEEE_802_15_4_CHANNEL_ACCESS_FAILURE;
      m_macTxDropTrace (m_txPkt);
      if (!m_mcpsDataConfirmCallback.IsNull ())
        {
          m_mcpsDataConfirmCallback (confirmParams);
        }
      // remove the copy of the packet that was just sent
      RemoveFirstTxQElement ();

      ChangeMacState (MAC_IDLE);
    }
}

void
LrWpanContikiMac::SetLrWpanRadioState (const LrWpanPhyEnumeration state)
{
  NS_LOG_FUNCTION (this << state);
  m_currentState = state;
  std::string stateName;
  switch (state)
    {
    case IEEE_802_15_4_PHY_TX_ON:
      stateName = "TX_ON";
      break;
    case IEEE_802_15_4_PHY_RX_ON:
      stateName = "RX_ON";
      break;
    case IEEE_802_15_4_PHY_TRX_OFF:
      stateName = "TRX_OFF";
      break;
    case IEEE_802_15_4_PHY_UNSPECIFIED:
      stateName = "TRANSITION";
      break;
    default:
      NS_FATAL_ERROR ("LrWpanRadioEnergyModel:Undefined radio state:" << m_currentState);
    }
  NS_LOG_DEBUG ("LrWpanContikiMac:Switching to state: " << stateName << " at time = " << Simulator::Now ());
}

} // namespace ns3
