/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010-2012 ComSys, RWTH Aachen University
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

#ifndef PEERCONNECTORSTRATEGY_LIVE_H_
#define PEERCONNECTORSTRATEGY_LIVE_H_

#include "ns3/AbstractStrategy.h"
#include "ns3/BitTorrentHttpClient.h"
#include "ns3/PeerConnectorStrategyBase.h"

#include "ns3/address.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"

#include <map>
#include <set>
#include <utility>

namespace ns3
{
namespace bittorrent
{

class BitTorrentClient;
class Peer;

/**
 * \ingroup BitTorrent
 *
 * \brief Implements a HTTP-tracker-based peer discovery mechanism for BitTorrent swarms.
 *
 * This class implements the "traditional" HTTP-tracker based method to discover and connect to peers in a BitTorrent swarm.
 * It uses a simplified HTTP client implementation provided by the BitTorrentHttpClient class
 * to communicate with both internal (i.e., ns-3 based) and external BitTorrent trackers
 * using the standardized HTTP-based BitTorrent Tracker protocol.
 *
 * This class implements several methods commonly used in BitTorrent peer discovery mechanisms and applies them to the tracker-based approach.
 * These methods can, however, be overridden in derived classes to implement other peer discovery mechanisms.
 */
class PeerConnectorStrategyLive : public PeerConnectorStrategyBase
{
private:
    std::set<std::pair<std::string, std::pair<uint32_t, uint16_t>>> m_potentialClients;
    std::map<Ptr<Peer>, std::set<std::string>> m_PeerWithToRegisterHash;                    // IP addresses (in integer representation) of clients we are already connected to
    std::map<uint32_t, Ptr<Peer>> m_connectedToPeers;                    // IP addresses (in integer representation) of clients we are already connected to
    std::map<uint32_t, Ptr<Peer>> m_pendingConnectionToPeers;                    // IP addresses (in integer representation) of clients we are already connected to
    EventId m_reportLoadEvent;
    EventId m_getSeederEvent;
    // std::map<uint32_t, uint32_t> m_pendingConnectionsWithPeer;             // IP addresses of clients we are currently trying to establish a connection with
public:
    PeerConnectorStrategyLive(Ptr<BitTorrentClient> myClient);
    ~PeerConnectorStrategyLive() override;

  protected:
    void ProcessPeriodicSchedule() override;
    void ProcessPeriodicReannouncements () override;
    void ConnectToCloud() override;
    void GetSeeder(std::string streamHash);
    uint16_t ConnectToPeers (uint16_t count) override;
    uint16_t GetPeerCount() const override;
    /**
     * \brief Parse the bencoded response received from the peer discovery mechanism.
     *
     * This method inserts peers found within the response into the internal list of available peers.
     *
     * @param response an istream object containing the response of the peer discovery mechanism.
     */
    void ParseResponse(std::istream& response) override;
    void SubscribeAllStreams(Ptr<Peer> peer);
    void ProcessConnectionCloseEvent(Ptr<Peer> peer) override;
    void ProcessPeerConnectionEstablishedEvent(Ptr<Peer> peer) override;
    void ProcessPeerConnectionFailEvent(Ptr<Peer> peer) override;
    void AddConnection(uint32_t address) override;
    void DeleteConnection(uint32_t address) override;

    void CheckAndDisconnectIfRejected(Ptr<Peer> peer) override;

  public:
    /**
     * \brief Get the list of clients that the client has so far discovered.
     *
     * @returns a list of <IP, port> pairs representing the clients so far discovered.
     */
    const std::set<std::pair<std::string, std::pair<uint32_t, uint16_t>>>& GetPotentialClients() const;
    void DoInitialize() override;
    void StartListening(uint16_t port) override;
    bool AcceptCallback(Ptr<Socket> sock, const Address& addr) override;
    void NewConnectionCreatedCallback(Ptr<Socket> sock, const Address& addr) override;
    void DisconnectFromCloud() override;
    // void TrackerResponseEvent(Ptr<Socket> socket);
    // bool ContactTracker(TrackerContactReason event,
    //                     uint16_t numwant,
    //                     std::map<std::string, std::string> additionalParameters,
    //                     bool closeCurrentConnection);
    // bool ContactTracker (TrackerContactReason event, uint16_t numwant, std::map<std::string, std::string> additionalParameters, bool
    // closeCurrentConnection) override;
};

} // namespace bittorrent
} // namespace ns3

#endif /* PEERCONNECTORSTRATEGY_LIVE_H_ */
