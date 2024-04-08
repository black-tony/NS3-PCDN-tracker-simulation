/**
 * Author: Yinghao Yang <blacktonyrl@gmail.com>
 *
 */
#ifndef VIDEO_STREAM_PCDN_H
#define VIDEO_STREAM_PCDN_H
#include "VideoStreamHeader.h"
#include "three-gpp-http-header.h"
#include "three-gpp-http-variables.h"

#include <ns3/address.h>
#include <ns3/application.h>
#include <ns3/event-id.h>
#include <ns3/nstime.h>
#include <ns3/ptr.h>
#include <ns3/simple-ref-count.h>
#include <ns3/traced-callback.h>

#include <map>
#include <vector>
#include <ostream>
#include <queue>
#include <set>

namespace ns3
{

class Socket;
class Packet;
class ThreeGppHttpVariables;
class VideoStreamPCDNTxBuffer;

/**
 * \ingroup http
 * Model application which simulates the traffic of a web server. This
 * application works in conjunction with ThreeGppHttpClient applications.
 *
 * The application works by responding to requests. Each request is a small
 * packet of data which contains ThreeGppHttpHeader. The value of the *content
 * type* field of the header determines the type of object that the client is
 * requesting. The possible type is either a *main object* or an *embedded
 * object*.
 *
 * The application is responsible to generate the right type of object and send
 * it back to the client. The size of each object to be sent is randomly
 * determined (see ThreeGppHttpVariables). Each object may be sent as multiple packets
 * due to limited socket buffer space.
 *
 * To assist with the transmission, the application maintains several instances
 * of VideoStreamPCDNTxBuffer. Each instance keeps track of the object type to be
 * served and the number of bytes left to be sent.
 *
 * The application accepts connection request from clients. Every connection is
 * kept open until the client disconnects.
 */
class VideoStreamPCDN : public Application
{
  public:
    /**
     * Creates a new instance of HTTP server application.
     *
     * After creation, the application must be further configured through
     * attributes. To avoid having to do this process manually, please use
     * VideoSteamPCDNHelper.
     *
     * Upon creation, the application randomly determines the MTU size that it
     * will use (either 536 or 1460 bytes). The chosen size will be used while
     * creating the listener socket.
     */
    VideoStreamPCDN();

    /**
     * Returns the object TypeId.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Sets the maximum transmission unit (MTU) size used by the application.
     *
     * This overrides the MTU size which is randomly determined once the
     * application is created. Values other than the standard 536 and 1460 bytes
     * can be set using this method.
     *
     * \param mtuSize MTU size in bytes.
     */
    void SetMtuSize(uint32_t mtuSize);

    /**
     * Returns a pointer to the listening socket.
     * \return Pointer to the listening socket
     */
    Ptr<Socket> GetServerSocket() const;

    Ptr<Socket> GetClientSocket() const;

    /// The possible states of the application.
    enum StateServer_t
    {
        NOT_STARTED = 0, ///< Before StartApplication() is invoked.
        STARTED,         ///< Passively listening and responding to requests.
        STOPPED          ///< After StopApplication() is invoked.
    };

    enum StateClient_t
    {
        /// Before StartApplication() is invoked.
        CLIENT_NOT_STARTED = 0,
        /// Sent the server a connection request and waiting for the server to be accept it.
        CLIENT_CONNECTING,
        /// Sent the server a request for a main object and waiting to receive the packets.
        CLIENT_EXPECTING_MAIN_OBJECT,
        /// Parsing a main object that has just been received.
        CLIENT_PARSING_MAIN_OBJECT,
        /// Sent the server a request for an embedded object and waiting to receive the packets.
        CLIENT_EXPECTING_EMBEDDED_OBJECT,
        /// User reading a web page that has just been received.
        CLIENT_READING,
        /// After StopApplication() is invoked.
        CLIENT_STOPPED
    };

    /**
     * Returns the current state of the application.
     * \return The current state of the application.
     */
    StateServer_t GetServerState() const;
    StateClient_t GetClientState() const;

    /**
     * Returns the current state of the application in string format.
     * \return The current state of the application in string format.
     */
    std::string GetStateServerString() const;
    std::string GetStateClientString() const;

    /**
     * Returns the given state in string format.
     * \param state An arbitrary state of an application.
     * \return The given state equivalently expressed in string format.
     */
    static std::string GetStateServerString(StateServer_t state);
    static std::string GetStateClientString(StateClient_t state);

    /**
     * Common callback signature for `MainObject` and `EmbeddedObject` trace
     * sources.
     * \param size Size of the generated object in bytes.
     */
    typedef void (*ThreeGppHttpObjectCallback)(uint32_t size);

    /**
     * Callback signature for `ConnectionEstablished` trace source.
     * \param httpServer Pointer to this instance of VideoStreamPCDN, which is where
     *                   the trace originated.
     * \param socket Pointer to the socket where the connection is established.
     */
    typedef void (*ConnectionEstablishedCallback)(Ptr<const VideoStreamPCDN> httpServer,
                                                  Ptr<Socket> socket);

  protected:
    // Inherited from Object base class
    void DoDispose() override;

    // Inherited from Application base class
    void StartApplication() override;
    void StopApplication() override;

  private:
    // SOCKET CALLBACK METHODS

    /**
     * Invoked when #m_initialSocket receives a connection request.
     * \param socket Pointer to the socket where the event originates from.
     * \param address The address of the remote client where the connection
     *                request comes from.
     * \return Always true, to indicate to the other end that the connection
     *         request is accepted.
     */
    bool ConnectionRequestCallback(Ptr<Socket> socket, const Address& address);
    /**
     * Invoked when a new connection has been established.
     * \param socket Pointer to the socket that maintains the connection to the
     *               remote client. This socket will be saved to the Tx buffer.
     * \param address The address the connection is incoming from.
     */
    void NewConnectionCreatedCallback(Ptr<Socket> socket, const Address& address);
    /**
     * Invoked when a connection with a web client is terminated. The
     * corresponding socket will be removed from Tx buffer.
     * \param socket Pointer to the socket where the event originates from.
     */
    void NormalCloseCallback(Ptr<Socket> socket);
    /**
     * Invoked when a connection with a web client is terminated. The
     * corresponding socket will be removed from Tx buffer.
     * \param socket Pointer to the socket where the event originates from.
     */
    void ErrorCloseCallback(Ptr<Socket> socket);
    /**
     * Invoked when #m_initialSocket receives some packet data. It will check the
     * packet for ThreeGppHttpHeader. It also fires the `Rx` trace source.
     *
     * Depending on the type of object requested, the method will trigger
     * ServeMainObject() or ServeEmbeddedObject() after some delays.
     *
     * \param socket Pointer to the socket where the event originates from.
     */
    void ReceivedDataCallback(Ptr<Socket> socket);
    /**
     * Invoked when more buffer space for transmission is added to a socket. The
     * method will invoke ServeFromTxBuffer() to start some transmission using
     * the socket.
     * \param socket Pointer to the socket where the event originates from.
     * \param availableBufferSize The number of bytes available in the socket's
     *                            transmission buffer.
     */
    void SendCallback(Ptr<Socket> socket, uint32_t availableBufferSize);

    // TX-RELATED METHODS

    /**
     * Generates a new main object and push it into the Tx buffer.
     *
     * The size of the object is randomly determined by ThreeGppHttpVariables.
     * Fires the `MainObject` trace source. It then immediately triggers
     * ServeFromTxBuffer() to send the object.
     *
     * \param socket Pointer to the socket which is associated with the
     *               destination client.
     */
    void ServeNewMainObject(Ptr<Socket> socket, uint32_t videonumber);
    /**
     * Generates a new embedded object and push it into the Tx buffer.
     *
     * The size of the object is randomly determined by ThreeGppHttpVariables.
     * Fires the `EmbeddedObject` trace source. It then immediately triggers
     * ServeFromTxBuffer() to send the object.
     *
     * \param socket Pointer to the socket which is associated with the
     *               destination client.
     */
    void ServeNewEmbeddedObject(Ptr<Socket> socket);
    /**
     * Creates a packet out of a pending object in the Tx buffer send it over the
     * given socket. If the socket capacity is smaller than the object size, then
     * the method only convert a part of the object into a packet.
     *
     * ThreeGppHttpHeader will be attached in the beginning of each application
     * layer packet - if a packet is split, then then the following parts will
     * not have the header. The method fires the `Tx` trace source after sending
     * the packet to the socket.
     *
     * This method is invoked when a new object is generated by
     * ServeNewMainObject() or ServeNewEmbeddedObject(). It's also invoked when
     * the socket informs (through SendCallback()) that more buffer space for
     * transmission has become available.
     *
     * \param socket Pointer to the socket which is associated with the
     *               destination client.
     * \return Size of the packet sent (in bytes).
     */
    uint32_t ServeFromTxBuffer(Ptr<Socket> socket, uint32_t videonumber);
    uint32_t ServeFromTxBuffer(Ptr<Socket> socket);

    /**
     * Change the state of the server. Fires the `StateTransition` trace source.
     * \param state The new state.
     */
    void SwitchToStateServer(StateServer_t state);
    void SwitchToStateClient(StateClient_t state);

    // CLIENT SOCKET CALLBACK METHODS

    /**
     * Invoked when a connection is established successfully on #m_socket. This
     * triggers a request for a main object.
     * \param socket Pointer to the socket where the event originates from.
     */
    void ClientConnectionSucceededCallback(Ptr<Socket> socket);
    /**
     * Invoked when #m_socket cannot establish a connection with the web server.
     * Simulation will stop and error will be raised.
     * \param socket Pointer to the socket where the event originates from.
     */
    void ClientConnectionFailedCallback(Ptr<Socket> socket);
    /**
     * Invoked when connection between #m_socket and the web sever is terminated.
     * Error will be logged, but simulation continues.
     * \param socket Pointer to the socket where the event originates from.
     */
    void ClientNormalCloseCallback(Ptr<Socket> socket);
    /**
     * Invoked when connection between #m_socket and the web sever is terminated.
     * Error will be logged, but simulation continues.
     * \param socket Pointer to the socket where the event originates from.
     */
    void ClientErrorCloseCallback(Ptr<Socket> socket);
    /**
     * Invoked when #m_socket receives some packet data. Fires the `Rx` trace
     * source and triggers ReceiveMainObject() or ReceiveEmbeddedObject().
     * \param socket Pointer to the socket where the event originates from.
     */
    void ClientReceivedDataCallback(Ptr<Socket> socket);

    // CONNECTION-RELATED METHOD

    /**
     * Initialize #m_socket to connect to the destination web server at
     * #m_remoteServerAddress and #m_remoteServerPort and set up callbacks to
     * listen to its event. Invoked upon the start of the application.
     */
    void ClientOpenConnection();

    // TX-RELATED METHODS

    /**
     * Send a request object for a main object to the destination web server.
     * The size of the request packet is randomly determined by HttpVariables and
     * is assumed to be smaller than 536 bytes. Fires the `TxMainObjectRequest`
     * trace source.
     *
     * The method is invoked after a connection is established or after a
     * reading time has elapsed.
     */
    void ClientRequestMainObject(Ptr<Socket> socket, uint32_t videoNumber);
    /**
     * Send a request object for an embedded object to the destination web
     * server. The size of the request packet is randomly determined by
     * ThreeGppHttpVariables and is assumed to be smaller than 536 bytes. Fires the
     * `TxEmbeddedObjectRequest` trace source.
     */
    // void ClientRequestEmbeddedObject();

    // RX-RELATED METHODS

    /**
     * Receive a packet of main object from the destination web server. Fires the
     * `RxMainObjectPacket` trace source.
     *
     * A main object may come from more than one packets. This is determined by
     * comparing the content length specified in the ThreeGppHttpHeader of the packet and
     * the actual packet size. #m_objectBytesToBeReceived keeps track of the
     * number of bytes that has been received.
     *
     * If the received packet is not the last packet of the object, then the
     * method simply quits, expecting it to be invoked again when the next packet
     * comes.
     *
     * If the received packet is the last packet of the object, then the method
     * fires the `RxMainObject`, `RxDelay`, and `RxRtt` trace sources. The client
     * then triggers EnterParsingTime().
     *
     * \param packet The received packet.
     * \param from Address of the sender.
     */
    void ClientReceiveMainObject(Ptr<Packet> packet, const Address& from, Ptr<Socket> socket);
    /**
     * Receive a packet of embedded object from the destination web server. Fires
     * the `RxEmbeddedObjectPacket` trace source.
     *
     * An embedded object may come from more than one packets. This is determined
     * by comparing the content length specified in the TheeGppHttpHeader of the packet and
     * the actual packet size. #m_objectBytesToBeReceived keeps track of the
     * number of bytes that has been received.
     *
     * If the received packet is not the last packet of the object, then the
     * method simply quits, expecting it to be invoked again when the next packet
     * comes.
     *
     * If the received packet is the last packet of the object, then the method
     * fires the `RxEmbeddedObject`, `RxDelay`, and `RxRtt` trace sources.
     * Depending on the number of embedded objects remaining
     * (#m_embeddedObjectsToBeRequested) the client can either trigger
     * RequestEmbeddedObject() or EnterReadingTime().
     *
     * \param packet The received packet.
     * \param from Address of the sender.
     */
    // void ClientReceiveEmbeddedObject(Ptr<Packet> packet, const Address& from);
    /**
     * Simulate a consumption of the received packet by subtracting the packet
     * size from the internal counter at #m_objectBytesToBeReceived. Also updates
     * #m_objectClientTs and #m_objectServerTs according to the ThreeGppHttpHeader
     * found in the packet.
     *
     * This method is invoked as a sub-procedure of ReceiveMainObject() and
     * ReceiveEmbeddedObject().
     *
     * \param packet The received packet. If it is the first packet of the object,
     *               then it must have a ThreeGppHttpHeader attached to it.
     */
    void ClientReceive(Ptr<Socket> socket, Ptr<Packet> packet);

    // OFF-TIME-RELATED METHODS

    /**
     * Becomes idle for a randomly determined amount of time, and then triggers
     * ParseMainObject(). The length of idle time is determined by
     * TheeGppHttpVariables.
     *
     * The method is invoked after a complete main object has been received.
     */
    void ClientEnterParsingTime(Ptr<Socket> socket);
    /**
     * Randomly determines the number of embedded objects in the main object.
     * ThreeGppHttpVariables is utilized for this purpose. Then proceeds with
     * RequestEmbeddedObject(). If the number of embedded objects has been
     * determined as zero, then EnterReadingTime() is triggered.
     *
     * The method is invoked after parsing time has elapsed.
     */
    void ClientParseMainObject(Ptr<Socket> socket);
    /**
     * Becomes idle for a randomly determined amount of time, and then triggers
     * RequestMainObject(). The length of idle time is determined by
     * ThreeGppHttpVariables.
     *
     * The method is invoked after a complete web page has been received.
     */
    // void ClientEnterReadingTime();
    /**
     * Cancels #m_eventRequestMainObject, #m_eventRequestEmbeddedObject, and
     * #m_eventParseMainObject. Invoked by StopApplication() and when connection
     * has been terminated.
     */
    void ClientCancelAllPendingEvents(Ptr<Socket> socket);
    Ptr<Socket> P2POpenConnection(Address P2PNode);

    /// The current state of the client application. Begins with NOT_STARTED.
    StateServer_t m_state;

    StateClient_t m_clientState;
    /// The listening socket, for receiving connection requests from clients.
    Ptr<Socket> m_initialSocket;

    Ptr<Socket> m_clientSocket;
    Ptr<Socket> m_P2PSocket;
    /// Pointer to the transmission buffer.
    Ptr<VideoStreamPCDNTxBuffer> m_txBuffer;

    struct PCDN_P2P_INFOS
    {
        uint32_t m_objectBytesToBeReceived;
        /// The packet constructed of one or more parts with ThreeGppHttpHeader.
        Ptr<Packet> m_constructedPacket;
        /// The client time stamp of the ThreeGppHttpHeader from the last received packet.
        Time m_objectClientTs;
        /// The server time stamp of the ThreeGppHttpHeader from the last received packet.
        Time m_objectServerTs;
        /// Determined after parsing the main object.
        uint32_t m_embeddedObjectsToBeRequested;
        /// The time stamp when the page started loading.
        Time m_pageLoadStartTs;
        /// Number of embedded objects to requested in the current page.
        uint32_t m_numberEmbeddedObjectsRequested;
        /// Number of bytes received for the current page.
        uint32_t m_numberBytesPage;
        EventId m_eventRequestMainObject;
        EventId m_eventParseMainObject;
        EventId m_eventPopCachedChunk;

        PCDN_P2P_INFOS()
        {
            m_objectBytesToBeReceived = 0;
            m_constructedPacket = nullptr;
            m_objectClientTs = Seconds(0);
            m_objectServerTs = Seconds(0);
            m_embeddedObjectsToBeRequested = 0;
            m_pageLoadStartTs = Seconds(0);
            m_numberEmbeddedObjectsRequested = 0;
            m_numberBytesPage = 0;
        }
    };

    std::map<Ptr<Socket>, PCDN_P2P_INFOS> m_P2PSocketInfo;

    const int m_maxVideoCacheNumber = 5;

    const int m_maxChunkCacheNumber = 5;
    std::set<uint32_t> m_cachedVideoNumber;
    //<videoNumber, [<chunkNumber, chunkSize>, <>, ....]>
    std::map<uint32_t, std::queue<std::pair<uint32_t, uint32_t>>> m_cacheVideoBufferSize;

    std::map<uint32_t, std::set<Ptr<Socket>>> m_forwardTable;
    std::map<uint32_t, Ptr<Socket>> m_peerSocket;
    std::set<Ptr<Socket>> m_peerSocketSet;
    // ATTRIBUTES

    /// The `Variables` attribute.
    Ptr<ThreeGppHttpVariables> m_httpVariables;
    /// The `LocalAddress` attribute.
    Address m_localAddress;
    Address m_P2PAddress;
    Address m_remoteServerAddress;

    uint16_t m_remoteServerPort;
    /// The `LocalPort` attribute.
    uint16_t m_localPort;
    uint16_t m_P2PPort;
    /// The `Mtu` attribute.
    uint32_t m_mtuSize;

    uint32_t m_bitrateType;
    uint32_t m_videoRequested;
    uint32_t m_VideoChunkRequested;
    uint32_t m_bitrateSet[5];            // = {350, 600, 1000, 2000, 3000};
    uint32_t m_videoChunkTotalNumber[2]; // = {5, 10};
    // TRACE SOURCES

    /// The `ConnectionEstablished` trace source.
    TracedCallback<Ptr<const VideoStreamPCDN>, Ptr<Socket>> m_connectionEstablishedTrace;
    /// The `MainObject` trace source.
    TracedCallback<uint32_t> m_mainObjectTrace;
    /// The `EmbeddedObject` trace source.
    TracedCallback<uint32_t> m_embeddedObjectTrace;
    /// The `Tx` trace source.
    TracedCallback<Ptr<const Packet>> m_txTrace;
    /// The `Rx` trace source.
    TracedCallback<Ptr<const Packet>, const Address&> m_rxTrace;
    /// The `RxDelay` trace source.
    TracedCallback<const Time&, const Address&> m_rxDelayTrace;
    /// The `StateTransition` trace source.
    TracedCallback<const std::string&, const std::string&> m_stateTransitionTrace;

}; // end of `class VideoStreamPCDN`

/**
 * \internal
 * \ingroup http
 * Transmission buffer used by an HTTP server instance.
 *
 * The class handles the sockets which face the connected HTTP clients. An
 * individual buffer is allocated for each socket. The buffer indicates the
 * length (in bytes) and the type of the data within, i.e., it does *not*
 * contain the actual packet data.
 *
 * Types of data are expressed using the ThreeGppHttpHeader::ContentType_t type. Only one
 * type of data can be active for one client at a time, i.e., the current
 * content of a buffer has to be removed before a different type of data can
 * be added.
 */
class VideoStreamPCDNTxBuffer : public SimpleRefCount<VideoStreamPCDNTxBuffer>
{
  public:
    /// Create an empty instance of transmission buffer.
    VideoStreamPCDNTxBuffer();

    // SOCKET MANAGEMENT

    /**
     * This method is typically used before calling other methods. For example,
     * AddSocket() requires that the given socket does not exist among the stored
     * buffers. On the other hand, all the other methods that accept a pointer to
     * a socket as an argument require the existence of a buffer allocated to the
     * given socket.
     * \param socket Pointer to the socket to be found.
     * \return True if the given socket is found within the buffer.
     */
    bool IsSocketAvailable(Ptr<Socket> socket) const;

    /**
     * Add a new socket and create an empty transmission buffer for it. After the
     * method is completed, IsSocketAvailable() for the same pointer of socket
     * shall return true.
     * \param socket Pointer to the new socket to be added (must not exist in the
     *               buffer).
     * \warning Must be called only when IsSocketAvailable() for the given socket
     *          is false.
     */
    void AddSocket(Ptr<Socket> socket);

    /**
     * Remove a socket and its associated transmission buffer, and then unset the
     * socket's callbacks to prevent further interaction with the socket. If the
     * socket has a pending transmission event, it will be canceled.
     *
     * This method is useful for discarding a socket which is already closed,
     * e.g., by the HTTP client. This is due to the fact that double closing of a
     * socket may introduce undefined behaviour.
     *
     * After the method is completed, IsSocketAvailable() for the same pointer of
     * socket shall return false.
     *
     * \param socket Pointer to the socket to be removed.
     * \warning Must be called only when IsSocketAvailable() for the given socket
     *          is true.
     */
    void RemoveSocket(Ptr<Socket> socket);

    /**
     * Close and remove a socket and its associated transmission buffer, and then
     * unset the socket's callback to prevent further interaction with the
     * socket.
     *
     * This method is similar with RemoveSocket(), except that the latter does
     * not close the socket.
     *
     * After the method is completed, IsSocketAvailable() for the same pointer of
     * socket shall return false.
     *
     * \param socket Pointer to the socket to be closed and removed.
     * \warning Must be called only when IsSocketAvailable() for the given socket
     *          is true.
     */
    void CloseSocket(Ptr<Socket> socket);

    /**
     * Close and remove all stored sockets, hence clearing the buffer.
     */
    void CloseAllSockets();

    // BUFFER MANAGEMENT

    /**
     * \param socket Pointer to the socket which is associated with the
     *               transmission buffer of interest.
     * \return True if the current length of the transmission buffer is zero,
     *         i.e., no pending packet.
     * \warning Must be called only when IsSocketAvailable() for the given socket
     *          is true.
     */
    bool IsBufferEmpty(Ptr<Socket> socket) const;

    /**
     * \param socket Pointer to the socket which is associated with the
     *               transmission buffer of interest
     * \return The client time stamp that comes from the last request packet
     *         received by the given socket. It indicates the time the request
     *         packet was transmitted by the client.
     */
    Time GetClientTs(Ptr<Socket> socket) const;

    /**
     * Returns ThreeGppHttpHeader::NOT_SET when the buffer is new and never been filled
     * with any data before. Otherwise, returns either ThreeGppHttpHeader::MAIN_OBJECT
     * or ThreeGppHttpHeader::EMBEDDED_OBJECT.
     * \param socket Pointer to the socket which is associated with the
     *               transmission buffer of interest
     * \return The content type of the current data inside the transmission
     *         buffer.
     * \warning Must be called only when IsSocketAvailable() for the given socket
     *          is true.
     */
    ThreeGppHttpHeader::ContentType_t GetBufferContentType(Ptr<Socket> socket) const;

    /**
     * \param socket Pointer to the socket which is associated with the
     *               transmission buffer of interest
     * \return The length (in bytes) of the current data inside the transmission
     *         buffer.
     * \warning Must be called only when IsSocketAvailable() for the given socket
     *          is true.
     */
    uint32_t GetBufferSize(Ptr<Socket> socket) const;

    /**
     * \param socket pointer to the socket which is associated with the
     *               transmission buffer of interest
     * \return true if the buffer content has been read since it is written
     *
     * \warning Must be called only when IsSocketAvailable() for the given socket
     *          is true.
     *
     * This method returns true after WriteNewObject() method is called. It
     * becomes false after DepleteBufferSize() method is called.
     */
    bool HasTxedPartOfObject(Ptr<Socket> socket) const;

    /**
     * Writes a data representing a new main object or embedded object to the
     * transmission buffer.
     *
     * The stored data can be later consumed wholly of partially by
     * DepleteBufferSize() method.
     *
     * \param socket Pointer to the socket which is associated with the
     *               transmission buffer of interest.
     * \param contentType The content-type of the data to be written (must not
     *                    equal to ThreeGppHttpHeader:NOT_SET).
     * \param objectSize The length (in bytes) of the new object to be created
     *                   (must be greater than zero).
     * \warning Must be called only when both IsSocketAvailable() and
     *          IsBufferEmpty() for the given socket are true.
     */
    void WriteNewObject(Ptr<Socket> socket,
                        ThreeGppHttpHeader::ContentType_t contentType,
                        uint32_t objectSize);

    /**
     * Informs about a pending transmission event associated with the socket, so
     * that it would be automatically canceled in case the socket is closed.
     *
     * The method also indicates the time stamp given by the client. The time
     * stamp will be included in every packet sent.
     *
     * \param socket pointer to the socket which is associated with the
     *               transmission buffer of interest
     * \param eventId the event to be recorded, e.g., the return value of
     *                Simulator::Schedule function
     * \param clientTs client time stamp
     *
     * \warning Must be called only when IsSocketAvailable() for the given socket
     *          is true.
     */
    void RecordNextServe(Ptr<Socket> socket, const EventId& eventId, const Time& clientTs);

    /**
     * Decrements a buffer size by a given amount.
     *
     * The content type of the object to be consumed can be inquired beforehand
     * by the GetBufferContentType() method.
     *
     * If the method has consumed all the remaining bytes within the buffer,
     * IsBufferEmpty() for the buffer shall return true.
     *
     * \param socket Pointer to the socket which is associated with the
     *               transmission buffer of interest.
     * \param amount The length (in bytes) to be consumed (must be greater than
     *               zero).
     *
     * \warning Must be called only when IsSocketAvailable() for the given socket
     *          is true. In addition, the requested amount must be larger than
     *          the current buffer size, which can be checked by calling the
     *          GetBufferSize() method.
     */
    void DepleteBufferSize(Ptr<Socket> socket, uint32_t amount);

    /**
     * Tell the buffer to close the associated socket once the buffer becomes
     * empty.
     * \param socket Pointer to the socket which is associated with the
     *               transmission buffer of interest.
     * \warning Must be called only when IsSocketAvailable() for the given socket
     *          is true.
     */
    void PrepareClose(Ptr<Socket> socket);

  private:
    /**
     * Set of fields representing a single transmission buffer, which will be
     * associated with a socket.
     */
    struct TxBuffer_t
    {
        /**
         * Pending transmission event which will be automatically canceled when the
         * associated socket is closed.
         */
        EventId nextServe;
        /**
         * The client time stamp that comes from the request packet. This value
         * will be set in ThreeGppHttpHeader of every corresponding response packet sent, to
         * be used by the client to compute round trip delay time (RTT).
         */
        Time clientTs;
        /**
         * The content type of the current data inside the transmission buffer.
         * Accessible using the GetBufferContentType() method.
         */
        ThreeGppHttpHeader::ContentType_t txBufferContentType;
        /**
         * The length (in bytes) of the current data inside the transmission
         * buffer. Accessible using the GetBufferSize() method.
         */
        uint32_t txBufferSize;
        /**
         * True if the remote end has issued a request to close, which means that
         * this socket will immediately closes itself once the buffer becomes
         * empty.
         */
        bool isClosing;
        /**
         * \brief True if the buffer content has been read since it is written.
         *        Accessible using the HasTxedPartOfObject() method.
         */
        bool hasTxedPartOfObject;
    };

    /// Collection of accepted sockets and its individual transmission buffer.
    std::map<Ptr<Socket>, TxBuffer_t> m_txBuffer;

}; // end of `class VideoStreamPCDNTxBuffer`

} // namespace ns3
#endif