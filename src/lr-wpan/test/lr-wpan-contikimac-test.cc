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
 * Author:  Vishwesh Rege <vrege2012@gmail.com>
 */
#include <ns3/test.h>
#include <ns3/log.h>

#include <ns3/lr-wpan-module.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/packet.h>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("lr-wpan-contikimac-test");

class LrWpanContikiMacTestCase : public TestCase
{
public:
  LrWpanContikiMacTestCase ();
  virtual ~LrWpanContikiMacTestCase ();

private:
  virtual void DoRun (void);

  /**
   * Callback invoked on successful packet reception.
   */
  void DataIndication (McpsDataIndicationParams params, Ptr<Packet> p);

  int m_callbackCount;      // counter for # of callbacks invoked
};

LrWpanContikiMacTestCase::LrWpanContikiMacTestCase ()
  : TestCase ("Test ContikiMAC")
{
  m_callbackCount = 0;
}

LrWpanContikiMacTestCase::~LrWpanContikiMacTestCase ()
{
}

void 
LrWpanContikiMacTestCase::DataIndication (McpsDataIndicationParams params, Ptr<Packet> p)
{
  NS_TEST_ASSERT_MSG_EQ (p->GetSize (), 20, "Packet received with unexpected size");
  NS_TEST_ASSERT_MSG_EQ (params.m_dstAddr, Mac16Address ("00:02"), "Incorrect destination address");
  m_callbackCount++;
}

void
LrWpanContikiMacTestCase::DoRun (void)
{
  // Create 3 nodes, and a NetDevice for each one
  Ptr<Node> n0 = CreateObject <Node> ();
  Ptr<Node> n1 = CreateObject <Node> ();
  Ptr<Node> n2 = CreateObject <Node> ();

  Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice> ();
  Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice> ();
  Ptr<LrWpanNetDevice> dev2 = CreateObject<LrWpanNetDevice> ();

  Ptr<LrWpanContikiMac> mac0 = CreateObject<LrWpanContikiMac> ();     //Change Mac
  Ptr<LrWpanContikiMac> mac1 = CreateObject<LrWpanContikiMac> ();     //Change Mac
  Ptr<LrWpanContikiMac> mac2 = CreateObject<LrWpanContikiMac> ();     //Change Mac

  dev0->SetMac (mac0);
  dev1->SetMac (mac1);
  dev2->SetMac (mac2);

  dev0->SetAddress (Mac16Address ("00:01"));
  dev1->SetAddress (Mac16Address ("00:02"));
  dev2->SetAddress (Mac16Address ("00:03"));

  // Each device must be attached to the same channel
  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> propModel = CreateObject<LogDistancePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channel->AddPropagationLossModel (propModel);
  channel->SetPropagationDelayModel (delayModel);

  dev0->SetChannel (channel);
  dev1->SetChannel (channel);
  dev2->SetChannel (channel);

  // To complete configuration, a LrWpanNetDevice must be added to a node
  n0->AddDevice (dev0);
  n1->AddDevice (dev1);
  n2->AddDevice (dev2);

  Ptr<ConstantPositionMobilityModel> sender0Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender0Mobility->SetPosition (Vector (0,0,0));
  dev0->GetPhy ()->SetMobility (sender0Mobility);
  Ptr<ConstantPositionMobilityModel> sender1Mobility = CreateObject<ConstantPositionMobilityModel> ();
  // Configure position 10 m distance
  sender1Mobility->SetPosition (Vector (0,10,0));
  dev1->GetPhy ()->SetMobility (sender1Mobility);
  Ptr<ConstantPositionMobilityModel> sender2Mobility = CreateObject<ConstantPositionMobilityModel> ();
  // Configure position
  sender2Mobility->SetPosition (Vector (0,20,20));
  dev2->GetPhy ()->SetMobility (sender2Mobility);

  McpsDataIndicationCallback callback = MakeCallback (&LrWpanContikiMacTestCase::DataIndication, this);
  mac0->SetMcpsDataIndicationCallback (callback);
  mac1->SetMcpsDataIndicationCallback (callback);
  mac2->SetMcpsDataIndicationCallback (callback);

  Ptr<Packet> p = Create<Packet> (20);  // 20 bytes of dummy data
  NS_TEST_ASSERT_MSG_EQ (p->GetSize (), 20, "Packet created with unexpected size");
  McpsDataRequestParams params;
  params.m_srcAddrMode = SHORT_ADDR;
  params.m_dstAddrMode = SHORT_ADDR;
  params.m_dstPanId = 0;
  params.m_dstAddr = Mac16Address ("00:02");
  params.m_msduHandle = 0;
  params.m_txOptions = TX_OPTION_ACK;
  Simulator::Schedule (Seconds (0.1), &LrWpanContikiMac::McpsDataRequest, mac0, params, p);

  Simulator::Stop (Seconds (0.2));

  Simulator::Run ();

  Simulator::Destroy ();

  NS_TEST_ASSERT_MSG_EQ (m_callbackCount, 1, "Not all callbacks are invoked!");

}

// ==============================================================================
class LrWpanContikiMacTestSuite : public TestSuite
{
public:
  LrWpanContikiMacTestSuite ();
};

LrWpanContikiMacTestSuite::LrWpanContikiMacTestSuite ()
  : TestSuite ("lr-wpan-contikimac", UNIT)
{
  AddTestCase (new LrWpanContikiMacTestCase, TestCase::QUICK);
}

static LrWpanContikiMacTestSuite lrWpanContikiMacTestSuite;
