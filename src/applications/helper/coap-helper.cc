/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "coap-helper.h"
#include "ns3/coap-server.h"
#include "ns3/coap-client.h"
#include "ns3/coap-cache-gtw.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

    CoapServerHelper::CoapServerHelper(uint16_t port) {
        m_factory.SetTypeId(CoapServer::GetTypeId());
        SetAttribute("Port", UintegerValue(port));
    }

    void
    CoapServerHelper::SetAttribute(
            std::string name,
            const AttributeValue & value) {
        m_factory.Set(name, value);
    }

    ApplicationContainer
    CoapServerHelper::Install(Ptr<Node> node) const {
        return ApplicationContainer(InstallPriv(node));
    }

    ApplicationContainer
    CoapServerHelper::Install(std::string nodeName) const {
        Ptr<Node> node = Names::Find<Node> (nodeName);
        return ApplicationContainer(InstallPriv(node));
    }

    ApplicationContainer
    CoapServerHelper::Install(NodeContainer c) const {
        ApplicationContainer apps;
        for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i) {
            apps.Add(InstallPriv(*i));
        }

        return apps;
    }

    void
    CoapServerHelper::SetIPv6Bucket(Ptr<Application> app, std::vector<Ipv6Address> &bucket) {
        app->GetObject<CoapServer>()->SetIPv6Bucket(bucket);
    }

    void
    CoapServerHelper::SetNodeToGtwMap(Ptr<Application> app, std::map<Ipv6Address, Ipv6Address> gtw_to_node) {
        app->GetObject<CoapServer>()->SetNodeToGtwMap(gtw_to_node);
    }

    Ptr<Application>
    CoapServerHelper::InstallPriv(Ptr<Node> node) const {
        Ptr<Application> app = m_factory.Create<CoapServer> ();
        node->AddApplication(app);

        return app;
    }

    CoapCacheGtwHelper::CoapCacheGtwHelper(uint16_t port) {
        m_factory.SetTypeId(CoapCacheGtw::GetTypeId());
        SetAttribute("Port", UintegerValue(port));
    }

    void
    CoapCacheGtwHelper::SetAttribute(
            std::string name,
            const AttributeValue & value) {
        m_factory.Set(name, value);
    }

    ApplicationContainer
    CoapCacheGtwHelper::Install(Ptr<Node> node) const {
        return ApplicationContainer(InstallPriv(node));
    }

    ApplicationContainer
    CoapCacheGtwHelper::Install(std::string nodeName) const {
        Ptr<Node> node = Names::Find<Node> (nodeName);
        return ApplicationContainer(InstallPriv(node));
    }

    ApplicationContainer
    CoapCacheGtwHelper::Install(NodeContainer c) const {
        ApplicationContainer apps;
        for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i) {
            apps.Add(InstallPriv(*i));
        }

        return apps;
    }

    void
    CoapCacheGtwHelper::SetIPv6Bucket(Ptr<Application> app, std::vector<Ipv6Address> &bucket) {
        app->GetObject<CoapCacheGtw>()->SetIPv6Bucket(bucket);
    }

    void
    CoapCacheGtwHelper::SetNodeToGtwMap(Ptr<Application> app, std::map<Ipv6Address, Ipv6Address> gtw_to_node) {
        app->GetObject<CoapCacheGtw>()->SetNodeToGtwMap(gtw_to_node);
    }

    Ptr<Application>
    CoapCacheGtwHelper::InstallPriv(Ptr<Node> node) const {
        Ptr<Application> app = m_factory.Create<CoapCacheGtw> ();
        node->AddApplication(app);

        return app;
    }

    CoapClientHelper::CoapClientHelper(uint16_t port) {
        m_factory.SetTypeId(CoapClient::GetTypeId());
        SetAttribute("RemotePort", UintegerValue(port));
    }

    void
    CoapClientHelper::SetAttribute(
            std::string name,
            const AttributeValue & value) {
        m_factory.Set(name, value);
    }

    void
    CoapClientHelper::SetIPv6Bucket(Ptr<Application> app, std::vector<Ipv6Address> &bucket) {
        app->GetObject<CoapClient>()->SetIPv6Bucket(bucket);
    }

    void
    CoapClientHelper::SetNodeToGtwMap(Ptr<Application> app, std::map<Ipv6Address, Ipv6Address> gtw_to_node) {
        app->GetObject<CoapClient>()->SetNodeToGtwMap(gtw_to_node);
    }

    void
    CoapClientHelper::SetFill(Ptr<Application> app, std::string fill) {
        app->GetObject<CoapClient>()->SetFill(fill);
    }

    void
    CoapClientHelper::SetFill(Ptr<Application> app, uint8_t fill, uint32_t dataLength) {
        app->GetObject<CoapClient>()->SetFill(fill, dataLength);
    }

    void
    CoapClientHelper::SetFill(Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength) {
        app->GetObject<CoapClient>()->SetFill(fill, fillLength, dataLength);
    }

    ApplicationContainer
    CoapClientHelper::Install(Ptr<Node> node) const {
        return ApplicationContainer(InstallPriv(node));
    }

    ApplicationContainer
    CoapClientHelper::Install(std::string nodeName) const {
        Ptr<Node> node = Names::Find<Node> (nodeName);
        return ApplicationContainer(InstallPriv(node));
    }

    ApplicationContainer
    CoapClientHelper::Install(NodeContainer c) const {
        ApplicationContainer apps;
        for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i) {
            apps.Add(InstallPriv(*i));
        }

        return apps;
    }

    Ptr<Application>
    CoapClientHelper::InstallPriv(Ptr<Node> node) const {
        Ptr<Application> app = m_factory.Create<CoapClient> ();
        node->AddApplication(app);

        return app;
    }

} // namespace ns3
