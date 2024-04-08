/**
 * Author: Yinghao Yang <blacktonyrl@gmail.com>
 * 
*/

// #include "three-gpp-http-server.h"
#include "VideoStreamServerNew.h"
#include "three-gpp-http-variables.h"

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

NS_LOG_COMPONENT_DEFINE("VideoStreamServerNew");

namespace ns3
{

// HTTP SERVER ////////////////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(VideoStreamServerNew);

VideoStreamServerNew::VideoStreamServerNew()
    : m_state(NOT_STARTED),
      m_initialSocket(nullptr),
      m_txBuffer(Create<VideoStreamServerTxBuffer>()),
      m_httpVariables(CreateObject<ThreeGppHttpVariables>())
{
    NS_LOG_FUNCTION(this);
    m_bitrateSet[0] = 350;
    m_bitrateSet[1] = 600;
    m_bitrateSet[2] = 1000;
    m_bitrateSet[3] = 2000;
    m_bitrateSet[4] = 3000;

    m_videoChunkTotalNumber[0] = 5;
    m_videoChunkTotalNumber[1] = 10;
    m_usingP2P = 0;
    m_mtuSize = m_httpVariables->GetMtuSize();
    m_mtuSize = 1460;

    NS_LOG_INFO(this << " Server " << " MTU size for this server application is " << m_mtuSize << " bytes.");
}

// static
TypeId
VideoStreamServerNew::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::VideoStreamServerNew")
            .SetParent<Application>()
            .AddConstructor<VideoStreamServerNew>()
            .AddAttribute("Variables",
                          "Variable collection, which is used to control e.g. processing and "
                          "object generation delays.",
                          PointerValue(),
                          MakePointerAccessor(&VideoStreamServerNew::m_httpVariables),
                          MakePointerChecker<ThreeGppHttpVariables>())
            .AddAttribute("LocalAddress",
                          "The local address of the server, "
                          "i.e., the address on which to bind the Rx socket.",
                          AddressValue(),
                          MakeAddressAccessor(&VideoStreamServerNew::m_localAddress),
                          MakeAddressChecker())
            .AddAttribute("LocalPort",
                          "Port on which the application listen for incoming packets.",
                          UintegerValue(80), // the default HTTP port
                          MakeUintegerAccessor(&VideoStreamServerNew::m_localPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Mtu",
                          "Maximum transmission unit (in bytes) of the TCP sockets "
                          "used in this application, excluding the compulsory 40 "
                          "bytes TCP header. Typical values are 1460 and 536 bytes. "
                          "The attribute is read-only because the value is randomly "
                          "determined.",
                          TypeId::ATTR_GET,
                          UintegerValue(),
                          MakeUintegerAccessor(&VideoStreamServerNew::m_mtuSize),
                          MakeUintegerChecker<uint32_t>())
            .AddTraceSource(
                "ConnectionEstablished",
                "Connection to a remote web client has been established.",
                MakeTraceSourceAccessor(&VideoStreamServerNew::m_connectionEstablishedTrace),
                "ns3::HttpServer::ConnectionEstablishedCallback")
            .AddTraceSource("MainObject",
                            "A main object has been generated.",
                            MakeTraceSourceAccessor(&VideoStreamServerNew::m_mainObjectTrace),
                            "ns3::HttpServer::HttpObjectCallback")
            .AddTraceSource("EmbeddedObject",
                            "An embedded object has been generated.",
                            MakeTraceSourceAccessor(&VideoStreamServerNew::m_embeddedObjectTrace),
                            "ns3::HttpServer::HttpObjectCallback")
            .AddTraceSource("Tx",
                            "A packet has been sent.",
                            MakeTraceSourceAccessor(&VideoStreamServerNew::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("Rx",
                            "A packet has been received.",
                            MakeTraceSourceAccessor(&VideoStreamServerNew::m_rxTrace),
                            "ns3::Packet::PacketAddressTracedCallback")
            .AddTraceSource("RxDelay",
                            "A packet has been received with delay information.",
                            MakeTraceSourceAccessor(&VideoStreamServerNew::m_rxDelayTrace),
                            "ns3::Application::DelayAddressCallback")
            .AddTraceSource("StateTransition",
                            "Trace fired upon every HTTP client state transition.",
                            MakeTraceSourceAccessor(&VideoStreamServerNew::m_stateTransitionTrace),
                            "ns3::Application::StateTransitionCallback");
    return tid;
}

void
VideoStreamServerNew::SetMtuSize(uint32_t mtuSize)
{
    NS_LOG_FUNCTION(this << mtuSize);
    m_mtuSize = mtuSize;
}

Ptr<Socket>
VideoStreamServerNew::GetSocket() const
{
    return m_initialSocket;
}

VideoStreamServerNew::State_t
VideoStreamServerNew::GetState() const
{
    return m_state;
}

std::string
VideoStreamServerNew::GetStateString() const
{
    return GetStateString(m_state);
}

// static
std::string
VideoStreamServerNew::GetStateString(VideoStreamServerNew::State_t state)
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

void
VideoStreamServerNew::DoDispose()
{
    NS_LOG_FUNCTION(this);

    if (!Simulator::IsFinished())
    {
        StopApplication();
    }

    Application::DoDispose(); // Chain up.
}

void
VideoStreamServerNew::StartApplication()
{
    NS_LOG_FUNCTION(this);

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
                NS_LOG_INFO(this << " Server " << " Binding on " << ipv4 << " port " << m_localPort << " / "
                                 << inetSocket << ".");
                int ret [[maybe_unused]] = m_initialSocket->Bind(inetSocket);
                NS_LOG_DEBUG(this << " Bind() return value= " << ret
                                  << " GetErrNo= " << m_initialSocket->GetErrno() << ".");
            }
            else if (Ipv6Address::IsMatchingType(m_localAddress))
            {
                const Ipv6Address ipv6 = Ipv6Address::ConvertFrom(m_localAddress);
                const Inet6SocketAddress inet6Socket = Inet6SocketAddress(ipv6, m_localPort);
                NS_LOG_INFO(this << " Server " << " Binding on " << ipv6 << " port " << m_localPort << " / "
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
            MakeCallback(&VideoStreamServerNew::ConnectionRequestCallback, this),
            MakeCallback(&VideoStreamServerNew::NewConnectionCreatedCallback, this));
        m_initialSocket->SetCloseCallbacks(
            MakeCallback(&VideoStreamServerNew::NormalCloseCallback, this),
            MakeCallback(&VideoStreamServerNew::ErrorCloseCallback, this));
        m_initialSocket->SetRecvCallback(
            MakeCallback(&VideoStreamServerNew::ReceivedDataCallback, this));
        m_initialSocket->SetSendCallback(MakeCallback(&VideoStreamServerNew::SendCallback, this));
        SwitchToState(STARTED);

    } // end of `if (m_state == NOT_STARTED)`
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for StartApplication().");
    }

} // end of `void StartApplication ()`

void
VideoStreamServerNew::StopApplication()
{
    NS_LOG_FUNCTION(this);

    SwitchToState(STOPPED);

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

}

bool
VideoStreamServerNew::ConnectionRequestCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this << socket << address);
    return true; // Unconditionally accept the connection request.
}

void
VideoStreamServerNew::NewConnectionCreatedCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this << socket << address);

    socket->SetCloseCallbacks(MakeCallback(&VideoStreamServerNew::NormalCloseCallback, this),
                              MakeCallback(&VideoStreamServerNew::ErrorCloseCallback, this));
    socket->SetRecvCallback(MakeCallback(&VideoStreamServerNew::ReceivedDataCallback, this));
    socket->SetSendCallback(MakeCallback(&VideoStreamServerNew::SendCallback, this));

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
VideoStreamServerNew::NormalCloseCallback(Ptr<Socket> socket)
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
VideoStreamServerNew::ErrorCloseCallback(Ptr<Socket> socket)
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
VideoStreamServerNew::ReceivedDataCallback(Ptr<Socket> socket)
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
        //     NS_LOG_INFO(this << " Server " << " A packet of " << packet->GetSize() << " bytes"
        //                      << " received from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
        //                      << " port " << InetSocketAddress::ConvertFrom(from).GetPort() << " / "
        //                      << InetSocketAddress::ConvertFrom(from));
        // }
        // else if (Inet6SocketAddress::IsMatchingType(from))
        // {
        //     NS_LOG_INFO(this << " Server " << " A packet of " << packet->GetSize() << " bytes"
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
        NS_LOG_INFO(this << " Server " << "recieve streaming request" << vHeader.ToString());
        packet->AddHeader(httpHeader);
        // Fire trace sources.
        m_rxTrace(packet, from);
        m_rxDelayTrace(Simulator::Now() - httpHeader.GetClientTs(), from);

        Time processingDelay;
        switch (httpHeader.GetContentType())
        {
        case ThreeGppHttpHeader::MAIN_OBJECT:
            processingDelay = m_httpVariables->GetMainObjectGenerationDelay();
            NS_LOG_INFO(this << " Server " << " Will finish generating a main object"
                             << " in " << processingDelay.As(Time::S) << ".");
            
            if (InetSocketAddress::IsMatchingType(from))
            {
                // InetSocketAddress test = InetSocketAddress::ConvertFrom(from);
                // NS_LOG_INFO(this << " Server " << " A packet of " << packet->GetSize() << " bytes"
                //                 << " received from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
                //                 << " port " << InetSocketAddress::ConvertFrom(from).GetPort() << " / "
                //                 << InetSocketAddress::ConvertFrom(from));
                m_txBuffer->RecordNextServe(socket,
                                        Simulator::Schedule(processingDelay,
                                                            &VideoStreamServerNew::ServeNewMainObject,
                                                            this,
                                                            socket, from, vHeader.GetContentType() == VideoStreamHeader::VIDEO_NAK),
                                        httpHeader.GetClientTs());
            }
            else if (Inet6SocketAddress::IsMatchingType(from))
            {
                // Inet6SocketAddress test = Inet6SocketAddress::ConvertFrom(from);

                // NS_LOG_INFO(this << " Server " << " A packet of " << packet->GetSize() << " bytes"
                //                 << " received from " << Inet6SocketAddress::ConvertFrom(from).GetIpv6()
                //                 << " port " << Inet6SocketAddress::ConvertFrom(from).GetPort() << " / "
                //                 << Inet6SocketAddress::ConvertFrom(from));
                m_txBuffer->RecordNextServe(socket,
                                        Simulator::Schedule(processingDelay,
                                                            &VideoStreamServerNew::ServeNewMainObject,
                                                            this,
                                                            socket, from, vHeader.GetContentType() == VideoStreamHeader::VIDEO_NAK),
                                        httpHeader.GetClientTs());
            }
            // m_txBuffer->RecordNextServe(socket,
            //                             Simulator::Schedule(processingDelay,
            //                                                 &VideoStreamServerNew::ServeNewMainObject,
            //                                                 this,
            //                                                 socket, from),
            //                             httpHeader.GetClientTs());
            // InetSocketAddress test;
            // test.GetIpv4().
            break;

        case ThreeGppHttpHeader::EMBEDDED_OBJECT:
            processingDelay = m_httpVariables->GetEmbeddedObjectGenerationDelay();
            NS_LOG_INFO(this << " Server " << " Will finish generating an embedded object"
                             << " in " << processingDelay.As(Time::S) << ".");
            m_txBuffer->RecordNextServe(
                socket,
                Simulator::Schedule(processingDelay,
                                    &VideoStreamServerNew::ServeNewEmbeddedObject,
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
VideoStreamServerNew::SendCallback(Ptr<Socket> socket, uint32_t availableBufferSize)
{
    NS_LOG_FUNCTION(this << socket << availableBufferSize);

    if (!m_txBuffer->IsBufferEmpty(socket))
    {
        const uint32_t txBufferSize [[maybe_unused]] = m_txBuffer->GetBufferSize(socket);
        const uint32_t actualSent [[maybe_unused]] = ServeFromTxBuffer(socket);

#ifdef NS3_LOG_ENABLE
        // Some log messages.
        // if (actualSent < txBufferSize)
        // {
        //     switch (m_txBuffer->GetBufferContentType(socket))
        //     {
        //     case ThreeGppHttpHeader::MAIN_OBJECT:
        //         NS_LOG_INFO(this << " Server " << " Transmission of main object is suspended"
        //                          << " after " << actualSent << " bytes.");
        //         break;
        //     case ThreeGppHttpHeader::EMBEDDED_OBJECT:
        //         NS_LOG_INFO(this << " Server " << " Transmission of embedded object is suspended"
        //                          << " after " << actualSent << " bytes.");
        //         break;
        //     default:
        //         NS_FATAL_ERROR("Invalid Tx buffer content type.");
        //         break;
        //     }
        // }
        // else
        // {
        //     switch (m_txBuffer->GetBufferContentType(socket))
        //     {
        //     case ThreeGppHttpHeader::MAIN_OBJECT:
        //         NS_LOG_INFO(this << " Server " << " Finished sending a whole main object.");
        //         break;
        //     case ThreeGppHttpHeader::EMBEDDED_OBJECT:
        //         NS_LOG_INFO(this << " Server " << " Finished sending a whole embedded object.");
        //         break;
        //     default:
        //         NS_FATAL_ERROR("Invalid Tx buffer content type.");
        //         break;
        //     }
        // }
#endif /* NS3_LOG_ENABLE */

    } // end of `if (m_txBuffer->IsBufferEmpty (socket))`

} // end of `void SendCallback (Ptr<Socket> socket, uint32_t availableBufferSize)`

void
VideoStreamServerNew::SetUsingPCDN(bool isUsePCDN)
{
    if(isUsePCDN)
        m_usingP2P = 1;
    else
        m_usingP2P = 0;
    return;
}

void
VideoStreamServerNew::ServeNewMainObject(Ptr<Socket> socket, Address from, bool videoNak)
{
    NS_LOG_FUNCTION(this << socket);

    // const uint32_t objectSize = m_httpVariables->GetMainObjectSize();
    uint32_t objectSize = m_bitrateSet[m_bitrateType];
    objectSize = (uint32_t) objectSize * 4 * 1000 / 8;
    bool findPeer = false;
    auto it = m_peerInfo.find(m_videoRequested);

    if(it != m_peerInfo.end() && it->second != from)
    {
        objectSize = m_peerInfo[m_videoRequested].GetSerializedSize();
        findPeer = true;
    }
    NS_LOG_INFO(this << " Server " << " Main object to be served is " << objectSize << " bytes." << " Targeting is " << InetSocketAddress::ConvertFrom(from).GetIpv4() << ":" << InetSocketAddress::ConvertFrom(from).GetPort());

    m_mainObjectTrace(objectSize);
    if(!m_txBuffer->IsBufferEmpty(socket))
    {
        Simulator::Schedule(MilliSeconds(20), &VideoStreamServerNew::ServeNewMainObject, this, socket, from, videoNak);
        return;
    }

    m_txBuffer->WriteNewObject(socket, ThreeGppHttpHeader::MAIN_OBJECT, objectSize);
    uint32_t actualSent;
    if(m_usingP2P)
    {
        if(findPeer)
        {
            uint8_t *buffer = new uint8_t[objectSize];
            TagBuffer tbuffer(buffer, buffer + objectSize);
            m_peerInfo[m_videoRequested].Serialize(tbuffer);
            Ptr<Packet> packet = Create<Packet>(buffer, objectSize);
            // Tag
            // packet->AddPacketTag()
            delete[] buffer;
            actualSent = ServeFromTxBuffer(socket, packet);

        }
        else if(it != m_peerInfo.end() && it->second == from)
        {
            NS_LOG_INFO(this << " Server " << " recieve recorded peer " << from << " requesting" );
            actualSent = ServeFromTxBuffer(socket);

        }
        else
        {
            if(!videoNak)
            {
                NS_LOG_INFO(this << " Server " << " add peer " << from << " with video number " << m_videoRequested);
                m_peerInfo[m_videoRequested] = from;
            }
            else
                NS_LOG_INFO(this << " Server " << " get requet from " << from << " with video number " << m_videoRequested << " and video Nak Tag");


            actualSent = ServeFromTxBuffer(socket);
        }
    }
    else
    {
        actualSent = ServeFromTxBuffer(socket);
    }

    if (actualSent < objectSize)
    {
        NS_LOG_INFO(this << " Server " << " Transmission of main object is suspended"
                         << " after " << actualSent << " bytes.");
    }
    else
    {
        NS_LOG_INFO(this << " Server " << " Finished sending a whole main object.");
    }
}

void
VideoStreamServerNew::ServeNewEmbeddedObject(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    const uint32_t objectSize = m_httpVariables->GetEmbeddedObjectSize();
    NS_LOG_INFO(this << " Server " << " Embedded object to be served is " << objectSize << " bytes.");
    m_embeddedObjectTrace(objectSize);
    m_txBuffer->WriteNewObject(socket, ThreeGppHttpHeader::EMBEDDED_OBJECT, objectSize);
    const uint32_t actualSent = ServeFromTxBuffer(socket);

    if (actualSent < objectSize)
    {
        NS_LOG_INFO(this << " Server " << " Transmission of embedded object is suspended"
                         << " after " << actualSent << " bytes.");
    }
    else
    {
        NS_LOG_INFO(this << " Server " << " Finished sending a whole embedded object.");
    }
}

uint32_t
VideoStreamServerNew::ServeFromTxBuffer(Ptr<Socket> socket)
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
        vHeader.SetVideoNumber(m_videoRequested);
        vHeader.SetBitrateType(m_bitrateType);
        vHeader.SetVideoChunkNumber(m_VideoChunkRequested);
        const uint32_t videochunksize = 123456;
        const uint32_t videototalNumber = 666666;
        vHeader.setVideoChunkSize(videochunksize);
        vHeader.SetVideoChunkTotalNumber(videototalNumber);
        vHeader.SetContentType(VideoStreamHeader::VIDEO_CHUNK);

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

        NS_LOG_INFO(this << " Server " << " Created packet " << packet << " of " << packetSize << " bytes."
                         << " The corresponding request came "
                         << (Simulator::Now() - httpHeader.GetClientTs()).As(Time::S) << " ago.");
    }
    else
    {
        // NS_LOG_INFO(this << " Server " << " Created packet " << packet << " of " << packetSize
        //                  << " bytes to be appended to a previous packet.");
    }
    // packet->AddByteTag()
    // Send.
    const int actualBytes = socket->Send(packet);
    NS_LOG_DEBUG(this << " Send() packet " << packet << " of " << packetSize << " bytes,"
                      << " return value= " << actualBytes << ".");
    m_txTrace(packet);

    if (actualBytes == static_cast<int>(packetSize))
    {
        // The packet goes through successfully.
        m_txBuffer->DepleteBufferSize(socket, contentSize);
        // NS_LOG_INFO(this << " Server " << " Remaining object to be sent " << m_txBuffer->GetBufferSize(socket)
        //                  << " bytes.");
        return packetSize;
    }
    else
    {
        NS_LOG_INFO(this << " Server " << " Failed to send object,"
                         << " GetErrNo= " << socket->GetErrno() << ","
                         << " suspending transmission"
                         << " and waiting for another Tx opportunity.");
        return 0;
    }

} // end of `uint32_t ServeFromTxBuffer (Ptr<Socket> socket)`
uint32_t
VideoStreamServerNew::ServeFromTxBuffer(Ptr<Socket> socket, Ptr<Packet> packet)
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
    uint32_t contentSize = packet->GetSize();
    // Ptr<Packet> packet = Create<Packet>(contentSize);
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
        vHeader.SetVideoNumber(m_videoRequested);
        vHeader.SetBitrateType(m_bitrateType);
        vHeader.SetVideoChunkNumber(m_VideoChunkRequested);
        const uint32_t videochunksize = 123456;
        const uint32_t videototalNumber = 666666;
        vHeader.setVideoChunkSize(videochunksize);
        vHeader.SetVideoChunkTotalNumber(videototalNumber);
        vHeader.SetContentType(VideoStreamHeader::VIDEO_PEER);
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

        NS_LOG_INFO(this << " Server " << " Created packet " << packet << " of " << packetSize << " bytes."
                         << " The corresponding request came "
                         << (Simulator::Now() - httpHeader.GetClientTs()).As(Time::S) << " ago.");
    }
    else
    {
        // NS_LOG_INFO(this << " Server " << " Created packet " << packet << " of " << packetSize
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
        // NS_LOG_INFO(this << " Server " << " Remaining object to be sent " << m_txBuffer->GetBufferSize(socket)
        //                  << " bytes.");
        return packetSize;
    }
    else
    {
        NS_LOG_INFO(this << " Server " << " Failed to send object,"
                         << " GetErrNo= " << socket->GetErrno() << ","
                         << " suspending transmission"
                         << " and waiting for another Tx opportunity.");
        return 0;
    }

} // end of `uint32_t ServeFromTxBuffer (Ptr<Socket> socket)`
void
VideoStreamServerNew::SwitchToState(VideoStreamServerNew::State_t state)
{
    const std::string oldState = GetStateString();
    const std::string newState = GetStateString(state);
    NS_LOG_FUNCTION(this << oldState << newState);
    m_state = state;
    NS_LOG_INFO(this << " Server " << " VideoStreamServerNew " << oldState << " --> " << newState << ".");
    m_stateTransitionTrace(oldState, newState);
}

// HTTP SERVER TX BUFFER //////////////////////////////////////////////////////

VideoStreamServerTxBuffer::VideoStreamServerTxBuffer()
{
    NS_LOG_FUNCTION(this);
}

bool
VideoStreamServerTxBuffer::IsSocketAvailable(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    return (it != m_txBuffer.end());
}

void
VideoStreamServerTxBuffer::AddSocket(Ptr<Socket> socket)
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
VideoStreamServerTxBuffer::RemoveSocket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");

    if (!Simulator::IsExpired(it->second.nextServe))
    {
        NS_LOG_INFO(this << " Server " << " Canceling a serving event which is due in "
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
VideoStreamServerTxBuffer::CloseSocket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");

    if (!Simulator::IsExpired(it->second.nextServe))
    {
        NS_LOG_INFO(this << " Server " << " Canceling a serving event which is due in "
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
VideoStreamServerTxBuffer::CloseAllSockets()
{
    NS_LOG_FUNCTION(this);

    for (auto it = m_txBuffer.begin(); it != m_txBuffer.end(); ++it)
    {
        if (!Simulator::IsExpired(it->second.nextServe))
        {
            NS_LOG_INFO(this << " Server " << " Canceling a serving event which is due in "
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
VideoStreamServerTxBuffer::IsBufferEmpty(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    return (it->second.txBufferSize == 0);
}

Time
VideoStreamServerTxBuffer::GetClientTs(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    return it->second.clientTs;
}

ThreeGppHttpHeader::ContentType_t
VideoStreamServerTxBuffer::GetBufferContentType(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    return it->second.txBufferContentType;
}

uint32_t
VideoStreamServerTxBuffer::GetBufferSize(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    return it->second.txBufferSize;
}

bool
VideoStreamServerTxBuffer::HasTxedPartOfObject(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found");
    return it->second.hasTxedPartOfObject;
}

void
VideoStreamServerTxBuffer::WriteNewObject(Ptr<Socket> socket,
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
VideoStreamServerTxBuffer::RecordNextServe(Ptr<Socket> socket,
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
VideoStreamServerTxBuffer::DepleteBufferSize(Ptr<Socket> socket, uint32_t amount)
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
VideoStreamServerTxBuffer::PrepareClose(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    it->second.isClosing = true;
}

} // namespace ns3
