/**
 * Author: Yinghao Yang <blacktonyrl@gmail.com>
 * 
*/

#include <ns3/names.h>
#include "VideoStreamHelper.h"

namespace ns3 {


// 3GPP HTTP CLIENT HELPER /////////////////////////////////////////////////////////

VideoStreamClientHelper::VideoStreamClientHelper (const Address &address)
{
  m_factory.SetTypeId ("ns3::VideoStreamClient");
  m_factory.Set ("RemoteServerAddress", AddressValue (address));
}

void
VideoStreamClientHelper::SetAttribute (const std::string &name,
                                        const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
VideoStreamClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
VideoStreamClientHelper::Install (const std::string &nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
VideoStreamClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
VideoStreamClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}


// HTTP SERVER HELPER /////////////////////////////////////////////////////////

VideoStreamServerHelper::VideoStreamServerHelper (const Address &address)
{
  m_factory.SetTypeId ("ns3::VideoStreamServer");
  m_factory.Set ("LocalAddress", AddressValue (address));
}

void
VideoStreamServerHelper::SetAttribute (const std::string &name,
                                        const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
VideoStreamServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
VideoStreamServerHelper::Install (const std::string &nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
VideoStreamServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
VideoStreamServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}

// PCDN HElper
VideoStreamPCDNHelper::VideoStreamPCDNHelper (const Address &localAddress,const Address& P2PServerAddress,  const Address &remoteServerAddress)
{
  m_factory.SetTypeId ("ns3::VideoStreamPCDN");
  m_factory.Set ("LocalAddress", AddressValue (localAddress));
  m_factory.Set ("RemoteServerAddress", AddressValue (remoteServerAddress));
  m_factory.Set ("P2PServerAddress", AddressValue (P2PServerAddress));
}

void
VideoStreamPCDNHelper::SetAttribute (const std::string &name,
                                        const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
VideoStreamPCDNHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
VideoStreamPCDNHelper::Install (const std::string &nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
VideoStreamPCDNHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
VideoStreamPCDNHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}

} // end of `namespace ns3`