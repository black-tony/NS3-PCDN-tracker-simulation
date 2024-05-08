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

#include "RequestSchedulingStrategyLive.h"

#include "ns3/BitTorrentClient.h"
#include "ns3/BitTorrentPeer.h"
#include "ns3/log.h"

namespace ns3
{
namespace bittorrent
{

NS_LOG_COMPONENT_DEFINE("bittorrent::RequestSchedulingStrategyLive");
NS_OBJECT_ENSURE_REGISTERED(RequestSchedulingStrategyLive);

RequestSchedulingStrategyLive::RequestSchedulingStrategyLive(Ptr<BitTorrentClient> myClient)
    : RequestSchedulingStrategyBase(myClient)
{
    NS_LOG_FUNCTION_NOARGS();
}

RequestSchedulingStrategyLive::~RequestSchedulingStrategyLive()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
RequestSchedulingStrategyLive::DoInitialize()
{
    NS_LOG_FUNCTION_NOARGS();

    RequestSchedulingStrategyBase::DoInitialize();

    m_myClient->RegisterCallbackStreamBufferReadyEvent(MakeCallback(&RequestSchedulingStrategyLive::ProcessStreamBufferReadyEvent, this));
}

void
RequestSchedulingStrategyLive::ProcessStreamBufferReadyEvent(std::string streamHash)
{
    if (m_myClient->GetClientType() == BT_STREAM_PEERTYPE_CLIENT)
    {
        return;
    }
    int peernum = 0;
    for (auto it = m_myClient->GetSubscriptionListIterator(streamHash); it != m_myClient->GetSubscriptionListEnd(streamHash); it++)
    {
        (*it)->SendSegment(streamHash); // YTODO Block Infos

        peernum++;
    }
    NS_LOG_INFO("RequestSchedulingStrategyLive client" << m_myClient->GetSelfRepresent() << " stream buffer ready for " << streamHash
                                                       << " to peernum " << peernum);
}

// void RequestSchedulingStrateyLive::ProcessPeerRequestEvent (Ptr<Peer> peer, uint32_t pieceIndex, uint32_t blockOffset, uint32_t blockLength)
// {
//   // Only send out if we are not choking the peer...
//   if (!peer->GetAmChoking ())
//     {
//       // and if the requested piece (and hence, the block) is available
//       if ((*m_myClient->GetBitfield ())[pieceIndex / 8] & (0x01 << (7 - pieceIndex % 8)))
//         {
//           peer->SendBlock (pieceIndex, blockOffset, blockLength);
//         }
//     }
// }

} // namespace bittorrent
} // namespace ns3
