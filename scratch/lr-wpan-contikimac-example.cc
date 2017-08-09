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
 * Author: Vishwesh Rege <vrege2012@gmail.com>
 */

/*
 * Try to send data end-to-end through a LrWpanMac <-> LrWpanPhy <->
 * SpectrumChannel <-> LrWpanPhy <-> LrWpanMac chain
 *
 * Trace Phy state changes, and Mac DataIndication and DataConfirm events
 * to stdout
 */
#include <ns3/log.h>
#include <ns3/core-module.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/packet.h>

#include <iostream>

using namespace ns3;

static void DataIndication (McpsDataIndicationParams params, Ptr<Packet> p)
{
  std::cout << "Received packet of size " << p->GetSize () << "\n";
}

static void DataConfirm (McpsDataConfirmParams params)
{
  std::cout << "LrWpanMcpsDataConfirmStatus = " << params.m_status << "\n";
}

static void StateChangeNotification (std::string context, Time now, LrWpanPhyEnumeration oldState, LrWpanPhyEnumeration newState)
{
  std::cout << context << " state change at " << now.GetSeconds ()
                       << " from " << LrWpanHelper::LrWpanPhyEnumerationPrinter (oldState)
                       << " to " << LrWpanHelper::LrWpanPhyEnumerationPrinter (newState) << "\n";
}

int main (int argc, char *argv[])
{
  bool verbose = false;

  CommandLine cmd;

  cmd.AddValue ("verbose", "turn on all log components", verbose);

  cmd.Parse (argc, argv);

  LogComponentEnableAll (LOG_PREFIX_TIME);
  //LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnable ("LrWpanCsmaCa", LOG_LEVEL_DEBUG);
  LogComponentEnable ("LrWpanContikiMac", LOG_LEVEL_DEBUG);
  LogComponentEnable ("LrWpanPhy", LOG_LEVEL_DEBUG);

  // Enable calculation of FCS in the trailers. Only necessary when interacting with real devices or wireshark.
  // GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

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

  // Trace state changes in the phy
  dev0->GetPhy ()->TraceConnect ("TrxState", std::string ("phy1"), MakeCallback (&StateChangeNotification));
  dev1->GetPhy ()->TraceConnect ("TrxState", std::string ("phy2"), MakeCallback (&StateChangeNotification));
  dev2->GetPhy ()->TraceConnect ("TrxState", std::string ("phy3"), MakeCallback (&StateChangeNotification));

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

  McpsDataConfirmCallback cb0;
  cb0 = MakeCallback (&DataConfirm);
  dev0->GetMac ()->SetMcpsDataConfirmCallback (cb0);

  McpsDataIndicationCallback cb1;
  cb1 = MakeCallback (&DataIndication);
  dev0->GetMac ()->SetMcpsDataIndicationCallback (cb1);

  McpsDataConfirmCallback cb2;
  cb2 = MakeCallback (&DataConfirm);
  dev1->GetMac ()->SetMcpsDataConfirmCallback (cb2);

  McpsDataIndicationCallback cb3;
  cb3 = MakeCallback (&DataIndication);
  dev1->GetMac ()->SetMcpsDataIndicationCallback (cb3);

  dev2->GetMac ()->SetMcpsDataConfirmCallback (MakeCallback (&DataConfirm));
  dev2->GetMac ()->SetMcpsDataIndicationCallback (MakeCallback (&DataIndication));
  dev2->GetMac ()->SetAttribute ("SleepTime", DoubleValue (0.125));

  // The below should trigger two callbacks when end-to-end data is working
  // 1) DataConfirm callback is called
  // 2) DataIndication callback is called with value of 50
  Ptr<Packet> p0 = Create<Packet> (50);  // 50 bytes of dummy data
  McpsDataRequestParams params;
  params.m_srcAddrMode = SHORT_ADDR;
  params.m_dstAddrMode = SHORT_ADDR;
  params.m_dstPanId = 0;
  params.m_dstAddr = Mac16Address ("00:02");
  params.m_msduHandle = 0;
  params.m_txOptions = TX_OPTION_ACK;
  //Ptr<LrWpanContikiMac> lrWpanMac = DynamicCast<LrWpanContikiMac> (dev0->GetMac ());
  Simulator::Schedule (Seconds (0.1), &LrWpanMac::McpsDataRequest, dev0->GetMac (), params, p0);        //phy3 wakeup&sleep, phy2 wakeup&recv

  Ptr<Packet> p1 = Create<Packet> (50);  // 50 bytes of dummy data
  params.m_dstAddr = Mac16Address ("00:03");
  Simulator::Schedule (Seconds (0.12), &LrWpanMac::McpsDataRequest, dev0->GetMac (), params, p1);       //phy1 drop pkt to phy3

  Ptr<Packet> p2 = Create<Packet> (50);  // 50 bytes of dummy data
  params.m_dstAddr = Mac16Address ("ff:ff");
  Simulator::Schedule (Seconds (0.5), &LrWpanMac::McpsDataRequest, dev2->GetMac (), params, p2);        //phy1&2 recv broadcast

  Ptr<Packet> p3 = Create<Packet> (50);  // 50 bytes of dummy data
  params.m_txOptions = TX_OPTION_NONE;
  Simulator::Schedule (Seconds (0.52), &LrWpanMac::McpsDataRequest, dev2->GetMac (), params, p3);

  Simulator::Stop (Seconds (0.75));
  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}
