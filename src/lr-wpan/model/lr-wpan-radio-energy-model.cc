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

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/pointer.h"
#include "ns3/energy-source.h"
#include "lr-wpan-radio-energy-model.h"

NS_LOG_COMPONENT_DEFINE("LrWpanRadioEnergyModel");

namespace ns3{

    NS_OBJECT_ENSURE_REGISTERED(LrWpanRadioEnergyModel);

    TypeId
    LrWpanRadioEnergyModel::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::LrWpanRadioEnergyModel")
        .SetParent<DeviceEnergyModel> ()
        .AddConstructor<LrWpanRadioEnergyModel> ()
        .AddAttribute("TxCurrentA",
        "The radio Tx current in Ampere.",
        DoubleValue(0.0174), // transmit at 0dBm = 17.4 mA
        MakeDoubleAccessor(&LrWpanRadioEnergyModel::SetTxCurrentA,
        &LrWpanRadioEnergyModel::GetTxCurrentA),
        MakeDoubleChecker<double> ())
        .AddAttribute("RxCurrentA",
        "The radio Rx current in Ampere.",
        DoubleValue(0.0188), // receive mode = 18.8 mA
        MakeDoubleAccessor(&LrWpanRadioEnergyModel::SetRxCurrentA,
        &LrWpanRadioEnergyModel::GetRxCurrentA),
        MakeDoubleChecker<double> ())
        .AddAttribute("SleepCurrentA",
        "The radio Sleep current in Ampere.",
        DoubleValue(0.000426), // sleep mode = 20 uA, TRX_OFF mode 426 uA
        MakeDoubleAccessor(&LrWpanRadioEnergyModel::SetSleepCurrentA,
        &LrWpanRadioEnergyModel::GetSleepCurrentA),
        MakeDoubleChecker<double> ())
        .AddTraceSource("TotalEnergyConsumption",
        "Total energy consumption of the radio device.",
        MakeTraceSourceAccessor(&LrWpanRadioEnergyModel::m_totalEnergyConsumption),
        "ns3::TracedValue::DoubleCallback")
        ;
        return tid;
    }

    LrWpanRadioEnergyModel::LrWpanRadioEnergyModel()
    : m_totalEnergyConsumption(0)
    {
        NS_LOG_FUNCTION(this);
        m_currentState = IEEE_802_15_4_PHY_TRX_OFF; // initially SLEEP
        m_lastUpdateTime = Seconds(0.0);
        m_energyDepletionCallback.Nullify();
        m_source = NULL; // EnergySource
    }

    LrWpanRadioEnergyModel::~LrWpanRadioEnergyModel()
    {
        NS_LOG_FUNCTION(this);
    }

    void
    LrWpanRadioEnergyModel::AttachPhy(Ptr<LrWpanPhy> phy)
    {
        m_phy = phy;
        phy->RegisterListener(this);
    }

    void
    LrWpanRadioEnergyModel::SetEnergySource(Ptr<EnergySource> source)
    {
        NS_LOG_FUNCTION(this << source);
        NS_ASSERT(source != NULL);
        m_source = source;
    }

    double
    LrWpanRadioEnergyModel::GetTotalEnergyConsumption(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_totalEnergyConsumption;
    }

    double
    LrWpanRadioEnergyModel::GetTxCurrentA(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_txCurrentA;
    }

    void
    LrWpanRadioEnergyModel::SetTxCurrentA(double txCurrentA)
    {
        NS_LOG_FUNCTION(this << txCurrentA);
        m_txCurrentA = txCurrentA;
    }

    double
    LrWpanRadioEnergyModel::GetRxCurrentA(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_rxCurrentA;
    }

    void
    LrWpanRadioEnergyModel::SetRxCurrentA(double rxCurrentA)
    {
        NS_LOG_FUNCTION(this << rxCurrentA);
        m_rxCurrentA = rxCurrentA;
    }

    double
    LrWpanRadioEnergyModel::GetSleepCurrentA(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_sleepCurrentA;
    }

    void
    LrWpanRadioEnergyModel::SetSleepCurrentA(double sleepCurrentA)
    {
        NS_LOG_FUNCTION(this << sleepCurrentA);
        m_sleepCurrentA = sleepCurrentA;
    }

    LrWpanPhyEnumeration
    LrWpanRadioEnergyModel::GetCurrentState(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_currentState;
    }

    void
    LrWpanRadioEnergyModel::SetEnergyDepletionCallback(
    LrWpanRadioEnergyDepletionCallback callback)
    {
        NS_LOG_FUNCTION(this);
        if (callback.IsNull()) {
            NS_LOG_DEBUG("LrWpanRadioEnergyModel:Setting NULL energy depletion callback!");
        }
        m_energyDepletionCallback = callback;
    }

    void
    LrWpanRadioEnergyModel::SetEnergyRechargedCallback(
    LrWpanRadioEnergyRechargedCallback callback)
    {
        NS_LOG_FUNCTION(this);
        if (callback.IsNull()) {
            NS_LOG_DEBUG("LrWpanRadioEnergyModel:Setting NULL energy recharged callback!");
        }
        m_energyRechargedCallback = callback;
    }

    void
    LrWpanRadioEnergyModel::ChangeState(int newState)
    {
        NS_LOG_FUNCTION(this << newState);

        Time duration = Simulator::Now() - m_lastUpdateTime;
        NS_ASSERT(duration.GetNanoSeconds() >= 0); // check if duration is valid

        // energy to decrease = current * voltage * time
        double energyToDecrease = 0.0; //energyToDecrease = duration.GetSeconds () * DoGetCurrentA() * supplyVoltage;
        double supplyVoltage = m_source->GetSupplyVoltage();

        switch (m_currentState) {
            case IEEE_802_15_4_PHY_TX_ON:
                energyToDecrease = duration.GetSeconds() * m_txCurrentA * supplyVoltage;
                break;
            case IEEE_802_15_4_PHY_RX_ON:
                energyToDecrease = duration.GetSeconds() * m_rxCurrentA * supplyVoltage;
                break;
            case IEEE_802_15_4_PHY_TRX_OFF:
                energyToDecrease = duration.GetSeconds() * m_sleepCurrentA * supplyVoltage;
                break;
                // \todo use a better algorithm to guess the energy in this state
            case IEEE_802_15_4_PHY_UNSPECIFIED:
                energyToDecrease = duration.GetSeconds() * DoGetCurrentA() * supplyVoltage; //TRANSITION duration * current * supplyVoltage
                break;
            default:
                NS_FATAL_ERROR("LrWpanRadioEnergyModel:Undefined radio state: " << m_currentState);
                return;
        }

        // update total energy consumption
        m_totalEnergyConsumption += energyToDecrease;

        // update last update time stamp
        m_lastUpdateTime = Simulator::Now();

        // notify energy source
        m_source->UpdateEnergySource();

        // Call Sequence: BasicEnergySource::UpdateEnergySource -> BasicEnergySource::CalculateRemainingEnergy -> EnergySource::CalculateTotalCurrent ->
        // DeviceEnergyModel::GetCurrentA -> LrWpanRadioEnergyModel::DoGetCurrentA
        // If energy source is depleted, BasicEnergySource::UpdateEnergySource -> BasicEnergySource::HandleEnergyDrainedEvent -> 
        // EnergySource::NotifyEnergyDrained -> LrWpanRadioEnergyModel::HandleEnergyDepletion -> LrWpanPhy::EnergyDepletionHandler

        SetLrWpanRadioState((LrWpanPhyEnumeration) newState);
        NS_LOG_DEBUG("Current: " << DoGetCurrentA() << "A");
        NS_LOG_DEBUG("LrWpanRadioEnergyModel:Total energy consumption is " << m_totalEnergyConsumption << "J");
    }

    void
    LrWpanRadioEnergyModel::HandleEnergyDepletion(void)
    {
        NS_LOG_FUNCTION(this);
        NS_LOG_DEBUG("LrWpanRadioEnergyModel:Energy is depleted!");
        // invoke energy depletion callback, if set.
        if (!m_energyDepletionCallback.IsNull()) {
            m_energyDepletionCallback();
        }
        NS_ASSERT(m_phy != NULL);
        m_phy->EnergyDepletionHandler();
    }

    void
    LrWpanRadioEnergyModel::HandleEnergyRecharged(void)
    {
        NS_LOG_FUNCTION(this);
        NS_LOG_DEBUG("LrWpanRadioEnergyModel:Energy is recharged!");
        // invoke energy recharged callback, if set.
        if (!m_energyRechargedCallback.IsNull()) {
            m_energyRechargedCallback();
        }
        NS_ASSERT(m_phy != NULL);
        m_phy->EnergyRechargedHandler();
    }

    /*
     * Private functions start here.
     */

    void
    LrWpanRadioEnergyModel::DoDispose(void)
    {
        NS_LOG_FUNCTION(this);
        m_source = NULL;
        m_energyDepletionCallback.Nullify();
    }

    double
    LrWpanRadioEnergyModel::DoGetCurrentA(void) const
    {
        NS_LOG_FUNCTION(this);
        switch (m_currentState) {
            case IEEE_802_15_4_PHY_TX_ON:
                return m_txCurrentA;
            case IEEE_802_15_4_PHY_RX_ON:
                return m_rxCurrentA;
            case IEEE_802_15_4_PHY_TRX_OFF:
                return m_sleepCurrentA;
            case IEEE_802_15_4_PHY_UNSPECIFIED:
                switch (m_nextState) {
                    case IEEE_802_15_4_PHY_TX_ON:
                        return m_txCurrentA;
                    case IEEE_802_15_4_PHY_RX_ON:
                        return m_rxCurrentA;
                    case IEEE_802_15_4_PHY_TRX_OFF:
                        return m_sleepCurrentA;
                    default:
                        NS_FATAL_ERROR("LrWpanRadioEnergyModel:Undefined radio state:" << m_nextState);
                        return 0;
                }
            default:
                NS_FATAL_ERROR("LrWpanRadioEnergyModel:Undefined radio state:" << m_currentState);
                return 0;
        }
    }

    void
    LrWpanRadioEnergyModel::SetLrWpanRadioState(const LrWpanPhyEnumeration state)
    {
        NS_LOG_FUNCTION(this << state);
        m_currentState = state;
        std::string stateName;
        switch (state) {
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
                NS_FATAL_ERROR("LrWpanRadioEnergyModel:Undefined radio state:" << m_currentState);
                return;
        }
        NS_LOG_DEBUG("LrWpanRadioEnergyModel:Switching to state: " << stateName <<
        " at time = " << Simulator::Now());
    }

    // -------------------------------------------------------------------------- //

    void
    LrWpanRadioEnergyModel::NotifyRx()
    {
        NS_LOG_FUNCTION(this);
        ChangeState(IEEE_802_15_4_PHY_RX_ON);
    }

    void
    LrWpanRadioEnergyModel::NotifyRxStart()
    {
        NS_LOG_FUNCTION(this);
    }

    void
    LrWpanRadioEnergyModel::NotifyTx()
    {
        NS_LOG_FUNCTION(this);
        ChangeState(IEEE_802_15_4_PHY_TX_ON);
    }

    void
    LrWpanRadioEnergyModel::NotifyTxStart()
    {
        NS_LOG_FUNCTION(this);
    }

    void
    LrWpanRadioEnergyModel::NotifySleep(void)
    {
        NS_LOG_FUNCTION(this);
        ChangeState(IEEE_802_15_4_PHY_TRX_OFF);
    }

    void
    LrWpanRadioEnergyModel::NotifyTransition(LrWpanPhyEnumeration nextState)
    {
        NS_LOG_FUNCTION(this);
        m_nextState = (LrWpanPhyEnumeration) nextState;
        ChangeState(IEEE_802_15_4_PHY_UNSPECIFIED);
    }


} // namespace ns3
