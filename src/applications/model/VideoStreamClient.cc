/**
 * Author: Yinghao Yang <blacktonyrl@gmail.com>
 * 
*/

#include "VideoStreamClient.h"

#include "three-gpp-http-variables.h"

#include <ns3/callback.h>
#include <ns3/double.h>
#include <ns3/inet-socket-address.h>
#include <ns3/inet6-socket-address.h>
#include <ns3/log.h>
#include <ns3/packet.h>
#include <ns3/pointer.h>
#include <ns3/simulator.h>
#include <ns3/socket.h>
#include <ns3/tcp-socket-factory.h>
#include <ns3/uinteger.h>

NS_LOG_COMPONENT_DEFINE("VideoStreamClient");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(VideoStreamClient);

VideoStreamClient::VideoStreamClient()
    : m_state(NOT_STARTED),
      m_socket(nullptr),
      m_objectBytesToBeReceived(0),
      m_objectClientTs(MilliSeconds(0)),
      m_objectServerTs(MilliSeconds(0)),
      m_embeddedObjectsToBeRequested(0),
      m_pageLoadStartTs(MilliSeconds(0)),
      m_numberEmbeddedObjectsRequested(0),
      m_numberBytesPage(0),
      m_httpVariables(CreateObject<ThreeGppHttpVariables>())
{
    //TODO random choose video number
    m_VideoRequested = 1;
    m_firstRequestTime = Seconds(0);
    m_answeredTime = Seconds(0);
    m_firstTime = Seconds(0);
    m_firstRequested = 0;
    NS_LOG_FUNCTION(this);
}

// static
TypeId
VideoStreamClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::VideoStreamClient")
            .SetParent<Application>()
            .AddConstructor<VideoStreamClient>()
            .AddAttribute(
                "Variables",
                "Variable collection, which is used to control e.g. timing and HTTP request size.",
                PointerValue(),
                MakePointerAccessor(&VideoStreamClient::m_httpVariables),
                MakePointerChecker<ThreeGppHttpVariables>())
            .AddAttribute("RemoteServerAddress",
                          "The address of the destination server.",
                          AddressValue(),
                          MakeAddressAccessor(&VideoStreamClient::m_remoteServerAddress),
                          MakeAddressChecker())
            .AddAttribute("RemoteServerPort",
                          "The destination port of the outbound packets.",
                          UintegerValue(80), // the default HTTP port
                          MakeUintegerAccessor(&VideoStreamClient::m_remoteServerPort),
                          MakeUintegerChecker<uint16_t>())
            .AddTraceSource("RxPage",
                            "A page has been received.",
                            MakeTraceSourceAccessor(&VideoStreamClient::m_rxPageTrace),
                            "ns3::VideoStreamClient::RxPageTracedCallback")
            .AddTraceSource(
                "ConnectionEstablished",
                "Connection to the destination web server has been established.",
                MakeTraceSourceAccessor(&VideoStreamClient::m_connectionEstablishedTrace),
                "ns3::VideoStreamClient::TracedCallback")
            .AddTraceSource("ConnectionClosed",
                            "Connection to the destination web server is closed.",
                            MakeTraceSourceAccessor(&VideoStreamClient::m_connectionClosedTrace),
                            "ns3::VideoStreamClient::TracedCallback")
            .AddTraceSource("Tx",
                            "General trace for sending a packet of any kind.",
                            MakeTraceSourceAccessor(&VideoStreamClient::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource(
                "TxMainObjectRequest",
                "Sent a request for a main object.",
                MakeTraceSourceAccessor(&VideoStreamClient::m_txMainObjectRequestTrace),
                "ns3::Packet::TracedCallback")
            .AddTraceSource(
                "TxEmbeddedObjectRequest",
                "Sent a request for an embedded object.",
                MakeTraceSourceAccessor(&VideoStreamClient::m_txEmbeddedObjectRequestTrace),
                "ns3::Packet::TracedCallback")
            .AddTraceSource("RxMainObjectPacket",
                            "A packet of main object has been received.",
                            MakeTraceSourceAccessor(&VideoStreamClient::m_rxMainObjectPacketTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("RxMainObject",
                            "Received a whole main object. Header is included.",
                            MakeTraceSourceAccessor(&VideoStreamClient::m_rxMainObjectTrace),
                            "ns3::VideoStreamClient::TracedCallback")
            .AddTraceSource(
                "RxEmbeddedObjectPacket",
                "A packet of embedded object has been received.",
                MakeTraceSourceAccessor(&VideoStreamClient::m_rxEmbeddedObjectPacketTrace),
                "ns3::Packet::TracedCallback")
            .AddTraceSource("RxEmbeddedObject",
                            "Received a whole embedded object. Header is included.",
                            MakeTraceSourceAccessor(&VideoStreamClient::m_rxEmbeddedObjectTrace),
                            "ns3::VideoStreamClient::TracedCallback")
            .AddTraceSource("Rx",
                            "General trace for receiving a packet of any kind.",
                            MakeTraceSourceAccessor(&VideoStreamClient::m_rxTrace),
                            "ns3::Packet::PacketAddressTracedCallback")
            .AddTraceSource("RxDelay",
                            "General trace of delay for receiving a complete object.",
                            MakeTraceSourceAccessor(&VideoStreamClient::m_rxDelayTrace),
                            "ns3::Application::DelayAddressCallback")
            .AddTraceSource(
                "RxRtt",
                "General trace of round trip delay time for receiving a complete object.",
                MakeTraceSourceAccessor(&VideoStreamClient::m_rxRttTrace),
                "ns3::Application::DelayAddressCallback")
            .AddTraceSource("StateTransition",
                            "Trace fired upon every HTTP client state transition.",
                            MakeTraceSourceAccessor(&VideoStreamClient::m_stateTransitionTrace),
                            "ns3::Application::StateTransitionCallback")
            .AddTraceSource("FirstTime",
                            "Trace source FirstTime",
                            MakeTraceSourceAccessor(&VideoStreamClient::m_firstTime),
                            "ns3::TracedValueCallback::Time");
    return tid;
}

Ptr<Socket>
VideoStreamClient::GetSocket() const
{
    return m_socket;
}

VideoStreamClient::State_t
VideoStreamClient::GetState() const
{
    return m_state;
}

std::string
VideoStreamClient::GetStateString() const
{
    return GetStateString(m_state);
}

// static
std::string
VideoStreamClient::GetStateString(VideoStreamClient::State_t state)
{
    switch (state)
    {
    case NOT_STARTED:
        return "NOT_STARTED";
    case CONNECTING:
        return "CONNECTING";
    case EXPECTING_MAIN_OBJECT:
        return "EXPECTING_MAIN_OBJECT";
    case PARSING_MAIN_OBJECT:
        return "PARSING_MAIN_OBJECT";
    case EXPECTING_EMBEDDED_OBJECT:
        return "EXPECTING_EMBEDDED_OBJECT";
    case READING:
        return "READING";
    case STOPPED:
        return "STOPPED";
    default:
        NS_FATAL_ERROR("Unknown state");
        return "FATAL_ERROR";
    }
}

void
VideoStreamClient::DoDispose()
{
    NS_LOG_FUNCTION(this);

    if (!Simulator::IsFinished())
    {
        StopApplication();
    }

    Application::DoDispose(); // Chain up.
}

void
VideoStreamClient::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_state == NOT_STARTED)
    {
        m_httpVariables->Initialize();
        OpenConnection();
    }
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for StartApplication().");
    }
}

void
VideoStreamClient::StopApplication()
{
    NS_LOG_FUNCTION(this);

    SwitchToState(STOPPED);
    CancelAllPendingEvents();
    m_socket->Close();
    m_socket->SetConnectCallback(MakeNullCallback<void, Ptr<Socket>>(),
                                 MakeNullCallback<void, Ptr<Socket>>());
    m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
}

void
VideoStreamClient::ConnectionSucceededCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    if (m_state == CONNECTING)
    {
        NS_ASSERT_MSG(m_socket == socket, "Invalid socket.");
        m_connectionEstablishedTrace(this);
        socket->SetRecvCallback(MakeCallback(&VideoStreamClient::ReceivedDataCallback, this));
        NS_ASSERT(m_embeddedObjectsToBeRequested == 0);
        m_eventRequestMainObject =
            Simulator::ScheduleNow(&VideoStreamClient::RequestMainObject, this);
    }
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ConnectionSucceeded().");
    }
}
void VideoStreamClient::SetVideoInfos(uint32_t VideoRequested)
{
    m_VideoRequested = VideoRequested;
}
void
VideoStreamClient::ConnectionFailedCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    if (m_state == CONNECTING)
    {
        NS_LOG_ERROR("Client failed to connect"
                     << " to remote address " << m_remoteServerAddress << " port "
                     << m_remoteServerPort << ".");
    }
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ConnectionFailed().");
    }
}

void
VideoStreamClient::NormalCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    CancelAllPendingEvents();

    if (socket->GetErrno() != Socket::ERROR_NOTERROR)
    {
        NS_LOG_ERROR(this << " Connection has been terminated,"
                          << " error code: " << socket->GetErrno() << ".");
    }

    m_socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                MakeNullCallback<void, Ptr<Socket>>());

    m_connectionClosedTrace(this);
}

void
VideoStreamClient::ErrorCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    CancelAllPendingEvents();
    if (socket->GetErrno() != Socket::ERROR_NOTERROR)
    {
        NS_LOG_ERROR(this << " Connection has been terminated,"
                          << " error code: " << socket->GetErrno() << ".");
    }

    m_connectionClosedTrace(this);
}

void
VideoStreamClient::ReceivedDataCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() == 0)
        {
            break; // EOF
        }

#ifdef NS3_LOG_ENABLE
        // Some log messages.
        if (InetSocketAddress::IsMatchingType(from))
        {
            NS_LOG_INFO(this << " Client  " << " A packet of " << packet->GetSize() << " bytes"
                             << " received from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
                             << " port " << InetSocketAddress::ConvertFrom(from).GetPort() << " / "
                             << InetSocketAddress::ConvertFrom(from) << ".");
        }
        else if (Inet6SocketAddress::IsMatchingType(from))
        {
            NS_LOG_INFO(this << " Client  " << " A packet of " << packet->GetSize() << " bytes"
                             << " received from " << Inet6SocketAddress::ConvertFrom(from).GetIpv6()
                             << " port " << Inet6SocketAddress::ConvertFrom(from).GetPort() << " / "
                             << Inet6SocketAddress::ConvertFrom(from) << ".");
        }
#endif /* NS3_LOG_ENABLE */

        m_rxTrace(packet, from);

        switch (m_state)
        {
        case EXPECTING_MAIN_OBJECT:
            ReceiveMainObject(packet, from);
            break;
        case EXPECTING_EMBEDDED_OBJECT:
            ReceiveEmbeddedObject(packet, from);
            break;
        default:
            NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ReceivedData().");
            break;
        }

    } // end of `while ((packet = socket->RecvFrom (from)))`

} // end of `void ReceivedDataCallback (Ptr<Socket> socket)`

void
VideoStreamClient::OpenConnection()
{
    NS_LOG_FUNCTION(this);

    if (m_state == NOT_STARTED || m_state == EXPECTING_EMBEDDED_OBJECT ||
        m_state == PARSING_MAIN_OBJECT || m_state == READING)
    {
        m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());

        if (Ipv4Address::IsMatchingType(m_remoteServerAddress))
        {
            int ret [[maybe_unused]];

            ret = m_socket->Bind();
            NS_LOG_DEBUG(this << " Bind() return value= " << ret
                              << " GetErrNo= " << m_socket->GetErrno() << ".");

            Ipv4Address ipv4 = Ipv4Address::ConvertFrom(m_remoteServerAddress);
            InetSocketAddress inetSocket = InetSocketAddress(ipv4, m_remoteServerPort);
            NS_LOG_INFO(this << " Client  " << " Connecting to " << ipv4 << " port " << m_remoteServerPort << " / "
                             << inetSocket << ".");
            ret = m_socket->Connect(inetSocket);
            NS_LOG_DEBUG(this << " Connect() return value= " << ret
                              << " GetErrNo= " << m_socket->GetErrno() << ".");
        }
        else if (Ipv6Address::IsMatchingType(m_remoteServerAddress))
        {
            int ret [[maybe_unused]];

            ret = m_socket->Bind6();
            NS_LOG_DEBUG(this << " Bind6() return value= " << ret
                              << " GetErrNo= " << m_socket->GetErrno() << ".");

            Ipv6Address ipv6 = Ipv6Address::ConvertFrom(m_remoteServerAddress);
            Inet6SocketAddress inet6Socket = Inet6SocketAddress(ipv6, m_remoteServerPort);
            NS_LOG_INFO(this << " Client  " << " connecting to " << ipv6 << " port " << m_remoteServerPort << " / "
                             << inet6Socket << ".");
            ret = m_socket->Connect(inet6Socket);
            NS_LOG_DEBUG(this << " Connect() return value= " << ret
                              << " GetErrNo= " << m_socket->GetErrno() << ".");
        }

        NS_ASSERT_MSG(m_socket, "Failed creating socket.");

        SwitchToState(CONNECTING);

        m_socket->SetConnectCallback(
            MakeCallback(&VideoStreamClient::ConnectionSucceededCallback, this),
            MakeCallback(&VideoStreamClient::ConnectionFailedCallback, this));
        m_socket->SetCloseCallbacks(MakeCallback(&VideoStreamClient::NormalCloseCallback, this),
                                    MakeCallback(&VideoStreamClient::ErrorCloseCallback, this));
        m_socket->SetRecvCallback(MakeCallback(&VideoStreamClient::ReceivedDataCallback, this));
        m_socket->SetAttribute("MaxSegLifetime", DoubleValue(0.02)); // 20 ms.

    } // end of `if (m_state == {NOT_STARTED, EXPECTING_EMBEDDED_OBJECT, PARSING_MAIN_OBJECT,
      // READING})`
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for OpenConnection().");
    }

} // end of `void OpenConnection ()`

void
VideoStreamClient::RequestMainObject()
{
    NS_LOG_FUNCTION(this);
    //请求100块就主动停止
    if(m_firstRequested == 0)
    {
        m_firstRequestTime = Simulator::Now();
        m_firstRequested = 1;
    }
    if (m_chunknumber == 100)
    {
        StopApplication();
    }


    if (m_state == CONNECTING || m_state == READING)
    {
        VideoStreamHeader streamHeader;
        streamHeader.SetBitrateType(m_nextBitrateType);
        streamHeader.SetVideoChunkNumber(m_chunknumber++);
        streamHeader.SetVideoNumber(m_VideoRequested);

        ThreeGppHttpHeader header;
        header.SetContentLength(0); // Request does not need any content length.
        header.SetContentType(ThreeGppHttpHeader::MAIN_OBJECT);
        header.SetClientTs(Simulator::Now());

        const uint32_t requestSize = m_httpVariables->GetRequestSize();
        Ptr<Packet> packet = Create<Packet>(requestSize);
        packet->AddHeader(streamHeader);
        packet->AddHeader(header);

        // add some stream header 
        


        const uint32_t packetSize = packet->GetSize();
        m_txMainObjectRequestTrace(packet);
        m_txTrace(packet);
        const int actualBytes = m_socket->Send(packet);
        NS_LOG_DEBUG(this << " Send() packet " << packet << " of " << packet->GetSize() << " bytes,"
                          << " return value= " << actualBytes << ".");
        if (actualBytes != static_cast<int>(packetSize))
        {
            NS_LOG_ERROR(this << " Failed to send request for embedded object,"
                              << " GetErrNo= " << m_socket->GetErrno() << ","
                              << " waiting for another Tx opportunity.");
        }
        else
        {
            SwitchToState(EXPECTING_MAIN_OBJECT);
            m_pageLoadStartTs = Simulator::Now(); // start counting page loading time
        }
    }
    // else if (m_state == EXPECTING_MAIN_OBJECT)
    // {
    //     m_stopCounter++;

    // }
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for RequestMainObject().");
    }

} // end of `void RequestMainObject ()`


void VideoStreamClient::DisplayVideoChunk()
{
    if(m_bufferQueue.size() > 0)
    {
        m_bufferQueue.pop();
        m_stopCounter = 0;
        NS_LOG_INFO(this << " Client  " << " display one chunk of video");
    }
    else
    {
        m_stopCounter++;
        NS_LOG_INFO(this << " Client  " << " waiting for another chunk of video");

        if(m_stopCounter >= 3)
        {
            NS_LOG_INFO(this << " Client  " << " waiting for chunk of video too long, stop application");

            StopApplication();
            return;
        }
    }
    m_eventDisplayVideoChunk = 
        Simulator::Schedule(Seconds(1), &VideoStreamClient::DisplayVideoChunk, this);
}


void
VideoStreamClient::RequestEmbeddedObject()
{
    NS_LOG_FUNCTION(this);

    if (m_state == CONNECTING || m_state == PARSING_MAIN_OBJECT ||
        m_state == EXPECTING_EMBEDDED_OBJECT)
    {
        if (m_embeddedObjectsToBeRequested > 0)
        {
            ThreeGppHttpHeader header;
            header.SetContentLength(0); // Request does not need any content length.
            header.SetContentType(ThreeGppHttpHeader::EMBEDDED_OBJECT);
            header.SetClientTs(Simulator::Now());

            const uint32_t requestSize = m_httpVariables->GetRequestSize();
            Ptr<Packet> packet = Create<Packet>(requestSize);
            packet->AddHeader(header);
            const uint32_t packetSize = packet->GetSize();
            m_txEmbeddedObjectRequestTrace(packet);
            m_txTrace(packet);
            const int actualBytes = m_socket->Send(packet);
            NS_LOG_DEBUG(this << " Send() packet " << packet << " of " << packet->GetSize()
                              << " bytes,"
                              << " return value= " << actualBytes << ".");

            if (actualBytes != static_cast<int>(packetSize))
            {
                NS_LOG_ERROR(this << " Failed to send request for embedded object,"
                                  << " GetErrNo= " << m_socket->GetErrno() << ","
                                  << " waiting for another Tx opportunity.");
            }
            else
            {
                m_embeddedObjectsToBeRequested--;
                SwitchToState(EXPECTING_EMBEDDED_OBJECT);
            }
        }
        else
        {
            NS_LOG_WARN(this << " No embedded object to be requested.");
        }
    }
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for RequestEmbeddedObject().");
    }

} // end of `void RequestEmbeddedObject ()`

void
VideoStreamClient::ReceiveMainObject(Ptr<Packet> packet, const Address& from)
{
    NS_LOG_FUNCTION(this << packet << from);

    if (m_state == EXPECTING_MAIN_OBJECT)
    {
        /*
         * In the following call to Receive(), #m_objectBytesToBeReceived *will*
         * be updated. #m_objectClientTs and #m_objectServerTs *may* be updated.
         * ThreeGppHttpHeader will be removed from the packet, if it is the first
         * packet of the object to be received; the header will be available in
         * #m_constructedPacketHeader.
         * #m_constructedPacket will also be updated.
         */
        Receive(packet);
        m_rxMainObjectPacketTrace(packet);

        if (m_objectBytesToBeReceived > 0)
        {
            /*
             * There are more packets of this main object, so just stay still
             * and wait until they arrive.
             */
            // NS_LOG_INFO(this << " Client  " << " " << m_objectBytesToBeReceived << " byte(s)"
            //                  << " remains from this chunk of main object.");
        }
        else
        {
            /*
             * This is the last packet of this main object. Acknowledge the
             * reception of a whole main object
             */
            NS_LOG_INFO(this << " Client  " << " Finished receiving a main object.");
            m_rxMainObjectTrace(this, m_constructedPacket);

            if (!m_objectServerTs.IsZero())
            {
                m_rxDelayTrace(Simulator::Now() - m_objectServerTs, from);
                m_objectServerTs = MilliSeconds(0); // Reset back to zero.
            }

            if (!m_objectClientTs.IsZero())
            {
                m_rxRttTrace(Simulator::Now() - m_objectClientTs, from);
                m_objectClientTs = MilliSeconds(0); // Reset back to zero.
            }

            EnterParsingTime();

        } // end of else of `if (m_objectBytesToBeReceived > 0)`

    } // end of `if (m_state == EXPECTING_MAIN_OBJECT)`
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ReceiveMainObject().");
    }

} // end of `void ReceiveMainObject (Ptr<Packet> packet)`

void
VideoStreamClient::ReceiveEmbeddedObject(Ptr<Packet> packet, const Address& from)
{
    NS_LOG_FUNCTION(this << packet << from);

    if (m_state == EXPECTING_EMBEDDED_OBJECT)
    {
        /*
         * In the following call to Receive(), #m_objectBytesToBeReceived *will*
         * be updated. #m_objectClientTs and #m_objectServerTs *may* be updated.
         * ThreeGppHttpHeader will be removed from the packet, if it is the first
         * packet of the object to be received; the header will be available in
         * #m_constructedPacket, which will also be updated.
         */
        Receive(packet);
        m_rxEmbeddedObjectPacketTrace(packet);

        if (m_objectBytesToBeReceived > 0)
        {
            /*
             * There are more packets of this embedded object, so just stay
             * still and wait until they arrive.
             */
            NS_LOG_INFO(this << " Client  " << " " << m_objectBytesToBeReceived << " byte(s)"
                             << " remains from this chunk of embedded object");
        }
        else
        {
            /*
             * This is the last packet of this embedded object. Acknowledge
             * the reception of a whole embedded object
             */
            NS_LOG_INFO(this << " Client  " << " Finished receiving an embedded object.");
            m_rxEmbeddedObjectTrace(this, m_constructedPacket);

            if (!m_objectServerTs.IsZero())
            {
                m_rxDelayTrace(Simulator::Now() - m_objectServerTs, from);
                m_objectServerTs = MilliSeconds(0); // Reset back to zero.
            }

            if (!m_objectClientTs.IsZero())
            {
                m_rxRttTrace(Simulator::Now() - m_objectClientTs, from);
                m_objectClientTs = MilliSeconds(0); // Reset back to zero.
            }

            if (m_embeddedObjectsToBeRequested > 0)
            {
                NS_LOG_INFO(this << " Client  " << " " << m_embeddedObjectsToBeRequested
                                 << " more embedded object(s) to be requested.");
                // Immediately request another using the existing connection.
                m_eventRequestEmbeddedObject =
                    Simulator::ScheduleNow(&VideoStreamClient::RequestEmbeddedObject, this);
            }
            else
            {
                /*
                 * There is no more embedded object, the web page has been
                 * downloaded completely. Now is the time to read it.
                 */
                NS_LOG_INFO(this << " Client  " << " Finished receiving a web page.");
                FinishReceivingPage(); // trigger callback for page loading time
                EnterReadingTime();
            }

        } // end of else of `if (m_objectBytesToBeReceived > 0)`

    } // end of `if (m_state == EXPECTING_EMBEDDED_OBJECT)`
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ReceiveEmbeddedObject().");
    }

} // end of `void ReceiveEmbeddedObject (Ptr<Packet> packet)`

void
VideoStreamClient::Receive(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    /* In a "real" HTTP message the message size is coded differently. The use of a header
     * is to avoid the burden of doing a real message parser.
     */
    bool firstPacket = false;

    if (m_objectBytesToBeReceived == 0)
    {
        // This is the first packet of the object.
        firstPacket = true;

        // Remove the header in order to calculate remaining data to be received.
        ThreeGppHttpHeader httpHeader;
        packet->RemoveHeader(httpHeader);

        m_objectBytesToBeReceived = httpHeader.GetContentLength();
        m_objectClientTs = httpHeader.GetClientTs();
        m_objectServerTs = httpHeader.GetServerTs();

        // Take a copy for constructed packet trace. Note that header is included.
        m_constructedPacket = packet->Copy();
        m_constructedPacket->AddHeader(httpHeader);
    }
    uint32_t contentSize = packet->GetSize();
    m_numberBytesPage += contentSize; // increment counter of page size

    /* Note that the packet does not contain header at this point.
     * The content is purely raw data, which was the only intended data to be received.
     */
    if (m_objectBytesToBeReceived < contentSize)
    {
        NS_LOG_WARN(this << " The received packet"
                         << " (" << contentSize << " bytes of content)"
                         << " is larger than"
                         << " the content that we expected to receive"
                         << " (" << m_objectBytesToBeReceived << " bytes).");
        // Stop expecting any more packet of this object.
        m_objectBytesToBeReceived = 0;
        m_constructedPacket = nullptr;
    }
    else
    {
        m_objectBytesToBeReceived -= contentSize;
        if (!firstPacket)
        {
            Ptr<Packet> packetCopy = packet->Copy();
            m_constructedPacket->AddAtEnd(packetCopy);
        }
    }

} // end of `void Receive (packet)`

void
VideoStreamClient::EnterParsingTime()
{
    NS_LOG_FUNCTION(this);

    if (m_state == EXPECTING_MAIN_OBJECT)
    {
        if(m_firstRequested == 1)
        {
            m_answeredTime = Simulator::Now();
            m_firstRequested = 2;
            m_firstTime = m_answeredTime - m_firstRequestTime;
        }
        // const Time parsingTime = m_httpVariables->GetParsingTime();
        //TODO DYNAMIC parse time
        const Time parsingTime = MilliSeconds(20);
        NS_LOG_INFO(this << " Client  " << " The parsing of this main object"
                         << " will complete in " << parsingTime.As(Time::S) << ".");
        


        m_eventParseMainObject =
            Simulator::Schedule(parsingTime, &VideoStreamClient::ParseMainObject, this);
        SwitchToState(PARSING_MAIN_OBJECT);
    }
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for EnterParsingTime().");
    }
}

void
VideoStreamClient::ParseMainObject()
{
    NS_LOG_FUNCTION(this);

    if (m_state == PARSING_MAIN_OBJECT)
    {
        m_embeddedObjectsToBeRequested = 0;
        // m_embeddedObjectsToBeRequested = m_httpVariables->GetNumOfEmbeddedObjects();
        // saving total number of embedded objects
        m_numberEmbeddedObjectsRequested = m_embeddedObjectsToBeRequested;
        NS_LOG_INFO(this << " Client  " << " Parsing has determined " << m_embeddedObjectsToBeRequested
                         << " embedded object(s) in the main object.");

        if (m_embeddedObjectsToBeRequested > 0)
        {
            /*
             * Immediately request the first embedded object using the
             * existing connection.
             */
            m_eventRequestEmbeddedObject =
                Simulator::ScheduleNow(&VideoStreamClient::RequestEmbeddedObject, this);
        }
        else
        {
            /*
             * There is no embedded object in the main object. So sit back and
             * enjoy the plain web page.
             */
            ThreeGppHttpHeader header;
            VideoStreamHeader vHeader;
            m_constructedPacket->RemoveHeader(header);
            m_constructedPacket->RemoveHeader(vHeader);
            m_chunkSize = vHeader.GetVideoChunkSize();
            m_bufferQueue.push(vHeader.GetBitrateType());
            //至少三秒的buffer后启动
            if(m_chunknumber == 3)
                m_eventDisplayVideoChunk = Simulator::Schedule(Seconds(1.0), &VideoStreamClient::DisplayVideoChunk, this);
            NS_LOG_INFO(this << " Client  " << " recieve video chunk " << m_chunknumber << " with size " << m_chunkSize);
            NS_LOG_INFO(this << " Client  " << " header info is " << vHeader.ToString());
            NS_LOG_INFO(this << " Client  " << " Finished receiving a web page.");
            FinishReceivingPage(); // trigger callback for page loading time
            EnterReadingTime();
        }
    }
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ParseMainObject().");
    }

} // end of `void ParseMainObject ()`

void
VideoStreamClient::EnterReadingTime()
{
    NS_LOG_FUNCTION(this);

    if (m_state == EXPECTING_EMBEDDED_OBJECT || m_state == PARSING_MAIN_OBJECT)
    {
        // const Time readingTime = m_httpVariables->GetReadingTime();
        const Time readingTime = MilliSeconds(980);
        const Time boostTime = Seconds(0);
        Time mReadingTime = (m_chunknumber > 3 ? readingTime : boostTime);
        NS_LOG_INFO(this << " Client  " << " Client will finish reading this web page in "
                         << readingTime.As(Time::S) << ".");

        // Schedule a request of another main object once the reading time expires.
        m_eventRequestMainObject =
            Simulator::Schedule(mReadingTime, &VideoStreamClient::RequestMainObject, this);
        SwitchToState(READING);
    }
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for EnterReadingTime().");
    }
}

void
VideoStreamClient::CancelAllPendingEvents()
{
    NS_LOG_FUNCTION(this);

    if (!Simulator::IsExpired(m_eventRequestMainObject))
    {
        NS_LOG_INFO(this << " Client  " << " Canceling RequestMainObject() which is due in "
                         << Simulator::GetDelayLeft(m_eventRequestMainObject).As(Time::S) << ".");
        Simulator::Cancel(m_eventRequestMainObject);
    }

    if (!Simulator::IsExpired(m_eventRequestEmbeddedObject))
    {
        NS_LOG_INFO(this << " Client  " << " Canceling RequestEmbeddedObject() which is due in "
                         << Simulator::GetDelayLeft(m_eventRequestEmbeddedObject).As(Time::S)
                         << ".");
        Simulator::Cancel(m_eventRequestEmbeddedObject);
    }

    if (!Simulator::IsExpired(m_eventParseMainObject))
    {
        NS_LOG_INFO(this << " Client  " << " Canceling ParseMainObject() which is due in "
                         << Simulator::GetDelayLeft(m_eventParseMainObject).As(Time::S) << ".");
        Simulator::Cancel(m_eventParseMainObject);
    }

    if (!Simulator::IsExpired(m_eventDisplayVideoChunk))
    {
        NS_LOG_INFO(this << " Client  " << " Canceling DisplayVideoChunk() which is due in "
                         << Simulator::GetDelayLeft(m_eventDisplayVideoChunk).As(Time::S) << ".");
        Simulator::Cancel(m_eventDisplayVideoChunk);
    }
}

void
VideoStreamClient::SwitchToState(VideoStreamClient::State_t state)
{
    const std::string oldState = GetStateString();
    const std::string newState = GetStateString(state);
    NS_LOG_FUNCTION(this << oldState << newState);

    if ((state == EXPECTING_MAIN_OBJECT) || (state == EXPECTING_EMBEDDED_OBJECT))
    {
        if (m_objectBytesToBeReceived > 0)
        {
            NS_FATAL_ERROR("Cannot start a new receiving session"
                           << " if the previous object"
                           << " (" << m_objectBytesToBeReceived << " bytes)"
                           << " is not completely received yet.");
        }
    }

    m_state = state;
    NS_LOG_INFO(this << " Client  " << " HttpClient " << oldState << " --> " << newState << ".");
    m_stateTransitionTrace(oldState, newState);
}

void
VideoStreamClient::FinishReceivingPage()
{
    m_rxPageTrace(this,
                  Simulator::Now() - m_pageLoadStartTs,
                  m_numberEmbeddedObjectsRequested,
                  m_numberBytesPage);
    // Reset counter variables.
    m_numberEmbeddedObjectsRequested = 0;
    m_numberBytesPage = 0;
}

} // namespace ns3
