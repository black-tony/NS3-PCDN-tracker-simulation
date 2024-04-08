/**
 * Author: Yinghao Yang <blacktonyrl@gmail.com>
 * 
*/
#ifndef VIDEO_STREAM_CLIENT_H
#define VIDEO_STREAM_CLIENT_H
#include "three-gpp-http-header.h"
#include "VideoStreamHeader.h"
#include <ns3/address.h>
#include <ns3/application.h>
#include <ns3/traced-callback.h>
#include "ns3/time-probe.h"

#include <queue>
#include <vector>
#include "three-gpp-http-variables.h"
namespace ns3
{

class Socket;
class Packet;
class ThreeGppHttpVariables;

class VideoStreamClient  : public Application
{
  public:
    /**
     * Creates a new instance of HTTP client application.
     *
     * After creation, the application must be further configured through
     * attributes. To avoid having to do this process manually, please use
     * ThreeGppHttpClientHelper.
     */
    VideoStreamClient ();

    /**
     * Returns the object TypeId.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Returns a pointer to the associated socket.
     * \return Pointer to the associated socket.
     */
    Ptr<Socket> GetSocket() const;

    /// The possible states of the application.
    enum State_t
    {
        /// Before StartApplication() is invoked.
        NOT_STARTED = 0,
        /// Sent the server a connection request and waiting for the server to be accept it.
        CONNECTING,
        /// Sent the server a request for a main object and waiting to receive the packets.
        EXPECTING_MAIN_OBJECT,
        /// Parsing a main object that has just been received.
        PARSING_MAIN_OBJECT,
        /// Sent the server a request for an embedded object and waiting to receive the packets.
        EXPECTING_EMBEDDED_OBJECT,
        /// User reading a web page that has just been received.
        READING,
        /// After StopApplication() is invoked.
        STOPPED
    };
    /**
     * Returns the current state of the application.
     * \return The current state of the application.
     */
    State_t GetState() const;

    /**
     * Returns the current state of the application in string format.
     * \return The current state of the application in string format.
     */
    std::string GetStateString() const;

    /**
     * Returns the given state in string format.
     * \param state An arbitrary state of an application.
     * \return The given state equivalently expressed in string format.
     */
    static std::string GetStateString(State_t state);

    /**
     * Common callback signature for `ConnectionEstablished`, `RxMainObject`, and
     * `RxEmbeddedObject` trace sources.
     * \param httpClient Pointer to this instance of ThreeGppHttpClient,
     *                               which is where the trace originated.
     */
    typedef void (*TracedCallback)(Ptr<const VideoStreamClient> httpClient);

    /**
     * Callback signature for `RxPage` trace sources.
     * \param httpClient Pointer to this instance of ThreeGppHttpClient,
     *                               which is where the trace originated.
     * \param time Elapsed time from the start to the end of the request.
     * \param numObjects Number of objects downloaded, including main and
     *                                                  embedded objects.
     * \param numBytes Total number of bytes included in the page.
     */
    typedef void (*RxPageTracedCallback)(Ptr<const VideoStreamClient> httpClient,
                                         const Time& time,
                                         uint32_t numObjects,
                                         uint32_t numBytes);
    //TODO EXTEND dataset
    void SetVideoInfos(uint32_t VideoRequested);
  protected:
    // Inherited from Object base class.
    void DoDispose() override;

    // Inherited from Application base class.
    void StartApplication() override;
    void StopApplication() override;

  private:
    // SOCKET CALLBACK METHODS

    /**
     * Invoked when a connection is established successfully on #m_socket. This
     * triggers a request for a main object.
     * \param socket Pointer to the socket where the event originates from.
     */
    void ConnectionSucceededCallback(Ptr<Socket> socket);
    /**
     * Invoked when #m_socket cannot establish a connection with the web server.
     * Simulation will stop and error will be raised.
     * \param socket Pointer to the socket where the event originates from.
     */
    void ConnectionFailedCallback(Ptr<Socket> socket);
    /**
     * Invoked when connection between #m_socket and the web sever is terminated.
     * Error will be logged, but simulation continues.
     * \param socket Pointer to the socket where the event originates from.
     */
    void NormalCloseCallback(Ptr<Socket> socket);
    /**
     * Invoked when connection between #m_socket and the web sever is terminated.
     * Error will be logged, but simulation continues.
     * \param socket Pointer to the socket where the event originates from.
     */
    void ErrorCloseCallback(Ptr<Socket> socket);
    /**
     * Invoked when #m_socket receives some packet data. Fires the `Rx` trace
     * source and triggers ReceiveMainObject() or ReceiveEmbeddedObject().
     * \param socket Pointer to the socket where the event originates from.
     */
    void ReceivedDataCallback(Ptr<Socket> socket);

    // CONNECTION-RELATED METHOD

    /**
     * Initialize #m_socket to connect to the destination web server at
     * #m_remoteServerAddress and #m_remoteServerPort and set up callbacks to
     * listen to its event. Invoked upon the start of the application.
     */
    void OpenConnection();

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
    void RequestMainObject();
    /**
     * Send a request object for an embedded object to the destination web
     * server. The size of the request packet is randomly determined by
     * ThreeGppHttpVariables and is assumed to be smaller than 536 bytes. Fires the
     * `TxEmbeddedObjectRequest` trace source.
     */
    void RequestEmbeddedObject();

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
    void ReceiveMainObject(Ptr<Packet> packet, const Address& from);
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
    void ReceiveEmbeddedObject(Ptr<Packet> packet, const Address& from);
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
    void Receive(Ptr<Packet> packet);

    // OFF-TIME-RELATED METHODS

    /**
     * Becomes idle for a randomly determined amount of time, and then triggers
     * ParseMainObject(). The length of idle time is determined by
     * TheeGppHttpVariables.
     *
     * The method is invoked after a complete main object has been received.
     */
    void EnterParsingTime();
    /**
     * Randomly determines the number of embedded objects in the main object.
     * ThreeGppHttpVariables is utilized for this purpose. Then proceeds with
     * RequestEmbeddedObject(). If the number of embedded objects has been
     * determined as zero, then EnterReadingTime() is triggered.
     *
     * The method is invoked after parsing time has elapsed.
     */
    void ParseMainObject();
    /**
     * Becomes idle for a randomly determined amount of time, and then triggers
     * RequestMainObject(). The length of idle time is determined by
     * ThreeGppHttpVariables.
     *
     * The method is invoked after a complete web page has been received.
     */
    void EnterReadingTime();
    /**
     * Cancels #m_eventRequestMainObject, #m_eventRequestEmbeddedObject, and
     * #m_eventParseMainObject. Invoked by StopApplication() and when connection
     * has been terminated.
     */
    void CancelAllPendingEvents();

    /**
     * Change the state of the client. Fires the `StateTransition` trace source.
     * \param state The new state.
     */
    void SwitchToState(State_t state);

    /**
     * Finish receiving a page. This function should be called when a full-page
     * finishes loading, including the main object and all embedded objects.
     */
    void FinishReceivingPage();

    void DisplayVideoChunk();
    /// The current state of the client application. Begins with NOT_STARTED.
    State_t m_state;
    /// The socket for sending and receiving packets to/from the web server.
    Ptr<Socket> m_socket;
    /// According to the content length specified by the ThreeGppHttpHeader.
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

    uint32_t m_chunknumber = 0;
    uint32_t m_chunkTimeStart;
    uint32_t m_chunkTimeEnd;
    uint32_t m_throughputForChunk;
    uint32_t m_chunkSize;
    // uint32_t m_throughputList[2000];
    int m_throughputCount = 0;
    const uint32_t m_bitrateSet[5] = {350, 600, 1000, 2000, 3000};// TODO: dynamic
    uint32_t m_nextBitrateType;
    uint32_t m_randomVideoNumber;
    uint32_t m_VideoRequested;
    const uint32_t m_videoRandomList[3] = {0, 70, 100};
    const uint32_t m_bufferLen = 5;//TODO dynamic bufferLen
    std::queue<uint32_t> m_bufferQueue;
    std::vector<uint32_t> m_throughputList;
    uint32_t m_stopCounter;
    uint32_t m_videoChunkRequested;
    uint32_t m_rebufferTotalTime = 0; // 重缓冲总时间
    uint32_t m_rebufferTotalCount = 0; // 重缓冲总次数
    uint32_t m_buffer = 0;  // 当前缓冲区状态
    uint32_t m_transmissionTime = 0; // 一个块的传输时间

    // // Seconds to wait before displaying the content
    // uint16_t m_initialDelay;
    // // Counter to decide if the video streaming finishes
    // uint16_t m_stopCounter;
    // // Counter of the rebuffering event
    // uint16_t m_rebuferCounter;
    // // The quality of the video from the server
    // uint16_t m_videoLevel;
    // // Number of frames per second to be played
    // uint16_t m_frameRate;
    // // Total size of packets from one frame
    // uint16_t m_frameSize;
    // // Last received frame number
    // uint16_t m_lastRecvFrame;
    // // Last size of the buffer
    // uint16_t m_lastBufferSize;
    // // Size of the frame buffer
    // uint16_t m_currentBufferSize;

    // ATTRIBUTES

    /// The `Variables` attribute.
    Ptr<ThreeGppHttpVariables> m_httpVariables;
    /// The `RemoteServerAddress` attribute. The address of the web server.
    Address m_remoteServerAddress;
    /// The `RemoteServerPort` attribute.
    uint16_t m_remoteServerPort;

    // TRACE SOURCES

    /// The `RxPage` trace source.
    ns3::TracedCallback<Ptr<const VideoStreamClient>, const Time&, uint32_t, uint32_t>
        m_rxPageTrace;
    /// The `ConnectionEstablished` trace source.
    ns3::TracedCallback<Ptr<const VideoStreamClient>> m_connectionEstablishedTrace;
    /// The `ConnectionClosed` trace source.
    ns3::TracedCallback<Ptr<const VideoStreamClient>> m_connectionClosedTrace;
    /// The `Tx` trace source.
    ns3::TracedCallback<Ptr<const Packet>> m_txTrace;
    /// The `TxMainObjectRequest` trace source.
    ns3::TracedCallback<Ptr<const Packet>> m_txMainObjectRequestTrace;
    /// The `TxEmbeddedObjectRequest` trace source.
    ns3::TracedCallback<Ptr<const Packet>> m_txEmbeddedObjectRequestTrace;
    /// The `TxMainObjectPacket` trace source.
    ns3::TracedCallback<Ptr<const Packet>> m_rxMainObjectPacketTrace;
    /// The `TxMainObject` trace source.
    ns3::TracedCallback<Ptr<const VideoStreamClient>, Ptr<const Packet>> m_rxMainObjectTrace;
    /// The `TxEmbeddedObjectPacket` trace source.
    ns3::TracedCallback<Ptr<const Packet>> m_rxEmbeddedObjectPacketTrace;
    /// The `TxEmbeddedObject` trace source.
    ns3::TracedCallback<Ptr<const VideoStreamClient>, Ptr<const Packet>> m_rxEmbeddedObjectTrace;
    /// The `Rx` trace source.
    ns3::TracedCallback<Ptr<const Packet>, const Address&> m_rxTrace;
    /// The `RxDelay` trace source.
    ns3::TracedCallback<const Time&, const Address&> m_rxDelayTrace;
    /// The `RxRtt` trace source.
    ns3::TracedCallback<const Time&, const Address&> m_rxRttTrace;
    /// The `StateTransition` trace source.
    ns3::TracedCallback<const std::string&, const std::string&> m_stateTransitionTrace;
    ns3::TracedValue<Time> m_firstTime;
    Time m_firstRequestTime;
    Time m_answeredTime;
    uint32_t m_firstRequested;
    // EVENTS

    /**
     * An event of either RequestMainObject() or OpenConnection(), scheduled to
     * trigger after a connection has been established or reading time has
     * elapsed.
     */
    EventId m_eventRequestMainObject;
    /**
     * An event of either RequestEmbeddedObject() or OpenConnection().
     */
    EventId m_eventRequestEmbeddedObject;
    /**
     * An event of ParseMainObject(), scheduled to trigger after parsing time has
     * elapsed.
     */
    EventId m_eventParseMainObject;

    EventId m_eventDisplayVideoChunk;

}; // end of `class VideoStreamClient`

} // namespace ns3

#endif