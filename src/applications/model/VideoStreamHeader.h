#ifndef VIDEO_STREAM_HEADER_H
#define VIDEO_STREAM_HEADER_H

#include <ns3/header.h>
#include <ns3/nstime.h>


namespace ns3 {

class Packet;

/**
 * \ingroup http
 * \brief Header used by web browsing applications to transmit information about
 *        content type, content length and timestamps for delay statistics.
 *
 * The header contains the following fields (and their respective size when
 * serialized):
 *   - content type (2 bytes);
 *   - content length (4 bytes);
 *   - client time stamp (8 bytes); and
 *   - server time stamp (8 bytes).
 *
 * The header is attached to every packet transmitted by ThreeGppHttpClient and
 * ThreeGppHttpServer applications. In received, split packets, only the first packet
 * of transmitted object contains the header, which helps to identify how many bytes are
 * left to be received.
 *
 * The last 2 fields allow the applications to compute the propagation delay of
 * each packet. The *client TS* field indicates the time when the request
 * packet is sent by the ThreeGppHttpClient, while the *server TS* field indicates the
 * time when the response packet is sent by the ThreeGppHttpServer.
 */
class VideoStreamHeader : public Header
{
public:
  /// Creates an empty instance	.
  VideoStreamHeader ();

  /**
   * Returns the object TypeId.
   * \return The object TypeId.
   */
  static TypeId GetTypeId ();

  // Inherited from ObjectBase base class.
  virtual TypeId GetInstanceTypeId () const;

  // Inherited from Header base class.
  static uint32_t GetHeaderSize();
  virtual uint32_t GetSerializedSize () const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;

  /**
   * \return The string representation of the header.
   */
  std::string ToString () const;

  /// The possible types of content (default = NOT_SET).
  enum ContentType_t
  {
    VIDEO_NOT_SET,         ///< Integer equivalent = 0.
    VIDEO_CHUNK,     ///< Integer equivalent = 1.
    VIDEO_PEER,  ///< Integer equivalent = 2.
    VIDEO_CREATE_SUB, ///< Integer equivalent = 3;
    VIDEO_DESTORY_SUB, ///< Integer equivalent = 4;
    VIDEO_MAX_TYPE
  };

//   /**
//    * \param contentType The content type.
//    */
  void SetContentType (ContentType_t contentType);

//   /**
//    * \return The content type.
//    */
  ContentType_t GetContentType () const;

//   /**
//    * \param contentLength The content length (in bytes).
//    */
//   void SetContentLength (uint32_t contentLength);

//   /**
//    * \return The content length (in bytes).
//    */
//   uint32_t GetContentLength () const;

  /**
   * \param contentLength The content length (in bytes).
   */
  void SetBitrateType (uint32_t bitrateType);

  /**
   * \return The content length (in bytes).
   */
  uint32_t GetBitrateType () const;

    /**
   * \param contentLength The content length (in bytes).
   */
  void SetVideoNumber (uint32_t videoNumber);

  /**
   * \return The content length (in bytes).
   */
  uint32_t GetVideoNumber () const;

    /**
   * \param contentLength The content length (in bytes).
   */
  void SetVideoChunkNumber (uint32_t videoChunkNumber);

  /**
   * \return The content length (in bytes).
   */
  uint32_t GetVideoChunkNumber () const;


    void SetVideoChunkTotalNumber (uint32_t videoChunkTotalNumber);

  /**
   * \return The content length (in bytes).
   */
  uint32_t GetVideoChunkTotalNumber () const;

  uint32_t GetVideoChunkSize() const;

  void setVideoChunkSize(uint32_t chunkSize);
//   void SetEnbId (uint32_t enbId);

//   /**
//    * \return The content length (in bytes).
//    */
//   uint32_t GetEnbId () const;


//   void SetMecId (uint32_t mecId);

//   /**
//    * \return The content length (in bytes).
//    */
//   uint32_t GetMecId () const;


//   void SetCacheId (uint32_t cacheId);

//   /**
//    * \return The content length (in bytes).
//    */
//   uint32_t GetCacheId () const;


//   /**
//    * \param clientTs The client time stamp.
//    */
//   void SetClientTs (Time clientTs);

//   /**
//    * \return The client time stamp.
//    */
//   Time GetClientTs () const;

//   /**
//    * \param serverTs The server time stamp.
//    */
//   void SetServerTs (Time serverTs);

//   /**
//    * \return The server time stamp.
//    */
//   Time GetServerTs () const;

private:
  uint16_t m_contentType;   //!<" Content type field in integer format.
//   uint32_t m_contentLength; //!<" Content length field (in bytes unit).
//   uint64_t m_clientTs;      //!<" Client time stamp field (in time step unit).
//   uint64_t m_serverTs;      //!<" Server time stamp field (in time step unit).
  uint32_t m_bitrateType;   //to provide bitrate selection
  uint32_t m_videoNumber;   //the number of video requested
  uint32_t m_videoChunkNumber; //the number of video chunk requested
  uint32_t m_videoChunkTotalNumber; //the total number of video chunk requested;
  uint32_t m_chunkSize;

//   uint32_t m_enbId;
//   uint32_t m_mecId;
//   uint32_t m_cacheId;
}; // end of `class VideoStreamHeader`


} // end of `namespace ns3`


#endif /* THREE_GPP_HTTP_HEADER_H */
