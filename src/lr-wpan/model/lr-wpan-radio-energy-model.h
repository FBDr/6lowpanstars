/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#ifndef LRWPAN_RADIO_ENERGY_MODEL_H
#define LRWPAN_RADIO_ENERGY_MODEL_H

#include "ns3/device-energy-model.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/traced-value.h"
#include "lr-wpan-phy.h"

namespace ns3 {

class LrWpanRadioEnergyModel : public DeviceEnergyModel, public LrWpanPhyListener
{
public:
  /**
   * Callback type for energy depletion handling.
   */
  typedef Callback<void> LrWpanRadioEnergyDepletionCallback;

  /**
   * Callback type for energy recharged handling.
   */
  typedef Callback<void> LrWpanRadioEnergyRechargedCallback;

  static TypeId GetTypeId (void);
  LrWpanRadioEnergyModel ();
  virtual ~LrWpanRadioEnergyModel ();

  /**
   * \brief Switches the LrWpanRadioEnergyModel to RX_ON state.
   *
   * Implements LrWpanPhyListener::NotifyRx
   */
  virtual void NotifyRx (void);

  /**
   * \brief Switches the LrWpanRadioEnergyModel to BUSY_RX state.
   *
   * Implements LrWpanPhyListener::NotifyRxStart
   */
  virtual void NotifyRxStart (void);

  /**
   * \brief Switches the LrWpanRadioEnergyModel to TX_ON state.
   *
   * Implements LrWpanPhyListener::NotifyTx
   */
  virtual void NotifyTx (void);

  /**
   * \brief Switches the LrWpanRadioEnergyModel to BUSY_TX state.
   *
   * Implements LrWpanPhyListener::NotifyTxStart
   */
  virtual void NotifyTxStart (void);

  /**
   * \brief Switches the LrWpanRadioEnergyModel to TRX_OFF state.
   *
   * Implements LrWpanPhyListener::NotifySleep
   */
  virtual void NotifySleep (void);

  /**
   * \brief Switches the LrWpanRadioEnergyModel to Transition state.
   *
   * \param nextState The state that the Phy is switching to.
   *
   * Implements LrWpanPhyListener::NotifyTransition
   */
  virtual void NotifyTransition (LrWpanPhyEnumeration nextState);

  /**
   * \param phy Pointer to PHY layer attached to device.
   *
   * Registers the LrWpanRadioEnergyModel as listener to the Phy.
   */
  virtual void AttachPhy (Ptr<LrWpanPhy> phy);

  /**
   * \brief Sets pointer to EnergySouce installed on node.
   *
   * \param source Pointer to EnergySource installed on node.
   *
   * Implements DeviceEnergyModel::SetEnergySource.
   */
  virtual void SetEnergySource (Ptr<EnergySource> source);

  /**
   * \returns Total energy consumption of the LrWpan device.
   *
   * Implements DeviceEnergyModel::GetTotalEnergyConsumption.
   */
  virtual double GetTotalEnergyConsumption (void) const;

  // Setter & getters for state power consumption.
  double GetTxCurrentA (void) const;
  void SetTxCurrentA (double txCurrentA);
  double GetRxCurrentA (void) const;
  void SetRxCurrentA (double rxCurrentA);
  double GetSleepCurrentA (void) const;
  void SetSleepCurrentA (double sleepCurrentA);

  /**
   * \returns Current state.
   */
  LrWpanPhyEnumeration GetCurrentState (void) const;

  /**
   * \param callback Callback function.
   *
   * Sets callback for energy depletion handling.
   */
  void SetEnergyDepletionCallback (LrWpanRadioEnergyDepletionCallback callback);

  /**
   * \param callback Callback function.
   *
   * Sets callback for energy recharged handling.
   */
  void SetEnergyRechargedCallback (LrWpanRadioEnergyRechargedCallback callback);

  /**
   * \brief Changes state of the LrWpanRadioEnergyMode.
   *
   * \param newState New state the LrWpan radio is in.
   *
   * Implements DeviceEnergyModel::ChangeState.
   */
  virtual void ChangeState (int newState);

  /**
   * \brief Handles energy depletion.
   *
   * Implements DeviceEnergyModel::HandleEnergyDepletion
   */
  virtual void HandleEnergyDepletion (void);

  /**
   * \brief Handles energy recharged.
   *
   * Implements DeviceEnergyModel::HandleEnergyRecharged
   */
  virtual void HandleEnergyRecharged (void);

private:
  void DoDispose (void);

  /**
   * \returns Current draw of device, at current state.
   *
   * Implements DeviceEnergyModel::GetCurrentA.
   */
  virtual double DoGetCurrentA (void) const;

  /**
   * \param state New state the radio device is currently in.
   *
   * Sets current state. This function is private so that only the energy model
   * can change its own state.
   */
  void SetLrWpanRadioState (const LrWpanPhyEnumeration state);

  // The Energy Source and PHY associated with this model
  Ptr<EnergySource> m_source;
  Ptr<LrWpanPhy> m_phy;

  // Member variables for current draw in different radio modes.
  double m_txCurrentA;
  double m_rxCurrentA;
  double m_sleepCurrentA;

  /**
   * Variable to keep track of the total energy consumed.
   */
  TracedValue<double> m_totalEnergyConsumption;

  // State variables.
  LrWpanPhyEnumeration m_currentState;  //!< current state the radio is in
  LrWpanPhyEnumeration m_nextState;  //!< next state after transition
  Time m_lastUpdateTime;          //!< time stamp of previous energy update

  /**
   * Energy depletion callback.
   */
  LrWpanRadioEnergyDepletionCallback m_energyDepletionCallback;

  /**
   * Energy recharged callback.
   */
  LrWpanRadioEnergyRechargedCallback m_energyRechargedCallback;

};

} // namespace ns3

#endif /* LRWPAN_RADIO_ENERGY_MODEL_H */
