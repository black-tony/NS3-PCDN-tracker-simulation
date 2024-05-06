/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010-2011 ComSys, RWTH Aachen University
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
 * Authors: Rene Glebke, Martin Lang
 */

#include "ProtocolFactory.h"

#include "BitTorrentVideoMetricsBase.h"
#include "ChokeUnChokeStrategyBase.h"
#include "PartSelectionStrategyBase.h"
#include "PeerConnectorStrategyBase.h"
#include "RequestSchedulingStrategyBase.h"
#include "strategies/RarestFirstPartSelectionStrategy.h"
#include "streaming/PeerConnectorStrategyLive.h"
#include "streaming/RequestSchedulingStrategyLive.h"
namespace ns3
{
namespace bittorrent
{

NS_LOG_COMPONENT_DEFINE("bittorrent::ProtocolFactory");
NS_OBJECT_ENSURE_REGISTERED(ProtocolFactory);

ProtocolFactory::ProtocolFactory()
{
}

ProtocolFactory::~ProtocolFactory()
{
}

void
ProtocolFactory::CreateStrategyBundle(std::string protocolName,
                                      Ptr<BitTorrentClient> client,
                                      std::list<Ptr<AbstractStrategy>>& strategyStore,
                                      Ptr<PeerConnectorStrategyBase>& aPeerConnectorStrategy)
{
    if (protocolName == "default")
    {
        CreateBasicProtocol(client, strategyStore, aPeerConnectorStrategy);
    }
    else if (protocolName == "rarest-first")
    {
        CreateRarestFirstProtocol(client, strategyStore, aPeerConnectorStrategy);
    }
    else if (protocolName == "rarest-first-vod")
    {
        CreateRarestFirstVoDProtocol(client, strategyStore, aPeerConnectorStrategy);
    }
    else if (protocolName == "bitos")
    {
        CreateBiToSProtocol(client, strategyStore, aPeerConnectorStrategy);
    }
    else if (protocolName == "give-to-get")
    {
        CreateGiveToGetProtocol(client, strategyStore, aPeerConnectorStrategy);
    }
    else if (protocolName == "live-streaming-client")
    {
        CreateLiveStreamingClientProtocol(client, strategyStore, aPeerConnectorStrategy);
    }
    else if (protocolName == "live-streaming-box")
    {
        CreateLiveStreamingBoxProtocol(client, strategyStore, aPeerConnectorStrategy);
    }
    else if (protocolName == "live-streaming-server")
    {
        CreateLiveStreamingServerProtocol(client, strategyStore, aPeerConnectorStrategy);
    }
    else
    {
        CreateBasicProtocol(client, strategyStore, aPeerConnectorStrategy);
    }
}

void
ProtocolFactory::CreateBasicProtocol(Ptr<BitTorrentClient> client,
                                     std::list<Ptr<AbstractStrategy>>& strategyStore,
                                     Ptr<PeerConnectorStrategyBase>& aPeerConnectorStrategy)
{
    Ptr<PeerConnectorStrategyBase> peerConnectorStrategy = Ptr<PeerConnectorStrategyBase>(new PeerConnectorStrategyBase(client), false);
    strategyStore.push_back(peerConnectorStrategy);
    peerConnectorStrategy->DoInitialize();

    Ptr<ChokeUnChokeStrategyBase> chokeUnChokeStrategy = Ptr<ChokeUnChokeStrategyBase>(new ChokeUnChokeStrategyBase(client), false);
    strategyStore.push_back(chokeUnChokeStrategy);
    chokeUnChokeStrategy->DoInitialize();

    Ptr<PartSelectionStrategyBase> partSelectionStrategy = Ptr<PartSelectionStrategyBase>(new PartSelectionStrategyBase(client), false);
    strategyStore.push_back(partSelectionStrategy);
    partSelectionStrategy->DoInitialize();

    Ptr<RequestSchedulingStrategyBase> requestSchedulingStrategy =
        Ptr<RequestSchedulingStrategyBase>(new RequestSchedulingStrategyBase(client), false);
    strategyStore.push_back(requestSchedulingStrategy);
    requestSchedulingStrategy->DoInitialize();

    aPeerConnectorStrategy = peerConnectorStrategy;
}
void
ProtocolFactory::CreateLiveStreamingBoxProtocol(Ptr<BitTorrentClient> client,
                                     std::list<Ptr<AbstractStrategy>>& strategyStore,
                                     Ptr<PeerConnectorStrategyBase>& aPeerConnectorStrategy)
{
    Ptr<PeerConnectorStrategyLive> peerConnectorStrategy = Ptr<PeerConnectorStrategyLive>(new PeerConnectorStrategyLive(client), false);
    strategyStore.push_back(peerConnectorStrategy);
    peerConnectorStrategy->DoInitialize();

    // Ptr<ChokeUnChokeStrategyBase> chokeUnChokeStrategy = Ptr<ChokeUnChokeStrategyBase>(new ChokeUnChokeStrategyBase(client), false);
    // strategyStore.push_back(chokeUnChokeStrategy);
    // chokeUnChokeStrategy->DoInitialize();

    // Ptr<PartSelectionStrategyBase> partSelectionStrategy = Ptr<PartSelectionStrategyBase>(new PartSelectionStrategyBase(client), false);
    // strategyStore.push_back(partSelectionStrategy);
    // partSelectionStrategy->DoInitialize();

    Ptr<RequestSchedulingStrategyLive> requestSchedulingStrategy =
        Ptr<RequestSchedulingStrategyLive>(new RequestSchedulingStrategyLive(client), false);
    strategyStore.push_back(requestSchedulingStrategy);
    requestSchedulingStrategy->DoInitialize();

    aPeerConnectorStrategy = peerConnectorStrategy;
}
void
ProtocolFactory::CreateLiveStreamingServerProtocol(Ptr<BitTorrentClient> client,
                                     std::list<Ptr<AbstractStrategy>>& strategyStore,
                                     Ptr<PeerConnectorStrategyBase>& aPeerConnectorStrategy)
{
    // Ptr<PeerConnectorStrategyBase> peerConnectorStrategy = Ptr<PeerConnectorStrategyBase>(new PeerConnectorStrategyBase(client), false);
    // strategyStore.push_back(peerConnectorStrategy);
    // peerConnectorStrategy->DoInitialize();

    // Ptr<ChokeUnChokeStrategyBase> chokeUnChokeStrategy = Ptr<ChokeUnChokeStrategyBase>(new ChokeUnChokeStrategyBase(client), false);
    // strategyStore.push_back(chokeUnChokeStrategy);
    // chokeUnChokeStrategy->DoInitialize();

    // Ptr<PartSelectionStrategyBase> partSelectionStrategy = Ptr<PartSelectionStrategyBase>(new PartSelectionStrategyBase(client), false);
    // strategyStore.push_back(partSelectionStrategy);
    // partSelectionStrategy->DoInitialize();

    // Ptr<RequestSchedulingStrategyBase> requestSchedulingStrategy =
    //     Ptr<RequestSchedulingStrategyBase>(new RequestSchedulingStrategyBase(client), false);
    // strategyStore.push_back(requestSchedulingStrategy);
    // requestSchedulingStrategy->DoInitialize();

    // aPeerConnectorStrategy = peerConnectorStrategy;
}
void
ProtocolFactory::CreateLiveStreamingClientProtocol(Ptr<BitTorrentClient> client,
                                     std::list<Ptr<AbstractStrategy>>& strategyStore,
                                     Ptr<PeerConnectorStrategyBase>& aPeerConnectorStrategy)
{
    // Ptr<PeerConnectorStrategyBase> peerConnectorStrategy = Ptr<PeerConnectorStrategyBase>(new PeerConnectorStrategyBase(client), false);
    // strategyStore.push_back(peerConnectorStrategy);
    // peerConnectorStrategy->DoInitialize();

    // Ptr<ChokeUnChokeStrategyBase> chokeUnChokeStrategy = Ptr<ChokeUnChokeStrategyBase>(new ChokeUnChokeStrategyBase(client), false);
    // strategyStore.push_back(chokeUnChokeStrategy);
    // chokeUnChokeStrategy->DoInitialize();

    // Ptr<PartSelectionStrategyBase> partSelectionStrategy = Ptr<PartSelectionStrategyBase>(new PartSelectionStrategyBase(client), false);
    // strategyStore.push_back(partSelectionStrategy);
    // partSelectionStrategy->DoInitialize();

    // Ptr<RequestSchedulingStrategyBase> requestSchedulingStrategy =
    //     Ptr<RequestSchedulingStrategyBase>(new RequestSchedulingStrategyBase(client), false);
    // strategyStore.push_back(requestSchedulingStrategy);
    // requestSchedulingStrategy->DoInitialize();

    // aPeerConnectorStrategy = peerConnectorStrategy;
}
void
ProtocolFactory::CreateRarestFirstProtocol(Ptr<BitTorrentClient> client,
                                           std::list<Ptr<AbstractStrategy>>& strategyStore,
                                           Ptr<PeerConnectorStrategyBase>& aPeerConnectorStrategy)
{
    Ptr<PeerConnectorStrategyBase> peerConnectorStrategy = Ptr<PeerConnectorStrategyBase>(new PeerConnectorStrategyBase(client), false);
    strategyStore.push_back(peerConnectorStrategy);
    peerConnectorStrategy->DoInitialize();

    Ptr<ChokeUnChokeStrategyBase> chokeUnChokeStrategy = Ptr<ChokeUnChokeStrategyBase>(new ChokeUnChokeStrategyBase(client), false);
    strategyStore.push_back(chokeUnChokeStrategy);
    chokeUnChokeStrategy->DoInitialize();

    Ptr<RarestFirstPartSelectionStrategy> partSelectionStrategy =
        Ptr<RarestFirstPartSelectionStrategy>(new RarestFirstPartSelectionStrategy(client), false);
    strategyStore.push_back(partSelectionStrategy);
    partSelectionStrategy->DoInitialize();

    Ptr<RequestSchedulingStrategyBase> requestSchedulingStrategy =
        Ptr<RequestSchedulingStrategyBase>(new RequestSchedulingStrategyBase(client), false);
    strategyStore.push_back(requestSchedulingStrategy);
    requestSchedulingStrategy->DoInitialize();

    aPeerConnectorStrategy = peerConnectorStrategy;
}

void
ProtocolFactory::CreateRarestFirstVoDProtocol(Ptr<BitTorrentClient> client,
                                              std::list<Ptr<AbstractStrategy>>& strategyStore,
                                              Ptr<PeerConnectorStrategyBase>& aPeerConnectorStrategy)
{
    Ptr<PeerConnectorStrategyBase> peerConnectorStrategy = Ptr<PeerConnectorStrategyBase>(new PeerConnectorStrategyBase(client), false);
    strategyStore.push_back(peerConnectorStrategy);
    peerConnectorStrategy->DoInitialize();

    Ptr<ChokeUnChokeStrategyBase> chokeUnChokeStrategy = Ptr<ChokeUnChokeStrategyBase>(new ChokeUnChokeStrategyBase(client), false);
    strategyStore.push_back(chokeUnChokeStrategy);
    chokeUnChokeStrategy->DoInitialize();

    //	Ptr<RarestFirstVoDPartSelectionStrategy> partSelectionStrategy = Create<RarestFirstVoDPartSelectionStrategy, Ptr<BitTorrentClient> >(client);
    //	strategyStore.push_back(partSelectionStrategy);
    //	partSelectionStrategy->DoInitialize();

    Ptr<RequestSchedulingStrategyBase> requestSchedulingStrategy =
        Ptr<RequestSchedulingStrategyBase>(new RequestSchedulingStrategyBase(client), false);
    strategyStore.push_back(requestSchedulingStrategy);
    requestSchedulingStrategy->DoInitialize();

    Ptr<BitTorrentVideoMetricsBase> videoMetricsBase = Ptr<BitTorrentVideoMetricsBase>(new BitTorrentVideoMetricsBase(client), false);
    strategyStore.push_back(videoMetricsBase);
    videoMetricsBase->DoInitialize();

    aPeerConnectorStrategy = peerConnectorStrategy;
}

void
ProtocolFactory::CreateBiToSProtocol(Ptr<BitTorrentClient> client,
                                     std::list<Ptr<AbstractStrategy>>& strategyStore,
                                     Ptr<PeerConnectorStrategyBase>& aPeerConnectorStrategy)
{
    Ptr<PeerConnectorStrategyBase> peerConnectorStrategy = Ptr<PeerConnectorStrategyBase>(new PeerConnectorStrategyBase(client), false);
    strategyStore.push_back(peerConnectorStrategy);
    peerConnectorStrategy->DoInitialize();

    Ptr<ChokeUnChokeStrategyBase> chokeUnChokeStrategy = Ptr<ChokeUnChokeStrategyBase>(new ChokeUnChokeStrategyBase(client), false);
    strategyStore.push_back(chokeUnChokeStrategy);
    chokeUnChokeStrategy->DoInitialize();

    //	Ptr<BiToSPartSelectionStrategy> partSelectionStrategy = Create<BiToSPartSelectionStrategy, Ptr<BitTorrentClient> >(client);
    //	strategyStore.push_back(partSelectionStrategy);
    //	partSelectionStrategy->DoInitialize();

    Ptr<RequestSchedulingStrategyBase> requestSchedulingStrategy =
        Ptr<RequestSchedulingStrategyBase>(new RequestSchedulingStrategyBase(client), false);
    // Create<RequestSchedulingStrategyBase, Ptr<BitTorrentClient > > (client);
    strategyStore.push_back(requestSchedulingStrategy);
    requestSchedulingStrategy->DoInitialize();

    Ptr<BitTorrentVideoMetricsBase> videoMetricsBase = Ptr<BitTorrentVideoMetricsBase>(new BitTorrentVideoMetricsBase(client), false);
    strategyStore.push_back(videoMetricsBase);
    videoMetricsBase->DoInitialize();

    aPeerConnectorStrategy = peerConnectorStrategy;
}

void
ProtocolFactory::CreateGiveToGetProtocol(Ptr<BitTorrentClient> client,
                                         std::list<Ptr<AbstractStrategy>>& strategyStore,
                                         Ptr<PeerConnectorStrategyBase>& aPeerConnectorStrategy)
{
    Ptr<PeerConnectorStrategyBase> peerConnectorStrategy = Ptr<PeerConnectorStrategyBase>(new PeerConnectorStrategyBase(client), false);
    strategyStore.push_back(peerConnectorStrategy);
    peerConnectorStrategy->DoInitialize();

    //	Ptr<GTGChokeUnChokeStrategy> chokeUnChokeStrategy = Create<GTGChokeUnChokeStrategy, Ptr<BitTorrentClient> >(client);
    //	strategyStore.push_back(chokeUnChokeStrategy);
    //	chokeUnChokeStrategy->DoInitialize();

    //	Ptr<GTGPartSelectionStrategy> partSelectionStrategy = Create<GTGPartSelectionStrategy, Ptr<BitTorrentClient> >(client);
    //	strategyStore.push_back(partSelectionStrategy);
    //	partSelectionStrategy->DoInitialize();

    Ptr<RequestSchedulingStrategyBase> requestSchedulingStrategy =
        Ptr<RequestSchedulingStrategyBase>(new RequestSchedulingStrategyBase(client), false);
    strategyStore.push_back(requestSchedulingStrategy);
    requestSchedulingStrategy->DoInitialize();

    Ptr<BitTorrentVideoMetricsBase> videoMetricsBase = Ptr<BitTorrentVideoMetricsBase>(new BitTorrentVideoMetricsBase(client), false);
    strategyStore.push_back(videoMetricsBase);
    videoMetricsBase->DoInitialize();

    aPeerConnectorStrategy = peerConnectorStrategy;
}

} // namespace bittorrent
} // namespace ns3
