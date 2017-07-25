/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 The Boeing Company
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
 * Authors:
 *  Gary Pei <guangyu.pei@boeing.com>
 *  kwong yin <kwong-sang.yin@boeing.com>
 *  Tom Henderson <thomas.r.henderson@boeing.com>
 *  Sascha Alexander Jopen <jopen@cs.uni-bonn.de>
 */
#ifndef LR_WPAN_NULLMAC_H
#define LR_WPAN_NULLMAC_H

#include "lr-wpan-mac.h"


namespace ns3 {

/**
 * \defgroup lr-wpan LR-WPAN models
 *
 * This section documents the API of the IEEE 802.15.4-related models.  For a generic functional description, please refer to the ns-3 manual.
 */

/**
 * \ingroup lr-wpan
 *
 * Class that implements the LR-WPAN Mac state machine
 */
class LrWpanNullMac : public LrWpanMac
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
  LrWpanNullMac (void);
  virtual ~LrWpanNullMac (void);

  /**
   * Check if the receiver will be enabled when the MAC is idle.
   *
   * \return true, if the receiver is enabled during idle periods, false otherwise
   */
  bool GetRxOnWhenIdle (void);

  /**
   * Set if the receiver should be enabled when the MAC is idle.
   *
   * \param rxOnWhenIdle set to true to enable the receiver during idle periods
   */
  void SetRxOnWhenIdle (bool rxOnWhenIdle);

  /**
   * CSMA-CA algorithm calls back the MAC after executing channel assessment.
   *
   * \param macState indicate BUSY oder IDLE channel condition
   */
  void SetLrWpanMacState (LrWpanMacState macState);

  //MAC sublayer constants
  /**
   * Length of a superframe slot in symbols. Defaults to 60 symbols in each
   * superframe slot.
   * See IEEE 802.15.4-2006, section 7.4.1, Table 85.
   */
  uint64_t m_aBaseSlotDuration;

  /**
   * Number of a superframe slots per superframe. Defaults to 16.
   * See IEEE 802.15.4-2006, section 7.4.1, Table 85.
   */
  uint64_t m_aNumSuperframeSlots;

  /**
   * Length of a superframe in symbols. Defaults to
   * aBaseSlotDuration * aNumSuperframeSlots in symbols.
   * See IEEE 802.15.4-2006, section 7.4.1, Table 85.
   */
  uint64_t m_aBaseSuperframeDuration;

  //MAC PIB attributes
  /**
   * The time that the device transmitted its last beacon frame, in symbol
   * periods. Only 24 bits used.
   * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
   */
  uint64_t m_macBeaconTxTime;

  /**
   * Symbol boundary is same as m_macBeaconTxTime.
   * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
   */
  uint64_t m_macSyncSymbolOffset;

  /**
   * Specification of how often the coordinator transmits its beacon.
   * 0 - 15 with 15 means no beacons are being sent.
   * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
   */
  uint64_t m_macBeaconOrder;

  /**
   * The length of the active portion of the outgoing superframe, including the
   * beacon frame.
   * 0 - 15 with 15 means the superframe will not be active after the beacon.
   * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
   */
  uint64_t m_macSuperframeOrder;

  /**
   * Indication of whether the MAC sublayer is to enable its receiver during
   * idle periods.
   * See IEEE 802.15.4-2006, section 7.4.2, Table 86.
   */
  bool m_macRxOnWhenIdle;
  
protected:
  // Inherited from Object.
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

private:

};


} // namespace ns3

#endif /* LR_WPAN_MAC_H */
