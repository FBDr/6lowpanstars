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
#ifndef LR_WPAN_CONTIKIMAC_H
#define LR_WPAN_CONTIKIMAC_H

#include "lr-wpan-mac.h"


namespace ns3 {

/**
 * \ingroup lr-wpan
 *
 * Class that implements the LR-WPAN Mac state machine
 */
class LrWpanContikiMac : public LrWpanMac
{
public:

  /**
   * Get the type ID.
   *
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Default constructor.
   */
  LrWpanContikiMac (void);
  virtual ~LrWpanContikiMac (void);

  /**
   * The maximum packet time (for ContikiMAC Fast Sleep).
   * The 2400 MHz PHY has a maximum packet time of 4.256 ms, and the 915 MHz PHY has a maximum packet time of 26.6 ms
   */
  static const Time maxPacketTime;

  // ContikiMAC variables.
  double m_sleepTime;   //!< Time asleep / wake-up interval
  Time m_ccaInterval;   //!< The interval between each CCA
  Time m_pktInterval;   //!< The interval between each packet transmission
  bool m_fastSleep;     //!< Enable the ContikiMAC fast sleep optimization

  /**
   * The maximum number of retries for ContikiMAC RDC.
   */
  uint8_t m_rdcMaxFrameRetries;

  // Override some base MAC functions.
  virtual void McpsDataRequest (McpsDataRequestParams params, Ptr<Packet> p);
  virtual void PdDataIndication (uint32_t psduLength, Ptr<Packet> p, uint8_t lqi);
  virtual void PdDataConfirm (LrWpanPhyEnumeration status);
  virtual void PlmeCcaConfirm (LrWpanPhyEnumeration status);
  virtual void PlmeSetTRXStateConfirm (LrWpanPhyEnumeration status);
  virtual void SetLrWpanMacState (LrWpanMacState macState);

protected:
  // Inherited from Object.
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

private:

  LrWpanPhyEnumeration m_currentState;  // current state the radio is in
  /**
   * \ingroup lr-wpan
   * \brief Neighbor description.
   *
   * This structure takes into account the known neighbors and their wakeup time (if known).
   */
  struct Neighbor
  {
    Address m_neighborAddress; //!< Neighbor address.
    Time m_wakeupTime;              //!< Wakeup time.
    uint8_t failedTransmissions;    //!< Number of failed transmissions.
  };

  // list of active neighbors
  typedef std::vector<Neighbor> Neighbors;
  Neighbors m_nb; //!< Known Neighbors.

  uint8_t m_ccaCount;   //!< To keep count of two CCAs
  bool m_broadcast;     //!< Is Broadcast transmission
  Time m_bcStart;       //!< Start time of broadcast packet
  Time m_nextWakeUp;    //!< Time to next wakeup
  Time m_pastTxStart;   //!< Start time of last unicast packet
  Time m_currentTxStart;//!< Start time of current unicast packet

  /**
   * Implements ContikiMAC sleep mechanism
   *
   */
  void Sleep (void);

  /**
   * Implements ContikiMAC wakeup mechanism
   *
   */
  void WakeUp (void);

  /**
   * Check phase-lock module if it has a recorded wake-up phase
   * of the intended receiver.
   *
   * \param address The Address of the intended receiver
   */
  Time CheckPlm (Address address);

  /**
   * Notify the phase-lock module the transmission time of the last packet
   * as an approximation of the wake-up phase of the receiver.
   *
   * @param address The Address of the receiver
   * @param txStart Start time of unicast packet
   */
  void NotifyPlm (Address address, Time txStart);

  /**
   * Sets current state.
   */
  void SetLrWpanRadioState (const LrWpanPhyEnumeration state);

  /**
   * Handle an RDC ACK timeout.
   */
  void PktRetransmissionTimeout (void);

  /**
   * Check the transmission queue. If there are packets in the transmission
   * queue and the MAC is idle, pick the first one and initiate a packet
   * transmission.
   *
   * \return true, if the Queue is empty, false otherwise.
   */
  virtual bool CheckQueue (void);

  /**
   * The number of already used retransmission for the currently transmitted
   * packet.
   */
  uint8_t m_rdcRetries;

  /**
   * Scheduler event of a scheduled WakeUp.
   */
  EventId m_wakeUp;

  /**
   * Scheduler event of a scheduled Sleep.
   */
  EventId m_sleep;

  /**
   * Scheduler event of a repeated data packet.
   */
  EventId m_repeatPkt;

};


} // namespace ns3

#endif /* LR_WPAN_CONTIKIMAC_H */
