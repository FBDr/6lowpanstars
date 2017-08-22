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
 * Author:  Tom Henderson <thomas.r.henderson@boeing.com>
 *  	    Vishwesh Rege <vrege2012@gmail.com>
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
#include "ns3/lr-wpan-radio-energy-model.h"
#include "ns3/basic-energy-source.h"

#include <iostream>

using namespace ns3;

static void DataIndication(McpsDataIndicationParams params, Ptr<Packet> p) {
    std::cout << "Received packet of size " << p->GetSize() << "\n";
}

static void DataConfirm(McpsDataConfirmParams params) {
    std::cout << "LrWpanMcpsDataConfirmStatus = " << params.m_status << "\n";
}

static void StateChangeNotification(std::string context, Time now, LrWpanPhyEnumeration oldState, LrWpanPhyEnumeration newState) {
    std::cout << context << " state change at " << now.GetSeconds()
            << " from " << LrWpanHelper::LrWpanPhyEnumerationPrinter(oldState)
            << " to " << LrWpanHelper::LrWpanPhyEnumerationPrinter(newState) << "\n";
}

static void GetTotalEnergyConsumption(std::string context, Time now, double oldValue, double newValue) {
    std::cout << now.GetSeconds() << context << " TotalEnergyConsumption: " << newValue
            << " from " << oldValue << "\n";
}

int main(int argc, char *argv[]) {
    bool verbose = false;

    CommandLine cmd;

    cmd.AddValue("verbose", "turn on all log components", verbose);

    cmd.Parse(argc, argv);

    LrWpanHelper lrWpanHelper;
    if (verbose) {
        lrWpanHelper.EnableLogComponents();
    }

    LogComponentEnable("BasicEnergySource", LOG_LEVEL_DEBUG);

    // Enable calculation of FCS in the trailers. Only necessary when interacting with real devices or wireshark.
    // GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

    // Create 2 nodes, and a NetDevice for each one
    Ptr<Node> n0 = CreateObject <Node> ();
    Ptr<Node> n1 = CreateObject <Node> ();

    Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice> ();
    Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice> ();

    dev0->SetAddress(Mac16Address("00:01"));
    dev1->SetAddress(Mac16Address("00:02"));

    // Each device must be attached to the same channel
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
    Ptr<LogDistancePropagationLossModel> propModel = CreateObject<LogDistancePropagationLossModel> ();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    dev0->SetChannel(channel);
    dev1->SetChannel(channel);

    // To complete configuration, a LrWpanNetDevice must be added to a node
    n0->AddDevice(dev0);
    n1->AddDevice(dev1);

    Ptr<LrWpanRadioEnergyModel> em[2];

    em[0] = CreateObject<LrWpanRadioEnergyModel> ();
    em[1] = CreateObject<LrWpanRadioEnergyModel> ();

    Ptr<BasicEnergySource> es0 = CreateObject<BasicEnergySource> ();
    es0->SetSupplyVoltage(3.3);
    Ptr<BasicEnergySource> es1 = CreateObject<BasicEnergySource> ();
    es1->SetSupplyVoltage(3.3);

    es0->SetNode(n0);
    es0->AppendDeviceEnergyModel(em[0]);
    em[0]->SetEnergySource(es0);
    em[0]->AttachPhy(dev0->GetPhy());

    es1->SetNode(n1);
    es1->AppendDeviceEnergyModel(em[1]);
    em[1]->SetEnergySource(es1);
    em[1]->AttachPhy(dev1->GetPhy());

    es0->TraceConnect("RemainingEnergy", std::string("phy0"), MakeCallback(&GetTotalEnergyConsumption));
    es1->TraceConnect("RemainingEnergy", std::string("phy1"), MakeCallback(&GetTotalEnergyConsumption));

    // Trace state changes in the phy
    dev0->GetPhy()->TraceConnect("TrxState", std::string("phy0"), MakeCallback(&StateChangeNotification));
    dev1->GetPhy()->TraceConnect("TrxState", std::string("phy1"), MakeCallback(&StateChangeNotification));

    Ptr<ConstantPositionMobilityModel> sender0Mobility = CreateObject<ConstantPositionMobilityModel> ();
    // Configure position 0 m distance
    sender0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(sender0Mobility);

    Ptr<ConstantPositionMobilityModel> sender1Mobility = CreateObject<ConstantPositionMobilityModel> ();
    // Configure position 10 m distance
    sender1Mobility->SetPosition(Vector(0, 10, 0));
    dev1->GetPhy()->SetMobility(sender1Mobility);

    McpsDataConfirmCallback cb0; // Set the callback for the confirmation of a data transmission request
    cb0 = MakeCallback(&DataConfirm);
    dev0->GetMac()->SetMcpsDataConfirmCallback(cb0);

    McpsDataIndicationCallback cb1; // Set the callback for the indication of an incoming data packet
    cb1 = MakeCallback(&DataIndication);
    dev0->GetMac()->SetMcpsDataIndicationCallback(cb1);

    McpsDataConfirmCallback cb2;
    cb2 = MakeCallback(&DataConfirm);
    dev1->GetMac()->SetMcpsDataConfirmCallback(cb2);

    McpsDataIndicationCallback cb3;
    cb3 = MakeCallback(&DataIndication);
    dev1->GetMac()->SetMcpsDataIndicationCallback(cb3);

    // Tracing
    lrWpanHelper.EnablePcapAll(std::string("lr-wpan-data"), true);
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream("lr-wpan-data.tr");
    lrWpanHelper.EnableAsciiAll(stream);

    // The below should trigger two callbacks when end-to-end data is working
    // 1) DataConfirm callback is called
    // 2) DataIndication callback is called with value of 50
    Ptr<Packet> p0 = Create<Packet> (50); // 50 bytes of dummy data
    McpsDataRequestParams params;
    params.m_srcAddrMode = SHORT_ADDR;
    params.m_dstAddrMode = SHORT_ADDR;
    params.m_dstPanId = 0;
    params.m_dstAddr = Mac16Address("00:02");
    params.m_msduHandle = 0;
    params.m_txOptions = TX_OPTION_ACK;
    //  dev0->GetMac ()->McpsDataRequest (params, p0);
    Simulator::ScheduleWithContext(1, Seconds(0.0),
            &LrWpanMac::McpsDataRequest,
            dev0->GetMac(), params, p0);

    // Send a packet back at time 2 seconds
    Ptr<Packet> p2 = Create<Packet> (60); // 60 bytes of dummy data
    params.m_dstAddr = Mac16Address("00:01");
    Simulator::ScheduleWithContext(2, Seconds(2.0),
            &LrWpanMac::McpsDataRequest,
            dev1->GetMac(), params, p2);

    Simulator::Stop(Seconds(5));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
