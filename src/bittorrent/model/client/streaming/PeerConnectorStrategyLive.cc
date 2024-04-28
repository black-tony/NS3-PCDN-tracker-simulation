/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010-2014 ComSys, RWTH Aachen University
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
 * Authors: Rene Glebke, Martin Lang (principal authors), Alexander Hocks
 */

#include "PeerConnectorStrategyLive.h"

#include "BitTorrentClient.h"
#include "BitTorrentHttpClient.h"
#include "BitTorrentPeer.h"
#include "PeerConnectorStrategyBase.h"

#include "ns3/BitTorrentDefines.h"
#include "ns3/TorrentFile.h"
#include "ns3/address.h"
#include "ns3/event-id.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/uinteger.h"

#include <map>
#include <set>
#include <utility>

namespace ns3
{
namespace bittorrent
{

NS_LOG_COMPONENT_DEFINE("bittorrent::PeerConnectorStrategyLive");
NS_OBJECT_ENSURE_REGISTERED(PeerConnectorStrategyLive);

PeerConnectorStrategyLive::PeerConnectorStrategyLive(Ptr<BitTorrentClient> myClient)
    : PeerConnectorStrategyBase(myClient)
{
}

PeerConnectorStrategyLive::~PeerConnectorStrategyLive()
{
}

void
PeerConnectorStrategyLive::ProcessPeriodicSchedule()
{
    if (m_myClient->GetDownloadCompleted())
    {
        NS_FATAL_ERROR("in live, you should not finish download unless you exit");
        return;
    }

    if (m_myClient->GetAutoConnect() && GetPeerCount() < m_myClient->GetDesiredPeers()) // Still peers left that we want to have
    {
        ConnectToPeers(m_myClient->GetDesiredPeers() - GetPeerCount());
    }

    Simulator::Cancel(m_nextPeriodicEvent);
    m_nextPeriodicEvent = Simulator::Schedule(m_periodicInterval, &PeerConnectorStrategyLive::ProcessPeriodicSchedule, this);
}

void
PeerConnectorStrategyLive::ProcessPeriodicReannouncements()
{
    if (m_myClient->GetConnectedToCloud())
    {
        std::map<std::string, std::string> additionalParameters;
        additionalParameters["PeerType"] = "UNKNOWN"; // TODO : add peer type parameter

        // The last parameter in here is set to "false" so more important updates can override this one
        ContactTracker(PeerConnectorStrategyBase::REGULAR_UPDATE, 1, additionalParameters, false);
    }

    Simulator::Cancel(m_nextPeriodicReannouncement);
    m_nextPeriodicReannouncement = Simulator::Schedule(m_reannouncementInterval, &PeerConnectorStrategyLive::ProcessPeriodicReannouncements, this);
}

void
PeerConnectorStrategyLive::ConnectToCloud()
{
    NS_LOG_INFO("PeerConnectorStrategyLive: " << m_myClient->GetIp() << ": Contacting tracker to join cloud...");

    std::map<std::string, std::string> additionalParameters;
    additionalParameters["PeerType"] = "UNKNOWN"; // TODO : add peer type parameter
    // num want should always be one
    ContactTracker(PeerConnectorStrategyBase::STARTED, 1, additionalParameters, true);
    m_myClient->SetConnectionToCloudSuspended(false);
    m_myClient->CloudConnectionEstablishedEvent();

    ProcessPeriodicReannouncements();
}

} // namespace bittorrent
} // namespace ns3
