/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Magister Solutions
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
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#include <ns3/log.h>
#include <ns3/packet.h>
#include <sstream>
#include "VideoStreamHeader.h"

NS_LOG_COMPONENT_DEFINE ("VideoStreamHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (VideoStreamHeader);

VideoStreamHeader::VideoStreamHeader ()
  : Header (),
  m_contentType (VIDEO_NOT_SET),
//   m_contentLength (0),
//   m_clientTs (0),
//   m_serverTs (0),
  m_bitrateType (0),
  m_videoNumber (0),
  m_videoChunkNumber (0),
  m_videoChunkTotalNumber (0),
  m_chunkSize(0)
//   m_enbId (0),
//   m_mecId (0),
//   m_cacheId (0)
{
  NS_LOG_FUNCTION (this);
}


// static
TypeId
VideoStreamHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::VideoStreamHeader")
    .SetParent<Header> ()
    .AddConstructor<VideoStreamHeader> ()
  ;
  return tid;
}


TypeId
VideoStreamHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
VideoStreamHeader::GetHeaderSize()
{
    return 2 + 4 + 4 + 4 + 4 + 4;
}

uint32_t
VideoStreamHeader::GetSerializedSize() const
{
  return GetHeaderSize();
}


void
VideoStreamHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  start.WriteU16 (m_contentType);
//   start.WriteU32 (m_contentLength);
//   start.WriteU64 (m_clientTs);
//   start.WriteU64 (m_serverTs);
  start.WriteU32 (m_bitrateType);
  start.WriteU32 (m_videoNumber);
  start.WriteU32 (m_videoChunkNumber);
  start.WriteU32 (m_videoChunkTotalNumber);
  start.WriteU32 (m_chunkSize);
//   start.WriteU32 (m_enbId);
//   start.WriteU32 (m_mecId);
//   start.WriteU32 (m_cacheId);
}


uint32_t
VideoStreamHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  uint32_t bytesRead = 0;

  // First block of 2 bytes (content type)
  m_contentType = start.ReadU16 ();
  bytesRead += 2;

  // Second block of 4 bytes (content length)
//   m_contentLength = start.ReadU32 ();
//   bytesRead += 4;

  // Second block of 4 bytes (content length)
  //m_bitrateType = start.ReadU32 ();
  //bytesRead += 4;

  // Third block of 8 bytes (client time stamp)
//   m_clientTs = start.ReadU64 ();
//   bytesRead += 8;

  // Fourth block of 8 bytes (server time stamp)
//   m_serverTs = start.ReadU64 ();
//   bytesRead += 8;

  // Fourth block of 8 bytes (server time stamp)
  m_bitrateType = start.ReadU32 ();
  bytesRead += 4;

  // Fourth block of 8 bytes (server time stamp)
  m_videoNumber = start.ReadU32 ();
  bytesRead += 4;

  m_videoChunkNumber = start.ReadU32 ();
  bytesRead += 4;

  m_videoChunkTotalNumber = start.ReadU32 ();
  bytesRead += 4;

  m_chunkSize = start.ReadU32 ();
  bytesRead += 4;
//   m_enbId = start.ReadU32 ();
//   bytesRead += 4;
  
//   m_mecId = start.ReadU32 ();
//   bytesRead += 4;

//   m_cacheId = start.ReadU32 ();
//   bytesRead += 4;

  return bytesRead;
}


void
VideoStreamHeader::Print (std::ostream &os) const
{
    /*
    << "(Content-Type: " << m_contentType
     << " Content-Length: " << m_contentLength
     << " Client TS: " << TimeStep (m_clientTs).GetSeconds ()
     << " Server TS: " << TimeStep (m_serverTs).GetSeconds ()
          << " EnbId: " << m_enbId
     << " MecId: " << m_mecId
     << " CacheId: " << m_cacheId 
    */
  NS_LOG_FUNCTION (this << &os);
  os << " (BitrateType: " << m_bitrateType
     << " ContentType: " << m_contentType
     << " VideoNumber: " << m_videoNumber 
     << " VideoChunkNumber: " << m_videoChunkNumber 
     << " VideoChunkTotalNumber: " << m_videoChunkTotalNumber 
     << " ChunkSize:" << m_chunkSize
     << ")";
}


std::string
VideoStreamHeader::ToString () const
{
  NS_LOG_FUNCTION (this);
  std::ostringstream oss;
  Print (oss);
  return oss.str ();
}


void
VideoStreamHeader::SetContentType (VideoStreamHeader::ContentType_t contentType)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (contentType));
  if(contentType >= VIDEO_NOT_SET && contentType < VIDEO_MAX_TYPE)
  {
    m_contentType = (ContentType_t) contentType;
  }
  else
  {
    NS_FATAL_ERROR ("Unknown Content-Type: " << contentType);
  }
}


VideoStreamHeader::ContentType_t
VideoStreamHeader::GetContentType () const
{
  ContentType_t ret;
  if(m_contentType >= VIDEO_NOT_SET && m_contentType < VIDEO_MAX_TYPE)
  {
    ret = (ContentType_t) m_contentType;
  }
  else
  {
    NS_FATAL_ERROR ("Unknown Content-Type: " << m_contentType);
  }
  return ret;
}


// void
// VideoStreamHeader::SetContentLength (uint32_t contentLength)
// {
//   NS_LOG_FUNCTION (this << contentLength);
//   m_contentLength = contentLength;
// }


// uint32_t
// VideoStreamHeader::GetContentLength () const
// {
//   return m_contentLength;
// }
uint32_t VideoStreamHeader::GetVideoChunkSize() const
{
  return m_chunkSize;

}

void VideoStreamHeader::setVideoChunkSize(uint32_t chunkSize)
{
  NS_LOG_FUNCTION (this << chunkSize);
  m_chunkSize = chunkSize;

}


void
VideoStreamHeader::SetBitrateType (uint32_t bitrateType)
{
  NS_LOG_FUNCTION (this << bitrateType);
  m_bitrateType = bitrateType;
}


uint32_t
VideoStreamHeader::GetBitrateType () const
{
  return m_bitrateType;
}

void
VideoStreamHeader::SetVideoNumber (uint32_t viedeoNumber)
{
  NS_LOG_FUNCTION (this << viedeoNumber);
  m_videoNumber = viedeoNumber;
}


uint32_t
VideoStreamHeader::GetVideoNumber () const
{
  return m_videoNumber;
}

void
VideoStreamHeader::SetVideoChunkNumber (uint32_t viedeoChunkNumber)
{
  NS_LOG_FUNCTION (this << viedeoChunkNumber);
  m_videoChunkNumber = viedeoChunkNumber;
}


uint32_t
VideoStreamHeader::GetVideoChunkNumber () const
{
  return m_videoChunkNumber;
}


// void
// VideoStreamHeader::SetEnbId (uint32_t enbId)
// {
//   NS_LOG_FUNCTION (this << enbId);
//   m_enbId = enbId;
// }


// uint32_t
// VideoStreamHeader::GetEnbId () const
// {
//   return m_enbId;
// }

// void
// VideoStreamHeader::SetMecId (uint32_t mecId)
// {
//   NS_LOG_FUNCTION (this << mecId);
//   m_mecId = mecId;
// }


// uint32_t
// VideoStreamHeader::GetMecId () const
// {
//   return m_mecId;
// }

// void
// VideoStreamHeader::SetCacheId (uint32_t cacheId)
// {
//   NS_LOG_FUNCTION (this << cacheId);
//   m_cacheId = cacheId;
// }


// uint32_t
// VideoStreamHeader::GetCacheId () const
// {
//   return m_cacheId;
// }

void
VideoStreamHeader::SetVideoChunkTotalNumber (uint32_t viedeoChunkTotalNumber)
{
  NS_LOG_FUNCTION (this << viedeoChunkTotalNumber);
  m_videoChunkTotalNumber = viedeoChunkTotalNumber;
}


uint32_t
VideoStreamHeader::GetVideoChunkTotalNumber () const
{
  return m_videoChunkTotalNumber;
}

// void
// VideoStreamHeader::SetClientTs (Time clientTs)
// {
//   NS_LOG_FUNCTION (this << clientTs.GetSeconds ());
//   m_clientTs = clientTs.GetTimeStep ();
// }


// Time
// VideoStreamHeader::GetClientTs () const
// {
//   return TimeStep (m_clientTs);
// }


// void
// VideoStreamHeader::SetServerTs (Time serverTs)
// {
//   NS_LOG_FUNCTION (this << serverTs.GetSeconds ());
//   m_serverTs = serverTs.GetTimeStep ();
// }


// Time
// VideoStreamHeader::GetServerTs () const
// {
//   return TimeStep (m_serverTs);
// }


} // namespace ns3
