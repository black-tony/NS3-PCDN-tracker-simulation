/*
 * Copyright (c) 2016 Magister Solutions
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
 * Author: Lauri Sormunen <lauri.sormunen@magister.fi>
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/netanim-module.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VideoStreamP2P");

void
ServerConnectionEstablished(Ptr<const VideoStreamServer>, Ptr<Socket>)
{
    NS_LOG_INFO("Client has established a connection to the server.");
}

void
MainObjectGenerated(uint32_t size)
{
    NS_LOG_INFO("Server generated a main object of " << size << " bytes.");
}

void
EmbeddedObjectGenerated(uint32_t size)
{
    NS_LOG_INFO("Server generated an embedded object of " << size << " bytes.");
}

void
ServerTx(Ptr<const Packet> packet)
{
    NS_LOG_INFO("Server sent a packet of " << packet->GetSize() << " bytes.");
}

void
ClientRx(Ptr<const Packet> packet, const Address& address)
{
    NS_LOG_INFO("Client received a packet of " << packet->GetSize() << " bytes from " << address);
}

void
ClientMainObjectReceived(Ptr<const VideoStreamClient>, Ptr<const Packet> packet)
{
    Ptr<Packet> p = packet->Copy();
    ThreeGppHttpHeader header;
    p->RemoveHeader(header);
    if (header.GetContentLength() == p->GetSize() &&
        header.GetContentType() == ThreeGppHttpHeader::MAIN_OBJECT)
    {
        NS_LOG_INFO("Client has successfully received a main object of " << p->GetSize()
                                                                         << " bytes.");
    }
    else
    {
        NS_LOG_INFO("Client failed to parse a main object. ");
    }
}

void
ClientEmbeddedObjectReceived(Ptr<const VideoStreamClient>, Ptr<const Packet> packet)
{
    Ptr<Packet> p = packet->Copy();
    ThreeGppHttpHeader header;
    p->RemoveHeader(header);
    if (header.GetContentLength() == p->GetSize() &&
        header.GetContentType() == ThreeGppHttpHeader::EMBEDDED_OBJECT)
    {
        NS_LOG_INFO("Client has successfully received an embedded object of " << p->GetSize()
                                                                              << " bytes.");
    }
    else
    {
        NS_LOG_INFO("Client failed to parse an embedded object. ");
    }
}

void
ClientPageReceived(Ptr<const VideoStreamClient> client,
                   const Time& time,
                   uint32_t numObjects,
                   uint32_t numBytes)
{
    NS_LOG_INFO("Client " << client << " has received a page that took " << time.As(Time::MS)
                          << " ms to load with " << numObjects << " objects and " << numBytes
                          << " bytes.");
}

int
main(int argc, char* argv[])
{
    double simTimeSec = 6;
    CommandLine cmd(__FILE__);
    cmd.AddValue("SimulationTime", "Length of simulation in seconds.", simTimeSec);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnableAll(LOG_PREFIX_TIME);
    // LogComponentEnableAll (LOG_PREFIX_FUNC);
    LogComponentEnable ("VideoStreamClient", LOG_INFO);
    LogComponentEnable ("VideoStreamServer", LOG_INFO);
    LogComponentEnable ("VideoStreamPCDN", LOG_INFO);
    // LogComponentEnable("VideoStreamP2P", LOG_INFO);

    // Setup two nodes
    const int NODE_NUM = 2;
    NodeContainer clientNodes[2];
    NodeContainer HigherNodes;
    // CDNNodes.Create(1);
    HigherNodes.Create(3);
    clientNodes[0].Create(1);
    clientNodes[0].Add(HigherNodes.Get(1));
    clientNodes[1].Create(1);
    clientNodes[1].Add(HigherNodes.Get(2));
    PointToPointHelper pointToPoint[NODE_NUM];

    for(int i = 0; i < NODE_NUM; i++)
    {
        PointToPointHelper p2p;
        p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
        p2p.SetChannelAttribute("Delay", StringValue("2ms"));
        pointToPoint[i] = p2p;

    }
    // pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    // pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    //TODO larger scale
    NetDeviceContainer higherDevices, lowerDevice[2];
    higherDevices = csma.Install(HigherNodes);
    lowerDevice[0] = csma.Install(clientNodes[0]);
    lowerDevice[1] = csma.Install(clientNodes[1]);


    InternetStackHelper stack;
    stack.Install(HigherNodes);
    stack.Install(clientNodes[0].Get(0));
    stack.Install(clientNodes[1].Get(0));


    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer higherInterfaces = address.Assign(higherDevices), lowerInterface[2];
    address.SetBase("10.1.2.0", "255.255.255.0");

    lowerInterface[0] = address.Assign(lowerDevice[0]);
    address.SetBase("10.1.3.0", "255.255.255.0");

    lowerInterface[1] = address.Assign(lowerDevice[1]);


    Ipv4Address serverAddress = higherInterfaces.GetAddress(0), PCDNAddress[2];
    PCDNAddress[0] = lowerInterface[0].GetAddress(1);
    PCDNAddress[1] = lowerInterface[1].GetAddress(1);


    VideoStreamServerHelper serverHelper(serverAddress);
    ApplicationContainer serverApps = serverHelper.Install(HigherNodes.Get(0));
    serverApps.Stop(Seconds(simTimeSec));
    Ptr<VideoStreamServer> httpServer = serverApps.Get(0)->GetObject<VideoStreamServer>();
    httpServer->TraceConnectWithoutContext("ConnectionEstablished",
                                           MakeCallback(&ServerConnectionEstablished));
    httpServer->TraceConnectWithoutContext("MainObject", MakeCallback(&MainObjectGenerated));
    httpServer->TraceConnectWithoutContext("EmbeddedObject",
                                           MakeCallback(&EmbeddedObjectGenerated));
    httpServer->TraceConnectWithoutContext("Tx", MakeCallback(&ServerTx));
    httpServer->SetUsingPCDN(true);
    PointerValue varPtr;
    httpServer->GetAttribute("Variables", varPtr);
    Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables>();
    httpVariables->SetMainObjectSizeMean(102400);  // 100kB
    httpVariables->SetMainObjectSizeStdDev(40960); // 40kB
    httpVariables->SetNumOfEmbeddedObjectsMax(0);


    VideoStreamPCDNHelper  PCDNHelper[2] = {VideoStreamPCDNHelper(PCDNAddress[0],higherInterfaces.GetAddress(1),  serverAddress),VideoStreamPCDNHelper(PCDNAddress[1], higherInterfaces.GetAddress(2), serverAddress) };
    ApplicationContainer PCDNApps[2] = {PCDNHelper[0].Install(HigherNodes.Get(1)), PCDNHelper[1].Install(HigherNodes.Get(2))};
    // ApplicationContainer PCDNApps = ;
    for(auto &i : PCDNApps)
    {
        i.Stop(Seconds(simTimeSec));
    }

    VideoStreamClientHelper clientHelper[2] = {VideoStreamClientHelper(PCDNAddress[0]), VideoStreamClientHelper(PCDNAddress[1])};
    ApplicationContainer clientApps[2] = {clientHelper[0].Install(clientNodes[0].Get(0)), clientHelper[1].Install(clientNodes[1].Get(0))};
    for(auto &i : clientApps)
    {
        i.Stop(Seconds(simTimeSec));
    }

    Ptr<VideoStreamClient> httpClient = clientApps[0].Get(0)->GetObject<VideoStreamClient>();
    httpClient->SetVideoInfos(3);
    // clientApps[1].Start(Seconds(2));

    // clientApps[0].Stop(Seconds(simTimeSec));
    httpClient = clientApps[1].Get(0)->GetObject<VideoStreamClient>();
    httpClient->SetVideoInfos(3);
    clientApps[1].Start(Seconds(2));
    // clientApps[1].Stop(Seconds(simTimeSec));

    // for(int i = 0; i < NODE_NUM; i++)
    // {
    //     devices = pointToPoint[i].Install(nodes.Get(i), nodes.Get(NODE_NUM));
    //     Ipv4InterfaceContainer interfaces = address.Assign(devices);
    //     if(i == 0)
    //         serverAddress = interfaces.GetAddress(1);
    //     // address.NewNetwork()

    // }
    // devices = pointToPoint.Install(nodes);





    // Create HTTP server helper

    // Install HTTP server

    // Example of connecting to the trace sources


    // Setup HTTP variables for the server

    // Create HTTP client helper

    // Install HTTP client

    // for(int i = 0; i < NODE_NUM; i++)
    // {
    //     ApplicationContainer clientApps = clientHelper.Install(nodes.Get(i));
    //     Ptr<VideoStreamClient> httpClient = clientApps.Get(0)->GetObject<VideoStreamClient>();

    //     // Example of connecting to the trace sources
    //     httpClient->TraceConnectWithoutContext("RxMainObject", MakeCallback(&ClientMainObjectReceived));
    //     httpClient->TraceConnectWithoutContext("RxEmbeddedObject",
    //                                         MakeCallback(&ClientEmbeddedObjectReceived));
    //     httpClient->TraceConnectWithoutContext("Rx", MakeCallback(&ClientRx));
    //     httpClient->TraceConnectWithoutContext("RxPage", MakeCallback(&ClientPageReceived));
    //     httpClient->SetVideoInfos(i);
    //     // Stop browsing after 30 minutes
    //     clientApps.Stop(Seconds(simTimeSec));
    // }
    // for(int i = 0; i < NODE_NUM; i++)
    // {
    //     AsciiTraceHelper ascii;
    //     pointToPoint[i].EnableAsciiAll(ascii.CreateFileStream("myHTTPtest.tr"));
    //     pointToPoint[i].EnablePcapAll("myHTTPtest");
    // }
    // Names
    AnimationInterface anim("test.xml");
    // anim.SetConstantPosition(nodes.Get(0), 1.0, 2.0);//描述节点0的坐标
    // anim.SetConstantPosition(nodes.Get(1), 2.0, 3.0);//描述节点1的坐标
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
