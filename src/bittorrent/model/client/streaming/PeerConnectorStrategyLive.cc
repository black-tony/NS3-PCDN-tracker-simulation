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
        additionalParameters["PeerType"] = m_myClient->GetClientType();

        // The last parameter in here is set to "false" so more important updates can override this one
        ContactTracker(PeerConnectorStrategyBase::REGULAR_UPDATE, 1, additionalParameters, false);
    }

    Simulator::Cancel(m_nextPeriodicReannouncement);
    m_nextPeriodicReannouncement = Simulator::Schedule(m_reannouncementInterval, &PeerConnectorStrategyLive::ProcessPeriodicReannouncements, this);
}

void
PeerConnectorStrategyLive::ConnectToCloud()
{
    //YTODO client connect to cloud with self hash to get seeders 
    NS_LOG_INFO("PeerConnectorStrategyLive: " << m_myClient->GetIp() << ": Contacting tracker to join cloud...");

    std::map<std::string, std::string> additionalParameters;
    additionalParameters["PeerType"] = m_myClient->GetClientType();
    // num want should always be one
    ContactTracker(PeerConnectorStrategyBase::STARTED, 1, additionalParameters, true);
    m_myClient->SetConnectionToCloudSuspended(false);
    m_myClient->CloudConnectionEstablishedEvent();

    ProcessPeriodicReannouncements();
}

void
PeerConnectorStrategyLive::GetSeeder(std::string streamHash)
{
    std::map<std::string, std::string> additionalParameters;
    additionalParameters["PeerType"] = m_myClient->GetClientType();
    additionalParameters["StreamHash"] = streamHash;
    ContactTracker(PeerConnectorStrategyBase::GET_SEEDER, 1, additionalParameters, true);
}

uint16_t
PeerConnectorStrategyLive::ConnectToPeers(uint16_t count)
{
    //YTODO 因为要覆盖potential peers, 所以需要重新注册
    return 0;
}

void
PeerConnectorStrategyLive::ParseResponse(std::istream& response)
{
    if (m_myClient->GetConnectionToCloudSuspended())
    {
        return;
    }

    // NS_LOG_DEBUG ("Parsing tracker response \"" << dynamic_cast<std::stringstream&> (response).str () << "\"." << std::endl);

    Ptr<TorrentData> root = TorrentData::ReadBencodedData(response);
    Ptr<TorrentDataDictonary> rootDict = DynamicCast<TorrentDataDictonary>(root);

    // NS_ASSERT(rootDict);
    if (!rootDict)
    {
        return;
    }

    // test if there was an error
    Ptr<TorrentData> errorMsg = rootDict->GetData("failure reason");

    if (errorMsg)
    {
        Ptr<TorrentDataString> errorMsgStr = DynamicCast<TorrentDataString>(errorMsg);
        return;
    }

    // retrieve renewal interval
    // ALEX: Sometimes, the tracker response seems malformed. Check where this comes from (Sending? Receiving?)
    Ptr<TorrentDataInt> renewalInterval = DynamicCast<TorrentDataInt>(rootDict->GetData("interval"));
    if (!renewalInterval)
    {
        return;
    }
    // NS_ASSERT(renewalInterval);
    m_reannouncementInterval = Seconds(static_cast<uint32_t>(renewalInterval->GetData()));

    Ptr<TorrentDataInt> leechers = DynamicCast<TorrentDataInt>(rootDict->GetData("incomplete"));
    if (!leechers)
    {
        return;
    }
    // NS_ASSERT(leechers);
    leechers->GetData();

    Ptr<TorrentDataInt> seeders = DynamicCast<TorrentDataInt>(rootDict->GetData("complete"));
    if (!seeders)
    {
        return;
    }
    // NS_ASSERT(seeders);
    seeders->GetData();

    // update the tracker id if there is one
    Ptr<TorrentDataString> trackId = DynamicCast<TorrentDataString>(rootDict->GetData("tracker id"));
    if (trackId)
    {
        m_trackerId = trackId->GetData();
    }
    // retrieve the peer list from response
    Ptr<TorrentData> peers = rootDict->GetData("peers");
    if (!peers)
    {
        return;
    }
    // NS_ASSERT(peers);

    // A mini heuristic against dead (inactive) peers in the set of potential peers
    if (m_currentClientUpdateCycle == m_clientUpdateCycles - 1)
    {
        m_potentialClients.clear();
    }
    m_currentClientUpdateCycle = (m_currentClientUpdateCycle + 1) % m_clientUpdateCycles;

    // we have to distinguish which type of peerlist is coming in
    if (peers->GetDataType() == TorrentData::DATA_STRING)
    {
        // this is the plain style, the string contains peers in the format
        // 4 byte ipaddr, 2 byte port until the end
        Ptr<TorrentDataString> peerStringPtr = DynamicCast<TorrentDataString>(peers);
        if (!peerStringPtr)
        {
            return;
        }
        // NS_ASSERT(peerStringPtr);

        const char* peerString = peerStringPtr->GetData().c_str();

        size_t peerLen = peerStringPtr->GetData().size();
        if (!((peerLen % 6) == 0))
        {
            return;
        }
        // NS_ASSERT((peerLen % 6) == 0);

        // read data sequentially
        std::pair<uint32_t, uint16_t> peer;
        for (uint32_t peerNum = 0; peerNum < peerLen; peerNum += 2)
        {
            uint32_t ipaddr;
            ipaddr = peerString[peerNum++];
            ipaddr <<= 8;
            ipaddr |= peerString[peerNum++];
            ipaddr <<= 8;
            ipaddr |= peerString[peerNum++];
            ipaddr <<= 8;
            ipaddr |= peerString[peerNum++];

            // Skip own IP
            if (ipaddr == m_myClient->GetIp().Get())
            {
                continue;
            }

            peer.first = ipaddr;
            peer.second = (peerString[peerNum] << 8) | peerString[peerNum + 1];

            m_potentialClients.insert(peer);
        }
    }
    else
    {
        // this is decorated style
        // peers is a dictionary of peerid, ip, port,
        // ip is string with dotted addr, or dns name - DNS is not yet supported (TODO)
        Ptr<TorrentDataList> peerList = DynamicCast<TorrentDataList>(peers);
        if (!peerList)
        {
            return;
        }
        // NS_ASSERT(peerList);

        std::pair<uint32_t, uint16_t> peer;
        for (std::list<Ptr<TorrentData>>::const_iterator iter = peerList->GetIterator(); iter != peerList->GetListEnd(); ++iter)
        {
            Ptr<TorrentDataDictonary> currentPeer = DynamicCast<TorrentDataDictonary>(*iter);
            if (!currentPeer)
            {
                continue;
            }
            // NS_ASSERT(currentPeer);

            Ptr<TorrentDataString> peerAddr = DynamicCast<TorrentDataString>(currentPeer->GetData("ip"));
            if (!peerAddr)
            {
                continue;
            }
            // NS_ASSERT(peerAddr);

            Ptr<TorrentDataInt> peerPort = DynamicCast<TorrentDataInt>(currentPeer->GetData("port"));
            if (!peerPort)
            {
                continue;
            }
            // NS_ASSERT(peerPort);

            Ipv4Address buf;
            buf.Set(peerAddr->GetData().c_str());

            // Skip own IP
            if (buf == m_myClient->GetIp())
            {
                continue;
            }

            peer.first = buf.Get();
            peer.second = static_cast<uint16_t>(peerPort->GetData());
            //YTODO tracker transmission 
            m_potentialClients.insert(peer);
        }
    }

    if (!m_myClient->GetConnectedToCloud())
        m_myClient->CloudConnectionEstablishedEvent();

    m_myClient->SetConnectedToCloud(true);
}

const std::set<std::pair<std::string, std::pair<uint32_t, uint16_t>>>&
PeerConnectorStrategyLive::GetPotentialClients() const
{
    return m_potentialClients;
}

void
PeerConnectorStrategyLive::DoInitialize()
{
    PeerConnectorStrategyBase::DoInitialize();
    m_myClient->RegisterCallbackGetSeederEvent(MakeCallback(&PeerConnectorStrategyLive::GetSeeder, this));
    // this covered the parent class's connectToPeers function
    m_myClient->SetCallbackConnectToPeers(MakeCallback(&PeerConnectorStrategyLive::ConnectToPeers, this));
    // m_myClient->
    // m_myClient->RegisterCallbackStreamBufferReadyEvent(MakeCallback(&PeerConnectorStrategyLive::OnStreamBufferReady, this))
}

} // namespace bittorrent
} // namespace ns3
