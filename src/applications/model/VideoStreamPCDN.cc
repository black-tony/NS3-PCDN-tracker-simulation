/**
 * Author: Yinghao Yang <blacktonyrl@gmail.com>
 *
 */

// #include "three-gpp-http-server.h"
#include "VideoStreamPCDN.h"

#include "three-gpp-http-variables.h"
#include <ns3/double.h>

#include <ns3/callback.h>
#include <ns3/config.h>
#include <ns3/inet-socket-address.h>
#include <ns3/inet6-socket-address.h>
#include <ns3/log.h>
#include <ns3/packet.h>
#include <ns3/pointer.h>
#include <ns3/simulator.h>
#include <ns3/socket.h>
#include <ns3/tcp-socket-factory.h>
#include <ns3/tcp-socket.h>
#include <ns3/uinteger.h>

NS_LOG_COMPONENT_DEFINE("VideoStreamPCDN");

namespace ns3
{

// HTTP SERVER ////////////////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(VideoStreamPCDN);

VideoStreamPCDN::VideoStreamPCDN()
    : m_state(NOT_STARTED),
     m_clientState(CLIENT_NOT_STARTED),
      m_initialSocket(nullptr),
      m_P2PSocket(nullptr),
      m_txBuffer(Create<VideoStreamPCDNTxBuffer>()),
      m_httpVariables(CreateObject<ThreeGppHttpVariables>())
    //   m_objectBytesToBeReceived(0)
{
    NS_LOG_FUNCTION(this);
    m_bitrateSet[0] = 350;
    m_bitrateSet[1] = 600;
    m_bitrateSet[2] = 1000;
    m_bitrateSet[3] = 2000;
    m_bitrateSet[4] = 3000;
    // m_objectBytesToBeReceived = 0;
    m_videoChunkTotalNumber[0] = 5;
    m_videoChunkTotalNumber[1] = 10;

    m_mtuSize = m_httpVariables->GetMtuSize();
    m_mtuSize = 1460;
    NS_LOG_INFO(this << " PCDN " << " MTU size for this server application is " << m_mtuSize << " bytes.");
}

// static
TypeId
VideoStreamPCDN::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::VideoStreamPCDN")
            .SetParent<Application>()
            .AddConstructor<VideoStreamPCDN>()
            .AddAttribute("Variables",
                          "Variable collection, which is used to control e.g. processing and "
                          "object generation delays.",
                          PointerValue(),
                          MakePointerAccessor(&VideoStreamPCDN::m_httpVariables),
                          MakePointerChecker<ThreeGppHttpVariables>())
            .AddAttribute("LocalAddress",
                          "The local address of the server, "
                          "i.e., the address on which to bind the Rx socket.",
                          AddressValue(),
                          MakeAddressAccessor(&VideoStreamPCDN::m_localAddress),
                          MakeAddressChecker())
            .AddAttribute("LocalPort",
                          "Port on which the application listen for incoming packets.",
                          UintegerValue(80), // the default HTTP port
                          MakeUintegerAccessor(&VideoStreamPCDN::m_localPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("RemoteServerAddress",
                          "The address of the destination server.",
                          AddressValue(),
                          MakeAddressAccessor(&VideoStreamPCDN::m_remoteServerAddress),
                          MakeAddressChecker())
            .AddAttribute("RemoteServerPort",
                          "The destination port of the outbound packets.",
                          UintegerValue(80), // the default HTTP port
                          MakeUintegerAccessor(&VideoStreamPCDN::m_remoteServerPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("P2PServerAddress",
                          "The address of the destination server.",
                          AddressValue(),
                          MakeAddressAccessor(&VideoStreamPCDN::m_P2PAddress),
                          MakeAddressChecker())
            .AddAttribute("P2PServerPort",
                          "The destination port of the outbound packets.",
                          UintegerValue(8000), // the default HTTP port
                          MakeUintegerAccessor(&VideoStreamPCDN::m_P2PPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Mtu",
                          "Maximum transmission unit (in bytes) of the TCP sockets "
                          "used in this application, excluding the compulsory 40 "
                          "bytes TCP header. Typical values are 1460 and 536 bytes. "
                          "The attribute is read-only because the value is randomly "
                          "determined.",
                          TypeId::ATTR_GET,
                          UintegerValue(),
                          MakeUintegerAccessor(&VideoStreamPCDN::m_mtuSize),
                          MakeUintegerChecker<uint32_t>())
            .AddTraceSource("ConnectionEstablished",
                            "Connection to a remote web client has been established.",
                            MakeTraceSourceAccessor(&VideoStreamPCDN::m_connectionEstablishedTrace),
                            "ns3::HttpServer::ConnectionEstablishedCallback")
            .AddTraceSource("MainObject",
                            "A main object has been generated.",
                            MakeTraceSourceAccessor(&VideoStreamPCDN::m_mainObjectTrace),
                            "ns3::HttpServer::HttpObjectCallback")
            .AddTraceSource("EmbeddedObject",
                            "An embedded object has been generated.",
                            MakeTraceSourceAccessor(&VideoStreamPCDN::m_embeddedObjectTrace),
                            "ns3::HttpServer::HttpObjectCallback")
            .AddTraceSource("Tx",
                            "A packet has been sent.",
                            MakeTraceSourceAccessor(&VideoStreamPCDN::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("Rx",
                            "A packet has been received.",
                            MakeTraceSourceAccessor(&VideoStreamPCDN::m_rxTrace),
                            "ns3::Packet::PacketAddressTracedCallback")
            .AddTraceSource("RxDelay",
                            "A packet has been received with delay information.",
                            MakeTraceSourceAccessor(&VideoStreamPCDN::m_rxDelayTrace),
                            "ns3::Application::DelayAddressCallback")
            .AddTraceSource("StateTransition",
                            "Trace fired upon every HTTP client state transition.",
                            MakeTraceSourceAccessor(&VideoStreamPCDN::m_stateTransitionTrace),
                            "ns3::Application::StateTransitionCallback");
    return tid;
}

void
VideoStreamPCDN::SetMtuSize(uint32_t mtuSize)
{
    NS_LOG_FUNCTION(this << mtuSize);
    m_mtuSize = mtuSize;
}

Ptr<Socket>
VideoStreamPCDN::GetServerSocket() const
{
    return m_initialSocket;
}

Ptr<Socket>
VideoStreamPCDN::GetClientSocket() const
{
    return m_clientSocket;
}

VideoStreamPCDN::StateServer_t
VideoStreamPCDN::GetServerState() const
{
    return m_state;
}

VideoStreamPCDN::StateClient_t
VideoStreamPCDN::GetClientState() const
{
    return m_clientState;
}

std::string
VideoStreamPCDN::GetStateServerString() const
{
    return GetStateServerString(m_state);
}

std::string
VideoStreamPCDN::GetStateClientString() const
{
    return GetStateClientString(m_clientState);
}

// static
std::string
VideoStreamPCDN::GetStateServerString(VideoStreamPCDN::StateServer_t state)
{
    switch (state)
    {
    case NOT_STARTED:
        return "NOT_STARTED";
    case STARTED:
        return "STARTED";
    case STOPPED:
        return "STOPPED";
    default:
        NS_FATAL_ERROR("Unknown state");
        return "FATAL_ERROR";
    }
}

std::string
VideoStreamPCDN::GetStateClientString(StateClient_t state)
{
    switch (state)
    {
    case CLIENT_NOT_STARTED:
        return "NOT_STARTED";
    case CLIENT_CONNECTING:
        return "CONNECTING";
    case CLIENT_EXPECTING_MAIN_OBJECT:
        return "EXPECTING_MAIN_OBJECT";
    case CLIENT_PARSING_MAIN_OBJECT:
        return "PARSING_MAIN_OBJECT";
    case CLIENT_EXPECTING_EMBEDDED_OBJECT:
        return "EXPECTING_EMBEDDED_OBJECT";
    case CLIENT_READING:
        return "READING";
    case CLIENT_STOPPED:
        return "STOPPED";
    default:
        NS_FATAL_ERROR("Unknown state");
        return "FATAL_ERROR";
    }
}

// TODO CHANGE START AND END LOGIC
void
VideoStreamPCDN::DoDispose()
{
    NS_LOG_FUNCTION(this);

    if (!Simulator::IsFinished())
    {
        StopApplication();
    }

    Application::DoDispose(); // Chain up.
}

void
VideoStreamPCDN::StartApplication()
{
    NS_LOG_FUNCTION(this);
    ClientOpenConnection();
    if (m_state == NOT_STARTED)
    {
        m_httpVariables->Initialize();
        if (!m_initialSocket)
        {
            // Find the current default MTU value of TCP sockets.
            Ptr<const ns3::AttributeValue> previousSocketMtu;
            const TypeId tcpSocketTid = TcpSocket::GetTypeId();
            for (uint32_t i = 0; i < tcpSocketTid.GetAttributeN(); i++)
            {
                TypeId::AttributeInformation attrInfo = tcpSocketTid.GetAttribute(i);
                if (attrInfo.name == "SegmentSize")
                {
                    previousSocketMtu = attrInfo.initialValue;
                }
            }

            // Creating a TCP socket to connect to the server.
            m_initialSocket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
            m_initialSocket->SetAttribute("SegmentSize", UintegerValue(m_mtuSize));

            if (Ipv4Address::IsMatchingType(m_localAddress))
            {
                const Ipv4Address ipv4 = Ipv4Address::ConvertFrom(m_localAddress);
                const InetSocketAddress inetSocket = InetSocketAddress(ipv4, m_localPort);
                NS_LOG_INFO(this << " PCDN " << " Binding on " << ipv4 << " port " << m_localPort << " / "
                                 << inetSocket << ".");
                int ret [[maybe_unused]] = m_initialSocket->Bind(inetSocket);
                NS_LOG_DEBUG(this << " Bind() return value= " << ret
                                  << " GetErrNo= " << m_initialSocket->GetErrno() << ".");
            }
            else if (Ipv6Address::IsMatchingType(m_localAddress))
            {
                const Ipv6Address ipv6 = Ipv6Address::ConvertFrom(m_localAddress);
                const Inet6SocketAddress inet6Socket = Inet6SocketAddress(ipv6, m_localPort);
                NS_LOG_INFO(this << " PCDN " << " Binding on " << ipv6 << " port " << m_localPort << " / "
                                 << inet6Socket << ".");
                int ret [[maybe_unused]] = m_initialSocket->Bind(inet6Socket);
                NS_LOG_DEBUG(this << " Bind() return value= " << ret
                                  << " GetErrNo= " << m_initialSocket->GetErrno() << ".");
            }

            int ret [[maybe_unused]] = m_initialSocket->Listen();
            NS_LOG_DEBUG(this << " Listen () return value= " << ret
                              << " GetErrNo= " << m_initialSocket->GetErrno() << ".");

        } // end of `if (m_initialSocket == 0)`

        NS_ASSERT_MSG(m_initialSocket, "Failed creating socket.");
        m_initialSocket->SetAcceptCallback(
            MakeCallback(&VideoStreamPCDN::ConnectionRequestCallback, this),
            MakeCallback(&VideoStreamPCDN::NewConnectionCreatedCallback, this));
        m_initialSocket->SetCloseCallbacks(MakeCallback(&VideoStreamPCDN::NormalCloseCallback, this),
                                           MakeCallback(&VideoStreamPCDN::ErrorCloseCallback, this));
        m_initialSocket->SetRecvCallback(MakeCallback(&VideoStreamPCDN::ReceivedDataCallback, this));
        m_initialSocket->SetSendCallback(MakeCallback(&VideoStreamPCDN::SendCallback, this));
        // SwitchToStateServer(STARTED);

        //P2P listening
        if (!m_P2PSocket)
        {
            // Find the current default MTU value of TCP sockets.
            Ptr<const ns3::AttributeValue> previousSocketMtu;
            const TypeId tcpSocketTid = TcpSocket::GetTypeId();
            for (uint32_t i = 0; i < tcpSocketTid.GetAttributeN(); i++)
            {
                TypeId::AttributeInformation attrInfo = tcpSocketTid.GetAttribute(i);
                if (attrInfo.name == "SegmentSize")
                {
                    previousSocketMtu = attrInfo.initialValue;
                }
            }

            // Creating a TCP socket to connect to the server.
            m_P2PSocket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
            m_P2PSocket->SetAttribute("SegmentSize", UintegerValue(m_mtuSize));

            if (Ipv4Address::IsMatchingType(m_P2PAddress))
            {
                const Ipv4Address ipv4 = Ipv4Address::ConvertFrom(m_P2PAddress);
                const InetSocketAddress inetSocket = InetSocketAddress(ipv4, m_P2PPort);
                NS_LOG_INFO(this << " PCDN " << " P2P Binding on " << ipv4 << " port " << m_P2PPort << " / "
                                 << inetSocket << ".");
                int ret [[maybe_unused]] = m_P2PSocket->Bind(inetSocket);
                NS_LOG_DEBUG(this << " P2P Bind() return value= " << ret
                                  << " GetErrNo= " << m_P2PSocket->GetErrno() << ".");
            }
            else if (Ipv6Address::IsMatchingType(m_P2PAddress))
            {
                const Ipv6Address ipv6 = Ipv6Address::ConvertFrom(m_P2PAddress);
                const Inet6SocketAddress inet6Socket = Inet6SocketAddress(ipv6, m_P2PPort);
                NS_LOG_INFO(this << " PCDN " << " P2P Binding on " << ipv6 << " port " << m_P2PPort << " / "
                                 << inet6Socket << ".");
                int ret [[maybe_unused]] = m_P2PSocket->Bind(inet6Socket);
                NS_LOG_DEBUG(this << " P2P Bind() return value= " << ret
                                  << " GetErrNo= " << m_P2PSocket->GetErrno() << ".");
            }

            int ret [[maybe_unused]] = m_P2PSocket->Listen();
            NS_LOG_DEBUG(this << " Listen () return value= " << ret
                              << " GetErrNo= " << m_P2PSocket->GetErrno() << ".");

        } // end of `if (m_P2PSocket == 0)`

        NS_ASSERT_MSG(m_P2PSocket, "Failed creating socket.");
        m_P2PSocket->SetAcceptCallback(
            MakeCallback(&VideoStreamPCDN::ConnectionRequestCallback, this),
            MakeCallback(&VideoStreamPCDN::NewConnectionCreatedCallback, this));
        m_P2PSocket->SetCloseCallbacks(MakeCallback(&VideoStreamPCDN::NormalCloseCallback, this),
                                           MakeCallback(&VideoStreamPCDN::ErrorCloseCallback, this));
        m_P2PSocket->SetRecvCallback(MakeCallback(&VideoStreamPCDN::ReceivedDataCallback, this));
        m_P2PSocket->SetSendCallback(MakeCallback(&VideoStreamPCDN::SendCallback, this));
        SwitchToStateServer(STARTED);



    } // end of `if (m_state == NOT_STARTED)`
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateServerString() << " for StartApplication().");
    }

} // end of `void StartApplication ()`

void
VideoStreamPCDN::StopApplication()
{
    NS_LOG_FUNCTION(this);

    SwitchToStateServer(STOPPED);

    // Close all accepted sockets.
    m_txBuffer->CloseAllSockets();

    // Stop listening.
    if (m_initialSocket)
    {
        m_initialSocket->Close();
        m_initialSocket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                                           MakeNullCallback<void, Ptr<Socket>, const Address&>());
        m_initialSocket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                           MakeNullCallback<void, Ptr<Socket>>());
        m_initialSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        m_initialSocket->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());
    }
    if (m_P2PSocket)
    {
        m_P2PSocket->Close();
        m_P2PSocket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                                           MakeNullCallback<void, Ptr<Socket>, const Address&>());
        m_P2PSocket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                           MakeNullCallback<void, Ptr<Socket>>());
        m_P2PSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        m_P2PSocket->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());
    }
    for(auto &i : m_peerSocketSet)
    {
        ClientCancelAllPendingEvents(i);
        i->Close();
        i->SetConnectCallback(MakeNullCallback<void, Ptr<Socket>>(),
                                 MakeNullCallback<void, Ptr<Socket>>());
        i->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
    m_clientSocket->Close();
    m_clientSocket->SetConnectCallback(MakeNullCallback<void, Ptr<Socket>>(),
                                 MakeNullCallback<void, Ptr<Socket>>());
    m_clientSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
}

bool
VideoStreamPCDN::ConnectionRequestCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this << socket << address);
    return true; // Unconditionally accept the connection request.
}

void
VideoStreamPCDN::NewConnectionCreatedCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this << socket << address);

    socket->SetCloseCallbacks(MakeCallback(&VideoStreamPCDN::NormalCloseCallback, this),
                              MakeCallback(&VideoStreamPCDN::ErrorCloseCallback, this));
    socket->SetRecvCallback(MakeCallback(&VideoStreamPCDN::ReceivedDataCallback, this));
    socket->SetSendCallback(MakeCallback(&VideoStreamPCDN::SendCallback, this));

    m_connectionEstablishedTrace(this, socket);
    m_txBuffer->AddSocket(socket);

    /*
     * A typical connection is established after receiving an empty (i.e., no
     * data) TCP packet with ACK flag. The actual data will follow in a separate
     * packet after that and will be received by ReceivedDataCallback().
     *
     * However, that empty ACK packet might get lost. In this case, we may
     * receive the first data packet right here already, because it also counts
     * as a new connection. The statement below attempts to fetch the data from
     * that packet, if any.
     */
    ReceivedDataCallback(socket);
}

void
VideoStreamPCDN::NormalCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    if (socket == m_initialSocket)
    {
        if (m_state == STARTED)
        {
            NS_FATAL_ERROR("Initial listener socket shall not be closed"
                           << " when the server instance is still running.");
        }
    }
    else if (m_txBuffer->IsSocketAvailable(socket))
    {
        // The application should now prepare to close the socket.
        if (m_txBuffer->IsBufferEmpty(socket))
        {
            /*
             * Here we declare that we have nothing more to send and the socket
             * may be closed immediately.
             */
            socket->ShutdownSend();
            m_txBuffer->RemoveSocket(socket);
        }
        else
        {
            /*
             * Remember to close the socket later, whenever the buffer becomes
             * empty.
             */
            m_txBuffer->PrepareClose(socket);
        }
    }
}

void
VideoStreamPCDN::ErrorCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    if (socket == m_initialSocket)
    {
        if (m_state == STARTED)
        {
            NS_FATAL_ERROR("Initial listener socket shall not be closed"
                           << " when the server instance is still running.");
        }
    }
    else if (m_txBuffer->IsSocketAvailable(socket))
    {
        m_txBuffer->CloseSocket(socket);
    }
}

void
VideoStreamPCDN::ReceivedDataCallback(Ptr<Socket> socket)
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
        // if (InetSocketAddress::IsMatchingType(from))
        // {
        //     NS_LOG_INFO(this << " PCDN " << " A packet of " << packet->GetSize() << " bytes"
        //                      << " received from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
        //                      << " port " << InetSocketAddress::ConvertFrom(from).GetPort() << " / "
        //                      << InetSocketAddress::ConvertFrom(from));
        // }
        // else if (Inet6SocketAddress::IsMatchingType(from))
        // {
        //     NS_LOG_INFO(this << " PCDN " << " A packet of " << packet->GetSize() << " bytes"
        //                      << " received from " << Inet6SocketAddress::ConvertFrom(from).GetIpv6()
        //                      << " port " << Inet6SocketAddress::ConvertFrom(from).GetPort() << " / "
        //                      << Inet6SocketAddress::ConvertFrom(from));
        // }
#endif /* NS3_LOG_ENABLE */

        // Check the header. No need to remove it, since it is not a "real" header.
        ThreeGppHttpHeader httpHeader;
        packet->RemoveHeader(httpHeader);
        VideoStreamHeader vHeader;
        packet->PeekHeader(vHeader);
        m_bitrateType = vHeader.GetBitrateType();
        m_videoRequested = vHeader.GetVideoNumber();
        m_VideoChunkRequested = vHeader.GetVideoChunkNumber();
        NS_LOG_INFO(this << " PCDN " << "recieve streaming request" << vHeader.ToString());
        packet->AddHeader(httpHeader);
        // Fire trace sources.
        m_rxTrace(packet, from);
        m_rxDelayTrace(Simulator::Now() - httpHeader.GetClientTs(), from);

        Time processingDelay;
        switch (httpHeader.GetContentType())
        {
        case ThreeGppHttpHeader::MAIN_OBJECT:
            // processingDelay = m_httpVariables->GetMainObjectGenerationDelay();
            processingDelay = Seconds(0);
            if(m_cachedVideoNumber.find(m_videoRequested) != m_cachedVideoNumber.end())
            {
                // NS_LOG_INFO(this << " PCDN " << " Will finish generating a main object"
                //                 << " in " << processingDelay.As(Time::S) << ".");
                NS_LOG_INFO(this << " PCDN " << " find " << m_videoRequested << " has already cached.");
                m_txBuffer->RecordNextServe(socket,
                                            Simulator::Schedule(processingDelay,
                                                                &VideoStreamPCDN::ServeNewMainObject,
                                                                this,
                                                                socket, m_videoRequested),
                                            httpHeader.GetClientTs());
            }
            else
            {
                auto it = m_forwardTable.find(m_videoRequested);
                if(it == m_forwardTable.end())
                    m_forwardTable[m_videoRequested] = std::set<Ptr<Socket>>();
                m_forwardTable[m_videoRequested].insert(socket);
                Simulator::ScheduleNow(&VideoStreamPCDN::ClientRequestMainObject, this, m_clientSocket,  m_videoRequested);
            }
            break;

        case ThreeGppHttpHeader::EMBEDDED_OBJECT:
            processingDelay = m_httpVariables->GetEmbeddedObjectGenerationDelay();
            NS_LOG_INFO(this << " PCDN " << " Will finish generating an embedded object"
                             << " in " << processingDelay.As(Time::S) << ".");
            m_txBuffer->RecordNextServe(socket,
                                        Simulator::Schedule(processingDelay,
                                                            &VideoStreamPCDN::ServeNewEmbeddedObject,
                                                            this,
                                                            socket),
                                        httpHeader.GetClientTs());
            break;

        default:
            NS_FATAL_ERROR("Invalid packet.");
            break;
        }

    } // end of `while ((packet = socket->RecvFrom (from)))`

} // end of `void ReceivedDataCallback (Ptr<Socket> socket)`

void
VideoStreamPCDN::SendCallback(Ptr<Socket> socket, uint32_t availableBufferSize)
{
    NS_LOG_FUNCTION(this << socket << availableBufferSize);

    if (!m_txBuffer->IsBufferEmpty(socket))
    {
        const uint32_t txBufferSize [[maybe_unused]] = m_txBuffer->GetBufferSize(socket);
        const uint32_t actualSent [[maybe_unused]] = ServeFromTxBuffer(socket);

#ifdef NS3_LOG_ENABLE
        // Some log messages.
        if (actualSent < txBufferSize)
        {
            switch (m_txBuffer->GetBufferContentType(socket))
            {
            case ThreeGppHttpHeader::MAIN_OBJECT:
                NS_LOG_INFO(this << " PCDN " << " Transmission of main object is suspended"
                                 << " after " << actualSent << " bytes.");
                break;
            case ThreeGppHttpHeader::EMBEDDED_OBJECT:
                NS_LOG_INFO(this << " PCDN " << " Transmission of embedded object is suspended"
                                 << " after " << actualSent << " bytes.");
                break;
            default:
                NS_FATAL_ERROR("Invalid Tx buffer content type.");
                break;
            }
        }
        else
        {
            switch (m_txBuffer->GetBufferContentType(socket))
            {
            case ThreeGppHttpHeader::MAIN_OBJECT:
                NS_LOG_INFO(this << " PCDN " << " Finished sending a whole main object.");
                break;
            case ThreeGppHttpHeader::EMBEDDED_OBJECT:
                NS_LOG_INFO(this << " PCDN " << " Finished sending a whole embedded object.");
                break;
            default:
                NS_FATAL_ERROR("Invalid Tx buffer content type.");
                break;
            }
        }
#endif /* NS3_LOG_ENABLE */

    } // end of `if (m_txBuffer->IsBufferEmpty (socket))`

} // end of `void SendCallback (Ptr<Socket> socket, uint32_t availableBufferSize)`

void
VideoStreamPCDN::ServeNewMainObject(Ptr<Socket> socket, uint32_t videonumber)
{
    NS_LOG_FUNCTION(this << socket);

    // const uint32_t objectSize = m_httpVariables->GetMainObjectSize();
    uint32_t objectSize = m_bitrateSet[m_bitrateType];
    objectSize = (uint32_t)objectSize * 4 * 1000 / 8;
    NS_LOG_INFO(this << " PCDN " << " Main object to be served is " << objectSize << " bytes.");

    m_mainObjectTrace(objectSize);
    if(!m_txBuffer->IsBufferEmpty(socket))
    {
        Simulator::Schedule(MilliSeconds(20), &VideoStreamPCDN::ServeNewMainObject, this, socket, videonumber);
        return;
    }
    m_txBuffer->WriteNewObject(socket, ThreeGppHttpHeader::MAIN_OBJECT, objectSize);
    const uint32_t actualSent = ServeFromTxBuffer(socket, videonumber);

    if (actualSent < objectSize)
    {
        NS_LOG_INFO(this << " PCDN " << " Transmission of main object is suspended"
                         << " after " << actualSent << " bytes.");
    }
    else
    {
        NS_LOG_INFO(this << " PCDN " << " Finished sending a whole main object.");
    }
}

void
VideoStreamPCDN::ServeNewEmbeddedObject(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    const uint32_t objectSize = m_httpVariables->GetEmbeddedObjectSize();
    NS_LOG_INFO(this << " PCDN " << " Embedded object to be served is " << objectSize << " bytes.");
    m_embeddedObjectTrace(objectSize);
    m_txBuffer->WriteNewObject(socket, ThreeGppHttpHeader::EMBEDDED_OBJECT, objectSize);
    const uint32_t actualSent = ServeFromTxBuffer(socket);

    if (actualSent < objectSize)
    {
        // NS_LOG_INFO(this << " PCDN " << " Transmission of embedded object is suspended"
        //                  << " after " << actualSent << " bytes.");
    }
    else
    {
        NS_LOG_INFO(this << " PCDN " << " Finished sending a whole embedded object.");
    }
}

uint32_t
VideoStreamPCDN::ServeFromTxBuffer(Ptr<Socket> socket, uint32_t videonumber)
{
    NS_LOG_FUNCTION(this << socket);

    if (m_txBuffer->IsBufferEmpty(socket))
    {
        NS_LOG_LOGIC(this << " Tx buffer is empty. Not sending anything.");
        return 0;
    }
    bool firstPartOfObject = !m_txBuffer->HasTxedPartOfObject(socket);

    const uint32_t socketSize = socket->GetTxAvailable();
    NS_LOG_DEBUG(this << " Socket has " << socketSize << " bytes available for Tx.");

    // Get the number of bytes remaining to be sent.
    const uint32_t txBufferSize = m_txBuffer->GetBufferSize(socket);

    // Compute the size of actual content to be sent; has to fit into the socket.
    // Note that header size is NOT counted as TxBuffer content. Header size is overhead.
    uint32_t contentSize = std::min(txBufferSize, socketSize - 22 - VideoStreamHeader::GetHeaderSize());
    Ptr<Packet> packet = Create<Packet>(contentSize);
    uint32_t packetSize = contentSize;
    if (packetSize == 0)
    {
        NS_LOG_LOGIC(this << " Socket size leads to packet size of zero; not sending anything.");
        return 0;
    }

    // If this is the first packet of an object, attach a header.
    if (firstPartOfObject)
    {
        // Create header.
        VideoStreamHeader vHeader;
        vHeader.SetContentType(VideoStreamHeader::VIDEO_CHUNK);
        vHeader.SetVideoNumber(videonumber);
        vHeader.SetBitrateType(m_bitrateType);
        vHeader.SetVideoChunkNumber(m_VideoChunkRequested);
        const uint32_t videochunksize = 123456;
        const uint32_t videototalNumber = 666666;
        vHeader.setVideoChunkSize(videochunksize);
        vHeader.SetVideoChunkTotalNumber(videototalNumber);
        packet->AddHeader(vHeader);
        packetSize += vHeader.GetSerializedSize();
        ThreeGppHttpHeader httpHeader;
        httpHeader.SetContentLength(txBufferSize + vHeader.GetSerializedSize());
        httpHeader.SetContentType(m_txBuffer->GetBufferContentType(socket));
        // Using the client TS value as per the corresponding request packet.
        httpHeader.SetClientTs(m_txBuffer->GetClientTs(socket));
        httpHeader.SetServerTs(Simulator::Now());
        packet->AddHeader(httpHeader);
        packetSize += httpHeader.GetSerializedSize();

        NS_LOG_INFO(this << " PCDN " << " Created packet " << packet << " of " << packetSize << " bytes."
                         << " The corresponding request came "
                         << (Simulator::Now() - httpHeader.GetClientTs()).As(Time::S) << " ago.");
    }
    else
    {
        // NS_LOG_INFO(this << " PCDN " << " Created packet " << packet << " of " << packetSize
        //                  << " bytes to be appended to a previous packet.");
    }

    // Send.
    const int actualBytes = socket->Send(packet);
    NS_LOG_DEBUG(this << " Send() packet " << packet << " of " << packetSize << " bytes,"
                      << " return value= " << actualBytes << ".");
    m_txTrace(packet);

    if (actualBytes == static_cast<int>(packetSize))
    {
        // The packet goes through successfully.
        m_txBuffer->DepleteBufferSize(socket, contentSize);
        // NS_LOG_INFO(this << " PCDN " << " Remaining object to be sent " << m_txBuffer->GetBufferSize(socket)
        //                  << " bytes.");
        return packetSize;
    }
    else
    {
        NS_LOG_INFO(this << " PCDN " << " Failed to send object,"
                         << " GetErrNo= " << socket->GetErrno() << ","
                         << " suspending transmission"
                         << " and waiting for another Tx opportunity.");
        return 0;
    }

} // end of `uint32_t ServeFromTxBuffer (Ptr<Socket> socket)`
uint32_t
VideoStreamPCDN::ServeFromTxBuffer(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    if (m_txBuffer->IsBufferEmpty(socket))
    {
        NS_LOG_LOGIC(this << " Tx buffer is empty. Not sending anything.");
        return 0;
    }
    bool firstPartOfObject = !m_txBuffer->HasTxedPartOfObject(socket);

    const uint32_t socketSize = socket->GetTxAvailable();
    NS_LOG_DEBUG(this << " Socket has " << socketSize << " bytes available for Tx.");

    // Get the number of bytes remaining to be sent.
    const uint32_t txBufferSize = m_txBuffer->GetBufferSize(socket);

    // Compute the size of actual content to be sent; has to fit into the socket.
    // Note that header size is NOT counted as TxBuffer content. Header size is overhead.
    uint32_t contentSize = std::min(txBufferSize, socketSize - 22 - VideoStreamHeader::GetHeaderSize());
    Ptr<Packet> packet = Create<Packet>(contentSize);
    uint32_t packetSize = contentSize;
    if (packetSize == 0)
    {
        NS_LOG_LOGIC(this << " Socket size leads to packet size of zero; not sending anything.");
        return 0;
    }

    // If this is the first packet of an object, attach a header.
    if (firstPartOfObject)
    {
        // Create header.
        VideoStreamHeader vHeader;
        vHeader.SetContentType(VideoStreamHeader::VIDEO_CHUNK);
        vHeader.SetVideoNumber(m_videoRequested);
        vHeader.SetBitrateType(m_bitrateType);
        vHeader.SetVideoChunkNumber(m_VideoChunkRequested);
        const uint32_t videochunksize = 123456;
        const uint32_t videototalNumber = 666666;
        vHeader.setVideoChunkSize(videochunksize);
        vHeader.SetVideoChunkTotalNumber(videototalNumber);
        packet->AddHeader(vHeader);
        packetSize += vHeader.GetSerializedSize();
        ThreeGppHttpHeader httpHeader;
        httpHeader.SetContentLength(txBufferSize + vHeader.GetSerializedSize());
        httpHeader.SetContentType(m_txBuffer->GetBufferContentType(socket));
        // Using the client TS value as per the corresponding request packet.
        httpHeader.SetClientTs(m_txBuffer->GetClientTs(socket));
        httpHeader.SetServerTs(Simulator::Now());
        packet->AddHeader(httpHeader);
        packetSize += httpHeader.GetSerializedSize();

        NS_LOG_INFO(this << " PCDN " << " Created packet " << packet << " of " << packetSize << " bytes."
                         << " The corresponding request came "
                         << (Simulator::Now() - httpHeader.GetClientTs()).As(Time::S) << " ago.");
    }
    else
    {
        // NS_LOG_INFO(this << " PCDN " << " Created packet " << packet << " of " << packetSize
        //                  << " bytes to be appended to a previous packet.");
    }

    // Send.
    const int actualBytes = socket->Send(packet);
    NS_LOG_DEBUG(this << " Send() packet " << packet << " of " << packetSize << " bytes,"
                      << " return value= " << actualBytes << ".");
    m_txTrace(packet);

    if (actualBytes == static_cast<int>(packetSize))
    {
        // The packet goes through successfully.
        m_txBuffer->DepleteBufferSize(socket, contentSize);
        // NS_LOG_INFO(this << " PCDN " << " Remaining object to be sent " << m_txBuffer->GetBufferSize(socket)
        //                  << " bytes.");
        return packetSize;
    }
    else
    {
        NS_LOG_INFO(this << " PCDN " << " Failed to send object,"
                         << " GetErrNo= " << socket->GetErrno() << ","
                         << " suspending transmission"
                         << " and waiting for another Tx opportunity.");
        return 0;
    }

} // end of `uint32_t ServeFromTxBuffer (Ptr<Socket> socket)`
void
VideoStreamPCDN::SwitchToStateServer(VideoStreamPCDN::StateServer_t state)
{
    const std::string oldState = GetStateServerString();
    const std::string newState = GetStateServerString(state);
    NS_LOG_FUNCTION(this << oldState << newState);
    m_state = state;
    NS_LOG_INFO(this << " PCDN " << " VideoStreamPCDN " << oldState << " --> " << newState << ".");
    m_stateTransitionTrace(oldState, newState);
}

void
VideoStreamPCDN::SwitchToStateClient(StateClient_t state)
{
    const std::string oldState = GetStateClientString();
    const std::string newState = GetStateClientString(state);
    NS_LOG_FUNCTION(this << oldState << newState);

    // if ((state == CLIENT_EXPECTING_MAIN_OBJECT) || (state == CLIENT_EXPECTING_EMBEDDED_OBJECT))
    // {
    //     if (m_objectBytesToBeReceived > 0)
    //     {
    //         NS_FATAL_ERROR("Cannot start a new receiving session"
    //                        << " if the previous object"
    //                        << " (" << m_objectBytesToBeReceived << " bytes)"
    //                        << " is not completely received yet.");
    //     }
    // }

    m_clientState = state;
    NS_LOG_INFO(this << " PCDN " << " HttpClient " << oldState << " --> " << newState << ".");
    m_stateTransitionTrace(oldState, newState);
}

void
VideoStreamPCDN::ClientConnectionSucceededCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    if (m_clientState == CLIENT_CONNECTING)
    {
        NS_ASSERT_MSG(m_clientSocket == socket || (m_peerSocketSet.find(socket) != m_peerSocketSet.end()), "Invalid socket.");
        // m_connectionEstablishedTrace(this);
        // TODO TRACE modify
        socket->SetRecvCallback(MakeCallback(&VideoStreamPCDN::ClientReceivedDataCallback, this));
        NS_ASSERT(m_P2PSocketInfo[socket].m_embeddedObjectsToBeRequested == 0);
        // m_eventRequestMainObject =
        //     Simulator::ScheduleNow(&VideoStreamPCDN::ClientRequestMainObject, this);
    }
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateClientString() << " for ConnectionSucceeded().");
    }
}

void
VideoStreamPCDN::ClientConnectionFailedCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    if (m_clientState == CLIENT_CONNECTING)
    {
        NS_LOG_ERROR("Client failed to connect"
                     << " to remote address " << m_remoteServerAddress << " port "
                     << m_remoteServerPort << ".");
    }
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateClientString() << " for ConnectionFailed().");
    }
}

void
VideoStreamPCDN::ClientNormalCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    ClientCancelAllPendingEvents(socket);

    if (socket->GetErrno() != Socket::ERROR_NOTERROR)
    {
        NS_LOG_ERROR(this << " Connection has been terminated,"
                          << " error code: " << socket->GetErrno() << ".");
    }

    socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                MakeNullCallback<void, Ptr<Socket>>());
    if(m_peerSocketSet.find(socket) != m_peerSocketSet.end())
    {
        m_peerSocketSet.erase(socket);
        m_P2PSocketInfo.erase(socket);
    }
    // m_connectionClosedTrace(this);
}

void
VideoStreamPCDN::ClientErrorCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    ClientCancelAllPendingEvents(socket);
    if (socket->GetErrno() != Socket::ERROR_NOTERROR)
    {
        NS_LOG_ERROR(this << " Connection has been terminated,"
                          << " error code: " << socket->GetErrno() << ".");
    }
    if(m_peerSocketSet.find(socket) != m_peerSocketSet.end())
    {
        m_peerSocketSet.erase(socket);
        m_P2PSocketInfo.erase(socket);
    }
    // m_connectionClosedTrace(this);
}

void
VideoStreamPCDN::ClientReceivedDataCallback(Ptr<Socket> socket)
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
            NS_LOG_INFO(this << " PCDN " << " A packet of " << packet->GetSize() << " bytes"
                             << " received from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
                             << " port " << InetSocketAddress::ConvertFrom(from).GetPort() << " / "
                             << InetSocketAddress::ConvertFrom(from) << ".");
        }
        else if (Inet6SocketAddress::IsMatchingType(from))
        {
            NS_LOG_INFO(this << " PCDN " << " A packet of " << packet->GetSize() << " bytes"
                             << " received from " << Inet6SocketAddress::ConvertFrom(from).GetIpv6()
                             << " port " << Inet6SocketAddress::ConvertFrom(from).GetPort() << " / "
                             << Inet6SocketAddress::ConvertFrom(from) << ".");
        }
#endif /* NS3_LOG_ENABLE */

        m_rxTrace(packet, from);
        ClientReceiveMainObject(packet, from, socket);

        // switch (m_state)
        // {
        // case CLIENT_EXPECTING_MAIN_OBJECT:
        //     break;
        // // case CLIENT_EXPECTING_EMBEDDED_OBJECT:
        // //     ClientReceiveEmbeddedObject(packet, from);
        // //     break;
        // default:
        //     NS_FATAL_ERROR("Invalid state " << GetStateClientString() << " for ReceivedData().");
        //     break;
        // }

    } // end of `while ((packet = socket->RecvFrom (from)))`
}

void
VideoStreamPCDN::ClientOpenConnection()
{
    NS_LOG_FUNCTION(this);

    if (m_clientState == CLIENT_NOT_STARTED || m_clientState == CLIENT_EXPECTING_EMBEDDED_OBJECT ||
        m_clientState == CLIENT_PARSING_MAIN_OBJECT || m_clientState == CLIENT_READING)
    {
        m_clientSocket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());

        if (Ipv4Address::IsMatchingType(m_remoteServerAddress))
        {
            int ret [[maybe_unused]];

            ret = m_clientSocket->Bind();
            NS_LOG_DEBUG(this << " Bind() return value= " << ret
                              << " GetErrNo= " << m_clientSocket->GetErrno() << ".");

            Ipv4Address ipv4 = Ipv4Address::ConvertFrom(m_remoteServerAddress);
            InetSocketAddress inetSocket = InetSocketAddress(ipv4, m_remoteServerPort);
            NS_LOG_INFO(this << " PCDN " << " Connecting to " << ipv4 << " port " << m_remoteServerPort << " / "
                             << inetSocket << ".");
            ret = m_clientSocket->Connect(inetSocket);
            NS_LOG_DEBUG(this << " Connect() return value= " << ret
                              << " GetErrNo= " << m_clientSocket->GetErrno() << ".");
        }
        else if (Ipv6Address::IsMatchingType(m_remoteServerAddress))
        {
            int ret [[maybe_unused]];

            ret = m_clientSocket->Bind6();
            NS_LOG_DEBUG(this << " Bind6() return value= " << ret
                              << " GetErrNo= " << m_clientSocket->GetErrno() << ".");

            Ipv6Address ipv6 = Ipv6Address::ConvertFrom(m_remoteServerAddress);
            Inet6SocketAddress inet6Socket = Inet6SocketAddress(ipv6, m_remoteServerPort);
            NS_LOG_INFO(this << " PCDN " << " connecting to " << ipv6 << " port " << m_remoteServerPort << " / "
                             << inet6Socket << ".");
            ret = m_clientSocket->Connect(inet6Socket);
            NS_LOG_DEBUG(this << " Connect() return value= " << ret
                              << " GetErrNo= " << m_clientSocket->GetErrno() << ".");
        }

        NS_ASSERT_MSG(m_clientSocket, "Failed creating socket.");

        SwitchToStateClient(CLIENT_CONNECTING);

        m_clientSocket->SetConnectCallback(
            MakeCallback(&VideoStreamPCDN::ClientConnectionSucceededCallback, this),
            MakeCallback(&VideoStreamPCDN::ClientConnectionFailedCallback, this));
        m_clientSocket->SetCloseCallbacks(MakeCallback(&VideoStreamPCDN::ClientNormalCloseCallback, this),
                                    MakeCallback(&VideoStreamPCDN::ClientErrorCloseCallback, this));
        m_clientSocket->SetRecvCallback(MakeCallback(&VideoStreamPCDN::ClientReceivedDataCallback, this));
        m_clientSocket->SetAttribute("MaxSegLifetime", DoubleValue(0.02)); // 20 ms.

    } // end of `if (m_state == {NOT_STARTED, EXPECTING_EMBEDDED_OBJECT, PARSING_MAIN_OBJECT,
      // READING})`
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateClientString() << " for OpenConnection().");
    }
}
Ptr<Socket>
VideoStreamPCDN::P2POpenConnection(Address P2PNode)
{
    NS_LOG_FUNCTION(this);

    // if (m_clientState == CLIENT_NOT_STARTED || m_clientState == CLIENT_EXPECTING_EMBEDDED_OBJECT ||
    //     m_clientState == CLIENT_PARSING_MAIN_OBJECT || m_clientState == CLIENT_READING)
    // {
    Ptr<Socket> socket =  Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());

    // if (Ipv4Address::IsMatchingType(P2PNode))
    // {
    if (InetSocketAddress::IsMatchingType(P2PNode))
    {
        InetSocketAddress test = InetSocketAddress::ConvertFrom(P2PNode);
        // NS_LOG_INFO(this << " Server " << " A packet of " << packet->GetSize() << " bytes"
        //                 << " received from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
        //                 << " port " << InetSocketAddress::ConvertFrom(from).GetPort() << " / "
        //                 << InetSocketAddress::ConvertFrom(from));
        int ret [[maybe_unused]];

        ret = socket->Bind();
        NS_LOG_DEBUG(this << " Bind() return value= " << ret
                            << " GetErrNo= " << socket->GetErrno() << ".");

        // Ipv4Address ipv4 = Ipv4Address::ConvertFrom(P2PNode);
        InetSocketAddress inetSocket = InetSocketAddress(test.GetIpv4(), m_P2PPort);
        NS_LOG_INFO(this << " PCDN " << " Connecting to " << test.GetIpv4() << " port " << m_P2PPort << " / "
                            << inetSocket << ".");
        ret = socket->Connect(inetSocket);
        NS_LOG_DEBUG(this << " Connect() return value= " << ret
                            << " GetErrNo= " << socket->GetErrno() << ".");
    }
    else if (Inet6SocketAddress::IsMatchingType(P2PNode))
    {
        Inet6SocketAddress test = Inet6SocketAddress::ConvertFrom(P2PNode);

        // NS_LOG_INFO(this << " Server " << " A packet of " << packet->GetSize() << " bytes"
        //                 << " received from " << Inet6SocketAddress::ConvertFrom(from).GetIpv6()
        //                 << " port " << Inet6SocketAddress::ConvertFrom(from).GetPort() << " / "
        //                 << Inet6SocketAddress::ConvertFrom(from));
        int ret [[maybe_unused]];

        ret = socket->Bind();
        NS_LOG_DEBUG(this << " Bind() return value= " << ret
                            << " GetErrNo= " << socket->GetErrno() << ".");

        // Ipv4Address ipv4 = Ipv4Address::ConvertFrom(P2PNode);
        Inet6SocketAddress inetSocket = Inet6SocketAddress(test.GetIpv6(), m_P2PPort);
        NS_LOG_INFO(this << " PCDN " << " Connecting to " << test.GetIpv6() << " port " << m_P2PPort << " / "
                            << inetSocket << ".");
        ret = socket->Connect(inetSocket);
        NS_LOG_DEBUG(this << " Connect() return value= " << ret
                            << " GetErrNo= " << socket->GetErrno() << ".");
    }
        
    // }
    // else if (Ipv6Address::IsMatchingType(P2PNode))
    // {
    //     int ret [[maybe_unused]];

    //     ret = socket->Bind6();
    //     NS_LOG_DEBUG(this << " Bind6() return value= " << ret
    //                         << " GetErrNo= " << socket->GetErrno() << ".");

    //     Ipv6Address ipv6 = Ipv6Address::ConvertFrom(P2PNode);
    //     Inet6SocketAddress inet6Socket = Inet6SocketAddress(ipv6, m_remoteServerPort);
    //     NS_LOG_INFO(this << " PCDN " << " connecting to " << ipv6 << " port " << m_remoteServerPort << " / "
    //                         << inet6Socket << ".");
    //     ret = socket->Connect(inet6Socket);
    //     NS_LOG_DEBUG(this << " Connect() return value= " << ret
    //                         << " GetErrNo= " << socket->GetErrno() << ".");
    // }
    // else
    // {
    //     NS_LOG_INFO(this << " PCDN " << " ADDRESS FAIL!");
    // }

    NS_ASSERT_MSG(socket, "Failed creating socket.");

    // SwitchToStateClient(CLIENT_CONNECTING);
    m_P2PSocketInfo[socket] = PCDN_P2P_INFOS();
    socket->SetConnectCallback(
        MakeCallback(&VideoStreamPCDN::ClientConnectionSucceededCallback, this),
        MakeCallback(&VideoStreamPCDN::ClientConnectionFailedCallback, this));
    socket->SetCloseCallbacks(MakeCallback(&VideoStreamPCDN::ClientNormalCloseCallback, this),
                                MakeCallback(&VideoStreamPCDN::ClientErrorCloseCallback, this));
    socket->SetRecvCallback(MakeCallback(&VideoStreamPCDN::ClientReceivedDataCallback, this));
    socket->SetAttribute("MaxSegLifetime", DoubleValue(0.02)); // 20 ms.
    return socket;
    // } // end of `if (m_state == {NOT_STARTED, EXPECTING_EMBEDDED_OBJECT, PARSING_MAIN_OBJECT,
      // READING})`
    // else
    // {
    //     NS_FATAL_ERROR("Invalid state " << GetStateClientString() << " for OpenConnection().");
    // }
}
void
VideoStreamPCDN::ClientRequestMainObject(Ptr<Socket> socket, uint32_t videoNumber)
{
    NS_LOG_FUNCTION(this);
    // //请求100块就主动停止
    // if (m_chunknumber == 100)
    // {
    //     StopApplication();
    // }
    bool videoNak = false;
    if(socket == m_clientSocket)
    {
        if (m_cachedVideoNumber.find(videoNumber) == m_cachedVideoNumber.end())
        {
            if((int)m_cachedVideoNumber.size() >= m_maxVideoCacheNumber)
                videoNak = true;
            else
            {
                m_cachedVideoNumber.insert(videoNumber);
                m_cacheVideoBufferSize[videoNumber] = std::queue<std::pair<uint32_t, uint32_t>>();
            }

            
        }
        else if((int)m_cacheVideoBufferSize[videoNumber].size() >= m_maxChunkCacheNumber )
        {
            m_cacheVideoBufferSize[videoNumber].pop();
        }
    }
    
    
    // if (m_clientState == CONNECTING || m_clientState == READING)
    // {
    const uint32_t bitrateType = 0;
    VideoStreamHeader streamHeader;
    streamHeader.SetBitrateType(bitrateType);
    streamHeader.SetVideoChunkNumber(m_cacheVideoBufferSize[videoNumber].size() ?(m_cacheVideoBufferSize[videoNumber].back().first + 1) : 0 );
    streamHeader.SetVideoNumber(videoNumber);
    streamHeader.SetContentType(videoNak? VideoStreamHeader::VIDEO_NAK : VideoStreamHeader::VIDEO_CHUNK);
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
        // m_txMainObjectRequestTrace(packet);
        m_txTrace(packet);
        const int actualBytes = socket->Send(packet);
        NS_LOG_DEBUG(this << " Send() packet " << packet << " of " << packet->GetSize() << " bytes,"
                          << " return value= " << actualBytes << ".");
        if (actualBytes != static_cast<int>(packetSize))
        {
            NS_LOG_ERROR(this << " Failed to send request for embedded object,"
                              << " GetErrNo= " << socket->GetErrno() << ","
                              << " waiting for another Tx opportunity.");
        }
        else
        {
            // SwitchToState(EXPECTING_MAIN_OBJECT);
            m_P2PSocketInfo[socket].m_pageLoadStartTs = Simulator::Now(); // start counting page loading time
        }
    // }
    // else if (m_state == EXPECTING_MAIN_OBJECT)
    // {
    //     m_stopCounter++;

    // }
    // else
    // {
    //     NS_FATAL_ERROR("Invalid state " << GetStateString() << " for RequestMainObject().");
    // }
}

void
VideoStreamPCDN::ClientReceiveMainObject(Ptr<Packet> packet, const Address& from, Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << packet << from);

    // if (m_state == EXPECTING_MAIN_OBJECT)
    // {
        /*
         * In the following call to Receive(), #m_objectBytesToBeReceived *will*
         * be updated. #m_objectClientTs and #m_objectServerTs *may* be updated.
         * ThreeGppHttpHeader will be removed from the packet, if it is the first
         * packet of the object to be received; the header will be available in
         * #m_constructedPacketHeader.
         * #m_constructedPacket will also be updated.
         */
    // NS_LOG_INFO(this << "PCDN " << " THIS IS FIRST PACKET FROM" << InetSocketAddress::ConvertFrom(from).GetIpv4() << ":" << InetSocketAddress::ConvertFrom(from).GetPort() );
    
    ClientReceive(socket, packet);
    // m_rxMainObjectPacketTrace(packet);

    if (m_P2PSocketInfo[socket].m_objectBytesToBeReceived > 0)
    {
        /*
            * There are more packets of this main object, so just stay still
            * and wait until they arrive.
            */
        NS_LOG_INFO(this << " PCDN " << " " << m_P2PSocketInfo[socket].m_objectBytesToBeReceived << " byte(s)"
                            << " remains from this chunk of main object.");
    }
    else
    {
        /*
            * This is the last packet of this main object. Acknowledge the
            * reception of a whole main object
            */
        NS_LOG_INFO(this << " PCDN " << " Finished receiving a main object.");
        // m_rxMainObjectTrace(this, m_constructedPacket);

        if (!m_P2PSocketInfo[socket].m_objectServerTs.IsZero())
        {
            m_rxDelayTrace(Simulator::Now() - m_P2PSocketInfo[socket].m_objectServerTs, from);
            m_P2PSocketInfo[socket].m_objectServerTs = MilliSeconds(0); // Reset back to zero.
        }

        if (!m_P2PSocketInfo[socket].m_objectClientTs.IsZero())
        {
            // m_rxRttTrace(Simulator::Now() - m_objectClientTs, from);
            m_P2PSocketInfo[socket].m_objectClientTs = MilliSeconds(0); // Reset back to zero.
        }

        ClientEnterParsingTime(socket);

    } // end of else of `if (m_objectBytesToBeReceived > 0)`

    // } // end of `if (m_state == EXPECTING_MAIN_OBJECT)`
    // else
    // {
    //     NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ReceiveMainObject().");
    // }
}

void
VideoStreamPCDN::ClientReceive(Ptr<Socket> socket, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    /* In a "real" HTTP message the message size is coded differently. The use of a header
     * is to avoid the burden of doing a real message parser.
     */
    bool firstPacket = false;
    if(m_P2PSocketInfo.find(socket) == m_P2PSocketInfo.end())
    {
        NS_FATAL_ERROR("MISSING SOCKET!");
    }
    if (m_P2PSocketInfo[socket].m_objectBytesToBeReceived == 0)
    {
        // This is the first packet of the object.
        firstPacket = true;

        // Remove the header in order to calculate remaining data to be received.
        ThreeGppHttpHeader httpHeader;
        packet->RemoveHeader(httpHeader);

        m_P2PSocketInfo[socket].m_objectBytesToBeReceived = httpHeader.GetContentLength();
        m_P2PSocketInfo[socket].m_objectClientTs = httpHeader.GetClientTs();
        m_P2PSocketInfo[socket].m_objectServerTs = httpHeader.GetServerTs();

        // Take a copy for constructed packet trace. Note that header is included.
        m_P2PSocketInfo[socket].m_constructedPacket = packet->Copy();
        m_P2PSocketInfo[socket].m_constructedPacket->AddHeader(httpHeader);
        NS_LOG_INFO(this << " PCDN " << "GET FIRST PACKET with header size" << m_P2PSocketInfo[socket].m_objectBytesToBeReceived);

    }
    uint32_t contentSize = packet->GetSize();
    m_P2PSocketInfo[socket].m_numberBytesPage += contentSize; // increment counter of page size

    /* Note that the packet does not contain header at this point.
     * The content is purely raw data, which was the only intended data to be received.
     */
    if (m_P2PSocketInfo[socket].m_objectBytesToBeReceived < contentSize)
    {
        NS_LOG_INFO(this << " PCDN " << " ENTER CORRUPTED PACKET !");
        NS_LOG_WARN(this << " The received packet"
                         << " (" << contentSize << " bytes of content)"
                         << " is larger than"
                         << " the content that we expected to receive"
                         << " (" << m_P2PSocketInfo[socket].m_objectBytesToBeReceived << " bytes).");
        // Stop expecting any more packet of this object.
        m_P2PSocketInfo[socket].m_objectBytesToBeReceived = 0;
        m_P2PSocketInfo[socket].m_constructedPacket = nullptr;
    }
    else
    {
        m_P2PSocketInfo[socket].m_objectBytesToBeReceived -= contentSize;
        if (!firstPacket)
        {
            Ptr<Packet> packetCopy = packet->Copy();
            m_P2PSocketInfo[socket].m_constructedPacket->AddAtEnd(packetCopy);
        }
    }
}

void
VideoStreamPCDN::ClientEnterParsingTime(Ptr<Socket> socket)
{
     NS_LOG_FUNCTION(this);
    // const Time parsingTime = m_httpVariables->GetParsingTime();
    //TODO parsing or peer to be changed
    const Time parsingTime = MilliSeconds(0);
    NS_LOG_INFO(this << " PCDN " << " The parsing of this main object"
                        << " will complete in " << parsingTime.As(Time::S) << ".");
    


    m_P2PSocketInfo[socket].m_eventParseMainObject= Simulator::Schedule(parsingTime, &VideoStreamPCDN::ClientParseMainObject, this, socket);
    // SwitchToState(PARSING_MAIN_OBJECT);
}

void
VideoStreamPCDN::ClientParseMainObject(Ptr<Socket> socket)
{
     NS_LOG_FUNCTION(this);

    // if (m_state == PARSING_MAIN_OBJECT)
    // {
        m_P2PSocketInfo[socket].m_embeddedObjectsToBeRequested = 0;
        // m_embeddedObjectsToBeRequested = m_httpVariables->GetNumOfEmbeddedObjects();
        // saving total number of embedded objects
        m_P2PSocketInfo[socket].m_numberEmbeddedObjectsRequested = m_P2PSocketInfo[socket].m_embeddedObjectsToBeRequested;
        NS_LOG_INFO(this << " PCDN " << " Parsing has determined " << m_P2PSocketInfo[socket].m_embeddedObjectsToBeRequested
                         << " embedded object(s) in the main object.");

        // if (m_embeddedObjectsToBeRequested > 0)
        // {
        //     /*
        //      * Immediately request the first embedded object using the
        //      * existing connection.
        //      */
        //     // m_eventRequestEmbeddedObject =
        //     //     Simulator::ScheduleNow(&VideoStreamClient::RequestEmbeddedObject, this);
        // }
        // else
        // {
            /*
             * There is no embedded object in the main object. So sit back and
             * enjoy the plain web page.
             */
        ThreeGppHttpHeader header;
        VideoStreamHeader vHeader;
        if(m_P2PSocketInfo[socket].m_constructedPacket == nullptr)
        {
            NS_FATAL_ERROR(this << " PCDN  EMPTY PACKET!");
        }
        m_P2PSocketInfo[socket].m_constructedPacket->RemoveHeader(header);
        m_P2PSocketInfo[socket].m_constructedPacket->RemoveHeader(vHeader);
        if(vHeader.GetContentType() == VideoStreamHeader::VIDEO_PEER)
        {
            NS_LOG_INFO(this << " PCDN " << " get a peer");
            Address peerInfo;
            uint32_t objectSize =  m_P2PSocketInfo[socket].m_constructedPacket->GetSize();
            uint8_t *buffer = new uint8_t[objectSize];
            m_P2PSocketInfo[socket].m_constructedPacket->CopyData(buffer, objectSize);
            TagBuffer tbuffer(buffer, buffer + objectSize);
            peerInfo.Deserialize(tbuffer);
            NS_LOG_INFO(this << " PCDN " << " deserialized peer is " << peerInfo << " " << (int)peerInfo.GetLength());
            // peerInfo.Deserialize()
            // m_constructedPacket->
            // TODO peer logic
            Ptr<Socket> peerSocket = P2POpenConnection(peerInfo);
            // m_peerSocket[vHeader.GetVideoNumber()] = peerSocket;
            m_peerSocketSet.insert(peerSocket);
            Simulator::ScheduleNow(&VideoStreamPCDN::ClientRequestMainObject, this, peerSocket,vHeader.GetVideoNumber());
            return;
        }
        else
        {
            uint32_t videonumber = vHeader.GetVideoNumber();
            uint32_t chunksize = vHeader.GetVideoChunkSize();
            uint32_t videoChunkNumber = vHeader.GetVideoChunkNumber();
            if(m_cachedVideoNumber.find(videonumber) == m_cachedVideoNumber.end())
            {
                NS_LOG_INFO(this << " PCDN " << " video number " << videonumber << " not inside the cached buffer!");

                // return;
            }
            else
            {
                NS_LOG_INFO(this << " PCDN " << " video number " << videonumber << " inside the cached buffer!"); 
                std::queue<std::pair<uint32_t, uint32_t>> &bufferQueue = m_cacheVideoBufferSize[videonumber];
                bufferQueue.push(std::make_pair(videoChunkNumber, chunksize));
                NS_LOG_INFO(this << " PCDN " << " recieve (video number"<< videonumber << " chunk number " <<  videoChunkNumber << " with size " << chunksize);
                NS_LOG_INFO(this << " PCDN " << " header info is " << vHeader.ToString());
                NS_LOG_INFO(this << " PCDN " << " Finished receiving a web page.");
                const uint32_t m_minQueueSize = 3;
               
                if(bufferQueue.size() < m_minQueueSize)
                {
                    Simulator::ScheduleNow(&VideoStreamPCDN::ClientRequestMainObject, this, socket, videonumber);
                }
                else
                {
                    m_P2PSocketInfo[socket].m_eventRequestMainObject = Simulator::Schedule(Seconds(1), &VideoStreamPCDN::ClientRequestMainObject, this, socket, videonumber);

                }
            }
            if(m_forwardTable.find(videonumber) != m_forwardTable.end())
            {
                auto it = m_forwardTable.find(videonumber);
                for(auto i : it->second)
                {
                    Simulator::ScheduleNow(&VideoStreamPCDN::ServeNewMainObject, this, i, videonumber);
                }
                m_forwardTable.erase(it);
            
            }
            //至少三秒的buffer后启动
            // if(bufferQueue.size() >= 3)
                // m_eventDisplayVideoChunk = Simulator::Schedule(Seconds(1.0), &VideoStreamClient::DisplayVideoChunk, this);
            
        }
        
        // FinishReceivingPage(); // trigger callback for page loading time
        // EnterReadingTime();
        // }
    // }
    // else
    // {
    //     NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ParseMainObject().");
    // }
}

void
VideoStreamPCDN::ClientCancelAllPendingEvents(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);

    if (!Simulator::IsExpired(m_P2PSocketInfo[socket].m_eventRequestMainObject))
    {
        NS_LOG_INFO(this << " PCDN " << " Canceling RequestMainObject() which is due in "
                         << Simulator::GetDelayLeft(m_P2PSocketInfo[socket].m_eventRequestMainObject).As(Time::S) << ".");
        Simulator::Cancel(m_P2PSocketInfo[socket].m_eventRequestMainObject);
    }

    // if (!Simulator::IsExpired(m_eventRequestEmbeddedObject))
    // {
    //     NS_LOG_INFO(this << " PCDN " << " Canceling RequestEmbeddedObject() which is due in "
    //                      << Simulator::GetDelayLeft(m_eventRequestEmbeddedObject).As(Time::S)
    //                      << ".");
    //     Simulator::Cancel(m_eventRequestEmbeddedObject);
    // }

    if (!Simulator::IsExpired(m_P2PSocketInfo[socket].m_eventParseMainObject))
    {
        NS_LOG_INFO(this << " PCDN " << " Canceling ParseMainObject() which is due in "
                         << Simulator::GetDelayLeft(m_P2PSocketInfo[socket].m_eventParseMainObject).As(Time::S) << ".");
        Simulator::Cancel(m_P2PSocketInfo[socket].m_eventParseMainObject);
    }

    // if (!Simulator::IsExpired(m_eventDisplayVideoChunk))
    // {
    //     NS_LOG_INFO(this << " PCDN " << " Canceling DisplayVideoChunk() which is due in "
    //                      << Simulator::GetDelayLeft(m_eventDisplayVideoChunk).As(Time::S) << ".");
    //     Simulator::Cancel(m_eventDisplayVideoChunk);
    // }
}

// HTTP SERVER TX BUFFER //////////////////////////////////////////////////////

VideoStreamPCDNTxBuffer::VideoStreamPCDNTxBuffer()
{
    NS_LOG_FUNCTION(this);
}

bool
VideoStreamPCDNTxBuffer::IsSocketAvailable(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    return (it != m_txBuffer.end());
}

void
VideoStreamPCDNTxBuffer::AddSocket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    NS_ASSERT_MSG(!IsSocketAvailable(socket),
                  this << " Cannot add socket " << socket
                       << " because it has already been added before.");

    TxBuffer_t txBuffer;
    txBuffer.txBufferContentType = ThreeGppHttpHeader::NOT_SET;
    txBuffer.txBufferSize = 0;
    txBuffer.isClosing = false;
    txBuffer.hasTxedPartOfObject = false;
    m_txBuffer.insert(std::pair<Ptr<Socket>, TxBuffer_t>(socket, txBuffer));
}

void
VideoStreamPCDNTxBuffer::RemoveSocket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");

    if (!Simulator::IsExpired(it->second.nextServe))
    {
        NS_LOG_INFO(this << " PCDN " << " Canceling a serving event which is due in "
                         << Simulator::GetDelayLeft(it->second.nextServe).As(Time::S) << ".");
        Simulator::Cancel(it->second.nextServe);
    }

    it->first->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                 MakeNullCallback<void, Ptr<Socket>>());
    it->first->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    it->first->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());

    m_txBuffer.erase(it);
}

void
VideoStreamPCDNTxBuffer::CloseSocket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");

    if (!Simulator::IsExpired(it->second.nextServe))
    {
        NS_LOG_INFO(this << " PCDN " << " Canceling a serving event which is due in "
                         << Simulator::GetDelayLeft(it->second.nextServe).As(Time::S) << ".");
        Simulator::Cancel(it->second.nextServe);
    }

    if (it->second.txBufferSize > 0)
    {
        NS_LOG_WARN(this << " Closing a socket where " << it->second.txBufferSize
                         << " bytes of transmission"
                         << " is still pending in the corresponding Tx buffer.");
    }

    it->first->Close();
    it->first->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                 MakeNullCallback<void, Ptr<Socket>>());
    it->first->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    it->first->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());

    m_txBuffer.erase(it);
}

void
VideoStreamPCDNTxBuffer::CloseAllSockets()
{
    NS_LOG_FUNCTION(this);

    for (auto it = m_txBuffer.begin(); it != m_txBuffer.end(); ++it)
    {
        if (!Simulator::IsExpired(it->second.nextServe))
        {
            NS_LOG_INFO(this << " PCDN " << " Canceling a serving event which is due in "
                             << Simulator::GetDelayLeft(it->second.nextServe).As(Time::S) << ".");
            Simulator::Cancel(it->second.nextServe);
        }

        it->first->Close();
        it->first->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                     MakeNullCallback<void, Ptr<Socket>>());
        it->first->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        it->first->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());
    }

    m_txBuffer.clear();
}

bool
VideoStreamPCDNTxBuffer::IsBufferEmpty(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    return (it->second.txBufferSize == 0);
}

Time
VideoStreamPCDNTxBuffer::GetClientTs(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    return it->second.clientTs;
}

ThreeGppHttpHeader::ContentType_t
VideoStreamPCDNTxBuffer::GetBufferContentType(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    return it->second.txBufferContentType;
}

uint32_t
VideoStreamPCDNTxBuffer::GetBufferSize(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    return it->second.txBufferSize;
}

bool
VideoStreamPCDNTxBuffer::HasTxedPartOfObject(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found");
    return it->second.hasTxedPartOfObject;
}

void
VideoStreamPCDNTxBuffer::WriteNewObject(Ptr<Socket> socket,
                                       ThreeGppHttpHeader::ContentType_t contentType,
                                       uint32_t objectSize)
{
    NS_LOG_FUNCTION(this << socket << contentType << objectSize);

    NS_ASSERT_MSG(contentType != ThreeGppHttpHeader::NOT_SET,
                  "Unable to write an object without a proper Content-Type.");
    NS_ASSERT_MSG(objectSize > 0, "Unable to write a zero-sized object.");

    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    NS_ASSERT_MSG(it->second.txBufferSize == 0,
                  "Cannot write to Tx buffer of socket "
                      << socket << " until the previous content has been completely sent.");
    it->second.txBufferContentType = contentType;
    it->second.txBufferSize = objectSize;
    it->second.hasTxedPartOfObject = false;
}

void
VideoStreamPCDNTxBuffer::RecordNextServe(Ptr<Socket> socket,
                                        const EventId& eventId,
                                        const Time& clientTs)
{
    NS_LOG_FUNCTION(this << socket << clientTs.As(Time::S));

    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    it->second.nextServe = eventId;
    it->second.clientTs = clientTs;
}

void
VideoStreamPCDNTxBuffer::DepleteBufferSize(Ptr<Socket> socket, uint32_t amount)
{
    NS_LOG_FUNCTION(this << socket << amount);

    NS_ASSERT_MSG(amount > 0, "Unable to consume zero bytes.");

    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    NS_ASSERT_MSG(it->second.txBufferSize >= amount,
                  "The requested amount is larger than the current buffer size.");
    it->second.txBufferSize -= amount;
    it->second.hasTxedPartOfObject = true;

    if (it->second.isClosing && (it->second.txBufferSize == 0))
    {
        /*
         * The peer has earlier issued a close request and we have now waited
         * until all the existing data are pushed into the socket. Now we close
         * the socket explicitly.
         */
        CloseSocket(socket);
    }
}

void
VideoStreamPCDNTxBuffer::PrepareClose(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    it->second.isClosing = true;
}

} // namespace ns3
