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

#include "ns3/basic-energy-source.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/energy-source-container.h"
#include "ns3/lr-wpan-radio-energy-model.h"
#include "ns3/lr-wpan-phy.h"
#include "ns3/device-energy-model-container.h"
#include "ns3/log.h"
#include "ns3/test.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LrWpanEnergyModelTestSuite");

/**
 * Test case of update remaining energy for BasicEnergySource and
 * LrWpanRadioEnergyModel.
 */
class LrWpanEnergyUpdateTest : public TestCase
{
public:
  LrWpanEnergyUpdateTest ();
  virtual ~LrWpanEnergyUpdateTest ();

private:
  void DoRun (void);

  /**
   * \param state Radio state to switch to.
   * \return False if no error occurs.
   *
   * Runs simulation for a while, check if final state & remaining energy is
   * correctly updated.
   */
  bool StateSwitchTest (LrWpanPhyEnumeration state);

private:
  double m_timeS;     // in seconds
  double m_tolerance; // tolerance for power estimation

  ObjectFactory m_energySource;
  ObjectFactory m_deviceEnergyModel;
};

LrWpanEnergyUpdateTest::LrWpanEnergyUpdateTest ()
  : TestCase ("LrWpan energy model update remaining energy test case")
{
  m_timeS = 15.5; // idle for 15 seconds before changing state
  m_tolerance = 1.0e-13;
}

LrWpanEnergyUpdateTest::~LrWpanEnergyUpdateTest ()
{
}

void
LrWpanEnergyUpdateTest::DoRun (void)
{
  // set types
  m_energySource.SetTypeId ("ns3::BasicEnergySource");
  m_deviceEnergyModel.SetTypeId ("ns3::LrWpanRadioEnergyModel");

  // run state switch tests
  NS_TEST_ASSERT_MSG_EQ (StateSwitchTest (IEEE_802_15_4_PHY_RX_ON), false, "Problem with state switch test (LrWpanPhy RX_ON).");
  NS_TEST_ASSERT_MSG_EQ (StateSwitchTest (IEEE_802_15_4_PHY_TX_ON), false, "Problem with state switch test (LrWpanPhy TX_ON).");
  NS_TEST_ASSERT_MSG_EQ (StateSwitchTest (IEEE_802_15_4_PHY_TRX_OFF), false, "Problem with state switch test (LrWpanPhy TRX_OFF).");
}

bool
LrWpanEnergyUpdateTest::StateSwitchTest (LrWpanPhyEnumeration state)
{
  // create node
  Ptr<Node> node = CreateObject<Node> ();

  // create energy source
  Ptr<BasicEnergySource> source = m_energySource.Create<BasicEnergySource> ();
  // aggregate energy source to node
  node->AggregateObject (source);

  // create device energy model
  Ptr<LrWpanRadioEnergyModel> model =
    m_deviceEnergyModel.Create<LrWpanRadioEnergyModel> ();
  // set energy source pointer
  model->SetEnergySource (source);
  // add device energy model to model list in energy source
  source->AppendDeviceEnergyModel (model);

  // retrieve device energy model from energy source
  DeviceEnergyModelContainer models =
    source->FindDeviceEnergyModels ("ns3::LrWpanRadioEnergyModel");
  // check list
  NS_TEST_ASSERT_MSG_EQ_RETURNS_BOOL (false, (models.GetN () == 0), "Model list is empty!");
  // get pointer
  Ptr<LrWpanRadioEnergyModel> devModel =
    DynamicCast<LrWpanRadioEnergyModel> (models.Get (0));
  // check pointer
  NS_TEST_ASSERT_MSG_NE_RETURNS_BOOL (0, devModel, "NULL pointer to device model!");

  /*
   * The radio will stay in the intitial (Sleep) state for m_timeS seconds. Then it will switch into a
   * different state.
   */

  // schedule change of state
  Simulator::Schedule (Seconds (m_timeS),
                       &LrWpanRadioEnergyModel::ChangeState, devModel, state);

  // Calculate remaining energy at simulation stop time
  Simulator::Schedule (Seconds (m_timeS * 2), 
                       &BasicEnergySource::UpdateEnergySource, source);

  double timeDelta = 0.000000001; // 1 nanosecond
  // run simulation; stop just after last scheduled event
  Simulator::Stop (Seconds (m_timeS * 2 + timeDelta));
  Simulator::Run ();

  // energy = current * voltage * time

  // calculate sleep power consumption
  double estRemainingEnergy = source->GetInitialEnergy ();
  double voltage = source->GetSupplyVoltage ();
  estRemainingEnergy -= devModel->GetSleepCurrentA () * voltage * m_timeS;

  // calculate new state power consumption
  double current = 0.0;
  switch (state)
    {
    case IEEE_802_15_4_PHY_TX_ON:
      current = devModel->GetTxCurrentA ();
      break;
    case IEEE_802_15_4_PHY_RX_ON:
      current = devModel->GetRxCurrentA ();
      break;
    case IEEE_802_15_4_PHY_TRX_OFF:
      current = devModel->GetSleepCurrentA ();
      break;
    default:
      NS_FATAL_ERROR ("Undefined radio state: " << state);
      break;
    }
  estRemainingEnergy -= current * voltage * m_timeS;

  // obtain remaining energy from source
  double remainingEnergy = source->GetRemainingEnergy ();
  NS_LOG_DEBUG ("Remaining energy is " << remainingEnergy);
  NS_LOG_DEBUG ("Estimated remaining energy is " << estRemainingEnergy);
  NS_LOG_DEBUG ("Difference is " << estRemainingEnergy - remainingEnergy);
  // check remaining energy
  NS_TEST_ASSERT_MSG_EQ_TOL_RETURNS_BOOL (remainingEnergy, estRemainingEnergy, m_tolerance,
                                          "Incorrect remaining energy!");

  // obtain radio state
  LrWpanPhyEnumeration endState = devModel->GetCurrentState ();
  NS_LOG_DEBUG ("Radio state is " << endState);
  // check end state
  NS_TEST_ASSERT_MSG_EQ_RETURNS_BOOL (endState, state,  "Incorrect end state!");

  Simulator::Destroy ();

  return false; // no error
}

// -------------------------------------------------------------------------- //

/**
 * Test case of update remaining energy for BasicEnergySource and
 * LrWpanRadioEnergyModel.
 */
class LrWpanEnergyTest : public TestCase
{
public:
  LrWpanEnergyTest ();
  virtual ~LrWpanEnergyTest ();

private:
  double m_timeS;     // in seconds
  double m_tolerance; // tolerance for power estimation

  void DoRun (void);

  /**
   * Callback invoked on successful TRX state change.
   */
  void SetTRXStateConfirm (void);

  /**
   * \return False if no error occurs.
   *
   * Runs simulation for a while, check if final state & remaining energy is
   * correctly updated.
   */
  bool PhyStateSwitchTest (void);

};

LrWpanEnergyTest::LrWpanEnergyTest ()
  : TestCase ("LrWpan energy model Phy update test case")
{
  m_tolerance = 1.0e-5;
}

LrWpanEnergyTest::~LrWpanEnergyTest ()
{
}

void
LrWpanEnergyTest::DoRun (void)
{
  NS_TEST_ASSERT_MSG_EQ (PhyStateSwitchTest (), false, "Phy update test case problem.");
}

bool
LrWpanEnergyTest::PhyStateSwitchTest (void)
{
  Ptr<LrWpanPhy> lrWpanPhy = CreateObject<LrWpanPhy> ();

  Ptr<LrWpanRadioEnergyModel> devModel = CreateObject<LrWpanRadioEnergyModel> ();

  Ptr<BasicEnergySource> source = CreateObject<BasicEnergySource> ();
  source->SetSupplyVoltage(3.3);

  devModel->AttachPhy (lrWpanPhy);
  devModel->SetEnergySource (source);
  source->AppendDeviceEnergyModel (devModel);

  Simulator::Schedule (Seconds (1.0),&LrWpanPhy::PlmeSetTRXStateRequest,lrWpanPhy,IEEE_802_15_4_PHY_TX_ON);
  Simulator::Schedule (Seconds (2.0),&LrWpanPhy::PlmeSetTRXStateRequest,lrWpanPhy,IEEE_802_15_4_PHY_RX_ON);
  Simulator::Schedule (Seconds (3.0),&LrWpanPhy::PlmeSetTRXStateRequest,lrWpanPhy,IEEE_802_15_4_PHY_TX_ON);
  Simulator::Schedule (Seconds (4.0),&LrWpanPhy::PlmeSetTRXStateRequest,lrWpanPhy,IEEE_802_15_4_PHY_TRX_OFF);
  Simulator::Schedule (Seconds (5.0),&LrWpanPhy::PlmeSetTRXStateRequest,lrWpanPhy,IEEE_802_15_4_PHY_RX_ON);
  Simulator::Schedule (Seconds (6.0),&LrWpanPhy::PlmeSetTRXStateRequest,lrWpanPhy,IEEE_802_15_4_PHY_TRX_OFF);
  // Calculate remaining energy at simulation stop time
  Simulator::Schedule (Seconds (9), &BasicEnergySource::UpdateEnergySource, source);
  double timeDelta = 0.000000001; // 1 nanosecond
  Simulator::Stop (Seconds (9 + timeDelta));
  Simulator::Run ();

  // energy = current * voltage * time

  // calculate sleep power consumption
  double estRemainingEnergy = source->GetInitialEnergy ();
  double voltage = source->GetSupplyVoltage ();
  estRemainingEnergy -= devModel->GetSleepCurrentA () * voltage * (1.0 + 1.0 + 3);
  estRemainingEnergy -= devModel->GetTxCurrentA () * voltage * (1.0 + 1.0);
  estRemainingEnergy -= devModel->GetRxCurrentA () * voltage * (1.0 + 1.0);

  // obtain remaining energy from source
  NS_LOG_UNCOND ("Now: " << Simulator::Now ());
  double remainingEnergy = source->GetRemainingEnergy ();
  NS_LOG_UNCOND ("Remaining energy is " << remainingEnergy);
  NS_LOG_UNCOND ("Estimated remaining energy is " << estRemainingEnergy);
  NS_LOG_UNCOND ("Difference is " << estRemainingEnergy - remainingEnergy);
  // check remaining energy
  NS_TEST_ASSERT_MSG_EQ_TOL_RETURNS_BOOL (remainingEnergy, estRemainingEnergy, m_tolerance,
                                          "Incorrect remaining energy!");

  Simulator::Destroy ();

  return false;
}

// -------------------------------------------------------------------------- //

/**
 * Unit test suite for LrWpan energy model.
 */
class LrWpanEnergyModelTestSuite : public TestSuite
{
public:
  LrWpanEnergyModelTestSuite ();
};

LrWpanEnergyModelTestSuite::LrWpanEnergyModelTestSuite ()
  : TestSuite ("lr-wpan-energy-model", UNIT)
{
  AddTestCase (new LrWpanEnergyUpdateTest, TestCase::QUICK);
  AddTestCase (new LrWpanEnergyTest, TestCase::QUICK);  //PhyUpdateTest
}

// create an instance of the test suite
static LrWpanEnergyModelTestSuite g_energyModelTestSuite;

