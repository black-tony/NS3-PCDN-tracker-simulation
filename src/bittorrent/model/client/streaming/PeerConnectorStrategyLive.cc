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

#include "ns3/BitTorrentClient.h"
#include "ns3/BitTorrentDefines.h"
#include "ns3/BitTorrentHttpClient.h"
#include "ns3/BitTorrentPeer.h"
#include "ns3/PeerConnectorStrategyBase.h"
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
        additionalParameters["StreamHash"] = m_myClient->GetStreamHash();
        additionalParameters["LiveStreaming"] = "1";

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
    additionalParameters["PeerType"] = m_myClient->GetClientType();
    additionalParameters["StreamHash"] = m_myClient->GetStreamHash();
    additionalParameters["LiveStreaming"] = "1";
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
    additionalParameters["LiveStreaming"] = "1";
    additionalParameters["StreamHash"] = streamHash;
    ContactTracker(PeerConnectorStrategyBase::GET_SEEDER, 1, additionalParameters, true);
}

uint16_t
PeerConnectorStrategyLive::ConnectToPeers(uint16_t count)
{
    //  We only need to do this if we have not yet completed our download
    if (m_myClient->GetDownloadCompleted())
    {
        return 0;
    }

    // Step 1: Get the clients that we MAY connect to
    const auto& potentialPeers = GetPotentialClients();
    if (potentialPeers.size() == 0) // No one we could connect to
    {
        return 0;
    }

    // Step 2a: If no count of peers to connect to is specified, we want to connect to as many peers as we are allowed to
    if (count == 0)
    {
        count = m_myClient->GetDesiredPeers();
    }

    // Step 2b: We don't actively connect to more peers than we are allowed to
    int32_t connectToNPeers = std::min(static_cast<int32_t>(count), m_myClient->GetMaxPeers() - GetPeerCount());
    if (connectToNPeers <= 0)
    {
        return 0;
    }

    // Step 3: Get an iterator through the data structure for our potential clients to connect to
    auto iter = potentialPeers.begin();
    // // This is a mini heuristic for random selection of peers to connect to.
    // // One might want to implement a better one
    // Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    // for (uint32_t start = uv->GetInteger(0, potentialPeers.size() - 1); start > 0; --start)
    // {
    //     ++iter;
    // }

    uint16_t peersConnected = 0; // i.e., in this call of ConnectToPeers()
    bool goOn = (iter != potentialPeers.end()) && (connectToNPeers > 0) && (GetPeerCount() < m_myClient->GetDesiredPeers());

    // Step 4: Connect to peers
    while (goOn)
    {
        // Step 4a: Only connect if we are not already connected or have a pending connection
        if (m_connectedTo.find((*iter).second.first) == m_connectedTo.end() &&
            m_pendingConnections.find((*iter).second.first) == m_pendingConnections.end())
        {
            // Step 4a1: Re-convert the IP address in integer format to a Ipv4Address object
            // TODO: We should change this back to using Ipv4Address objets only
            Ipv4Address connectToAddress;
            connectToAddress.Set((*iter).second.first);

            // Step 4a2: Create a Peer object for the new connection and schedule the connection to that peer by calling the appropriate method of the
            // Peer class
            Ptr<Peer> newPeer = CreateObject<Peer>(m_myClient);
            Simulator::ScheduleNow(&Peer::ConnectToPeer, newPeer, connectToAddress, (*iter).second.second);
            Simulator::Schedule(MilliSeconds(BT_PEER_CONNECTOR_CONNECTION_ACCEPTANCE_DELAY),
                                &PeerConnectorStrategyLive::CheckAndDisconnectIfRejected,
                                this,
                                newPeer);

            // newPeer->
            // Step 4a3: Insert this connection into the list of pending connections
            m_pendingConnections.insert((*iter).second.first);
            m_pendingConnectionToPeers[(*iter).second.first] = newPeer;
            // if(m_PeerWithToRegisterHash.find(m_connectedToPeers[(*iter).second.first]) == m_PeerWithToRegisterHash.end())
            m_PeerWithToRegisterHash[newPeer] = std::set<std::string>();
            m_PeerWithToRegisterHash[m_connectedToPeers[(*iter).second.first]].insert(iter->first);
            // m_pendingConnectionsWithPeer.insert
            // Step 4a4: Adjust our "counters"
            ++peersConnected;
            --connectToNPeers;

            NS_LOG_INFO("PeerConnectorStrategyLive: " << m_myClient->GetIp() << ": Connecting to " << connectToAddress << ":" << (*iter).second.second
                                                      << ".");
        }
        else if (m_connectedTo.find((*iter).second.first) != m_connectedTo.end())
        {
            if (m_PeerWithToRegisterHash.find(m_connectedToPeers[(*iter).second.first]) == m_PeerWithToRegisterHash.end())
                m_PeerWithToRegisterHash[m_connectedToPeers[(*iter).second.first]] = std::set<std::string>();
            m_PeerWithToRegisterHash[m_connectedToPeers[(*iter).second.first]].insert(iter->first);
            Simulator::ScheduleNow(&PeerConnectorStrategyLive::SubscribeAllStreams, this, m_connectedToPeers[(*iter).second.first]);
        }

        // Step 4b: Adjust our iterator and recalculate our "guard" goOn
        ++iter;
        goOn = (iter != potentialPeers.end()) && (connectToNPeers > 0) && (GetPeerCount() < m_myClient->GetDesiredPeers());
    }

    return peersConnected;
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
        NS_FATAL_ERROR("WE DO NOT support STRING TYPE PEERS");
        return;
        // this is the plain style, the string contains peers in the format
        // 4 byte ipaddr, 2 byte port until the end
        // Ptr<TorrentDataString> peerStringPtr = DynamicCast<TorrentDataString>(peers);
        // if (!peerStringPtr)
        // {
        //     return;
        // }
        // // NS_ASSERT(peerStringPtr);

        // const char* peerString = peerStringPtr->GetData().c_str();

        // size_t peerLen = peerStringPtr->GetData().size();
        // if (!((peerLen % 6) == 0))
        // {
        //     return;
        // }
        // // NS_ASSERT((peerLen % 6) == 0);

        // // read data sequentially
        // std::pair<uint32_t, uint16_t> peer;
        // for (uint32_t peerNum = 0; peerNum < peerLen; peerNum += 2)
        // {
        //     uint32_t ipaddr;
        //     ipaddr = peerString[peerNum++];
        //     ipaddr <<= 8;
        //     ipaddr |= peerString[peerNum++];
        //     ipaddr <<= 8;
        //     ipaddr |= peerString[peerNum++];
        //     ipaddr <<= 8;
        //     ipaddr |= peerString[peerNum++];

        //     // Skip own IP
        //     if (ipaddr == m_myClient->GetIp().Get())
        //     {
        //         continue;
        //     }

        //     peer.first = ipaddr;
        //     peer.second = (peerString[peerNum] << 8) | peerString[peerNum + 1];

        //     m_potentialClients.insert(peer);
        // }
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
        // }
        NS_LOG_INFO("NOW RECV PEER LIST WITH LENGTH " << ((peerList->GetListEnd() == peerList->GetIterator()) ? " 0" : ">0"));
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
            Ptr<TorrentDataString> peerStreamHash = DynamicCast<TorrentDataString>(currentPeer->GetData("streamHash"));
            if (!peerStreamHash)
            {
                NS_LOG_WARN("We cant find streamHash keyword in the peerlist!");
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
            m_potentialClients.insert(std::make_pair(peerStreamHash->GetData(), peer));
        }
    }

    if (!m_myClient->GetConnectedToCloud())
        m_myClient->CloudConnectionEstablishedEvent();

    m_myClient->SetConnectedToCloud(true);
}

void
PeerConnectorStrategyLive::SubscribeAllStreams(Ptr<Peer> peer)
{
    auto subIt = m_PeerWithToRegisterHash.find(peer);
    if (subIt == m_PeerWithToRegisterHash.end())
    {
        return;
    }
    for (auto streamhashIt = subIt->second.begin(); streamhashIt != subIt->second.end(); streamhashIt++)
    {
        peer->SendSubscribe(*streamhashIt);
    }
    m_PeerWithToRegisterHash.erase(subIt);
}

void
PeerConnectorStrategyLive::ProcessConnectionCloseEvent(Ptr<Peer> peer)
{
    DeleteConnection(peer->GetRemoteIp().Get()); // Already deletes only 1 entry from the multimap
}

void
PeerConnectorStrategyLive::ProcessPeerConnectionEstablishedEvent(Ptr<Peer> peer)
{
    AddConnection(peer->GetRemoteIp().Get());
    m_connectedToPeers[peer->GetRemoteIp().Get()] = peer;
    m_pendingConnections.erase(peer->GetRemoteIp().Get());
    m_pendingConnectionToPeers.erase(peer->GetRemoteIp().Get());
    Simulator::ScheduleNow(&PeerConnectorStrategyLive::SubscribeAllStreams, this, peer);

    // peer->SendBitfield();
}

void
PeerConnectorStrategyLive::ProcessPeerConnectionFailEvent(Ptr<Peer> peer)
{
    m_pendingConnections.erase(peer->GetRemoteIp().Get());
    m_pendingConnectionToPeers.erase(peer->GetRemoteIp().Get());

    if (!m_myClient->GetDownloadCompleted())
    {
        Simulator::ScheduleNow(&PeerConnectorStrategyLive::ProcessPeriodicSchedule, this);
    }
}

void
PeerConnectorStrategyLive::AddConnection(uint32_t address)
{
    m_connectedTo.insert(address);
}

void
PeerConnectorStrategyLive::DeleteConnection(uint32_t address)
{
    std::set<uint32_t>::iterator it = m_connectedTo.find(address);
    if (it != m_connectedTo.end())
    {
        m_connectedTo.erase(it);
    }
    auto Pit = m_connectedToPeers.find(address);
    if (Pit != m_connectedToPeers.end())
    {
        auto subIt = m_PeerWithToRegisterHash.find(Pit->second);
        if (subIt != m_PeerWithToRegisterHash.end())
            m_PeerWithToRegisterHash.erase(subIt);
        m_connectedToPeers.erase(Pit);
    }
}

void
PeerConnectorStrategyLive::CheckAndDisconnectIfRejected(Ptr<Peer> peer)
{
    // If for some reason, the connection to the remote peer could not be established successfully, disconnect that peer
    if (peer->GetConnectionState() != Peer::CONN_STATE_CONNECTED)
    {
        DisconnectPeerSilent(peer);
        m_pendingConnections.erase(peer->GetRemoteIp().Get());
        m_pendingConnectionToPeers.erase(peer->GetRemoteIp().Get());
    }
    else // Else, fully register the peer with the client
    {
        m_myClient->RegisterPeer(peer);
        m_myClient->PeerConnectionEstablishedEvent(peer);

        NS_LOG_INFO("PeerConnectorStrategyLive: " << m_myClient->GetIp() << ": Fully established connection with " << peer->GetRemoteIp() << ":"
                                                  << peer->GetRemotePort() << ".");
    }
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
    m_myClient->UnregisterCallbackConnectionEstablishedEvent(MakeCallback(&PeerConnectorStrategyBase::ProcessPeerConnectionEstablishedEvent, this));
    m_myClient->UnregisterCallbackConnectionFailEvent(MakeCallback(&PeerConnectorStrategyBase::ProcessPeerConnectionFailEvent, this));
    m_myClient->UnregisterCallbackConnectionCloseEvent(MakeCallback(&PeerConnectorStrategyBase::ProcessConnectionCloseEvent, this));

    m_myClient->RegisterCallbackConnectionEstablishedEvent(MakeCallback(&PeerConnectorStrategyLive::ProcessPeerConnectionEstablishedEvent, this));
    m_myClient->RegisterCallbackConnectionFailEvent(MakeCallback(&PeerConnectorStrategyLive::ProcessPeerConnectionFailEvent, this));
    m_myClient->RegisterCallbackConnectionCloseEvent(MakeCallback(&PeerConnectorStrategyLive::ProcessConnectionCloseEvent, this));

    m_myClient->RegisterCallbackGetSeederEvent(MakeCallback(&PeerConnectorStrategyLive::GetSeeder, this));
    // this covered the parent class's connectToPeers function
    m_myClient->SetCallbackConnectToPeers(MakeCallback(&PeerConnectorStrategyLive::ConnectToPeers, this));
    m_myClient->SetCallbackConnectToCloud(MakeCallback(&PeerConnectorStrategyLive::ConnectToCloud, this));

    // this covered the parent class's process periodic schedule function
    m_nextPeriodicEvent.Cancel();
    m_nextPeriodicEvent = Simulator::Schedule(m_periodicInterval, &PeerConnectorStrategyLive::ProcessPeriodicSchedule, this);
    // m_myClient->
    // m_myClient->RegisterCallbackStreamBufferReadyEvent(MakeCallback(&PeerConnectorStrategyLive::OnStreamBufferReady, this))
}

} // namespace bittorrent
} // namespace ns3
