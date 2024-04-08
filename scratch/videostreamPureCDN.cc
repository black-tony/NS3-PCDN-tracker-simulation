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
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/stats-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("videostreamPureCDN");

void
ServerConnectionEstablished(Ptr<const VideoStreamServer>, Ptr<Socket>)
{
    // NS_LOG_INFO("Client has established a connection to the server.");
}

void
MainObjectGenerated(uint32_t size)
{
    // NS_LOG_INFO("Server generated a main object of " << size << " bytes.");
}

void
EmbeddedObjectGenerated(uint32_t size)
{
    // NS_LOG_INFO("Server generated an embedded object of " << size << " bytes.");
}

void
ServerTx(Ptr<const Packet> packet)
{
    // NS_LOG_INFO("Server sent a packet of " << packet->GetSize() << " bytes.");
}

void
ClientRx(Ptr<const Packet> packet, const Address& address)
{
    // NS_LOG_INFO("Client received a packet of " << packet->GetSize() << " bytes from " <<
    // address);
}

void
ClientMainObjectReceived(Ptr<const VideoStreamClient>, Ptr<const Packet> packet)
{
    // Ptr<Packet> p = packet->Copy();
    // ThreeGppHttpHeader header;
    // p->RemoveHeader(header);
    // if (header.GetContentLength() == p->GetSize() &&
    //     header.GetContentType() == ThreeGppHttpHeader::MAIN_OBJECT)
    // {
    //     NS_LOG_INFO("Client has successfully received a main object of " << p->GetSize()
    //                                                                      << " bytes.");
    // }
    // else
    // {
    //     NS_LOG_INFO("Client failed to parse a main object. ");
    // }
}

void
ClientEmbeddedObjectReceived(Ptr<const VideoStreamClient>, Ptr<const Packet> packet)
{
    // Ptr<Packet> p = packet->Copy();
    // ThreeGppHttpHeader header;
    // p->RemoveHeader(header);
    // if (header.GetContentLength() == p->GetSize() &&
    //     header.GetContentType() == ThreeGppHttpHeader::EMBEDDED_OBJECT)
    // {
    //     NS_LOG_INFO("Client has successfully received an embedded object of " << p->GetSize()
    //                                                                           << " bytes.");
    // }
    // else
    // {
    //     NS_LOG_INFO("Client failed to parse an embedded object. ");
    // }
}

void
ClientPageReceived(Ptr<const VideoStreamClient> client,
                   const Time& time,
                   uint32_t numObjects,
                   uint32_t numBytes)
{
    // NS_LOG_INFO("Client " << client << " has received a page that took " << time.As(Time::MS)
    //                       << " ms to load with " << numObjects << " objects and " << numBytes
    //                       << " bytes.");
}

int
main(int argc, char* argv[])
{
    double simTimeSec = 6;
    // int ARG_GROUP_NODE_NUM = 3;
    int ARG_PCDN_NDOE_NUM = 3;
    int ARG_CDN_NODE_NUM = 1;
    CommandLine cmd(__FILE__);
    cmd.AddValue("SimulationTime", "Length of simulation in seconds.", simTimeSec);
    cmd.AddValue("GroupNodeNumber", "group node number", ARG_PCDN_NDOE_NUM);
    // cmd.AddValue("PCDNNodeNumber", "pcdn node number", ARG_PCDN_NDOE_NUM);
    cmd.AddValue("CDNNodeNumber", "cdn node number", ARG_CDN_NODE_NUM);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnableAll(LOG_PREFIX_TIME);
    // LogComponentEnableAll (LOG_PREFIX_FUNC);
    LogComponentEnable("VideoStreamClient", LOG_LEVEL_INFO);
    LogComponentEnable("VideoStreamServer", LOG_LEVEL_INFO);
    LogComponentEnable("VideoStreamPCDN", LOG_LEVEL_INFO);
    LogComponentEnable("videostreamPureCDN", LOG_LEVEL_INFO);

    // Setup two nodes
    // const int GROUP_NODE_NUM = 3;
    const int PCDN_NDOE_NUM = ARG_PCDN_NDOE_NUM;
    const int CDN_NODE_NUM = ARG_CDN_NODE_NUM;
#define CDN_TMP_NODEID(o) (o << 1)
#define CDN_SERVER_NODEID(o) ((o << 1) + 1)

    // #define GROUP_TMP_NODEID GROUP_NODE_NUM
    // NodeContainer clientNodes[CDN_NODE_NUM][PCDN_NDOE_NUM];
    NodeContainer* clientNodes = new NodeContainer[CDN_NODE_NUM];
    NodeContainer CDNNodes;
    InternetStackHelper stack;
    // for (int i = 0; i < CDN_NODE_NUM; i++)
    // {
    //     for (int j = 0; j < PCDN_NDOE_NUM; j++)
    //     {
    //         clientNodes[i][j].Create(GROUP_NODE_NUM + 1);
    //         stack.Install(clientNodes[i][j]);
    //     }
    // }

    for (int i = 0; i < CDN_NODE_NUM; i++)
    {
        clientNodes[i].Create(PCDN_NDOE_NUM); // PCDN_NUM
        stack.Install(clientNodes[i]);
    }
    CDNNodes.Create(CDN_NODE_NUM * 2);
    stack.Install(CDNNodes);
    // CDNNodes.Create(1);
    // HigherNodes.Create(3);
    // clientNodes[0].Create(1);
    // clientNodes[0].Add(HigherNodes.Get(1));
    // clientNodes[1].Create(1);
    // clientNodes[1].Add(HigherNodes.Get(2));
    // PointToPointHelper clientP2PHelper[CDN_NODE_NUM][PCDN_NDOE_NUM][GROUP_NODE_NUM + 1];
    // Ipv4InterfaceContainer clientP2PInterface[CDN_NODE_NUM][PCDN_NDOE_NUM][GROUP_NODE_NUM + 1];
    // PointToPointHelper clientP2PHelper[CDN_NODE_NUM][PCDN_NDOE_NUM + 1];
    PointToPointHelper** clientP2PHelper = new PointToPointHelper*[CDN_NODE_NUM];
    for (int i = 0; i < CDN_NODE_NUM; ++i)
    {
        clientP2PHelper[i] = new PointToPointHelper[PCDN_NDOE_NUM + 1];
        // for (int j = 0; j <= PCDN_NDOE_NUM; ++j) {
        //     clientP2PHelper[i][j] = new PointToPointHelper();
        // }
    }
    Ipv4InterfaceContainer** clientP2PInterface = new Ipv4InterfaceContainer*[CDN_NODE_NUM];
    for (int i = 0; i < CDN_NODE_NUM; ++i)
    {
        clientP2PInterface[i] = new Ipv4InterfaceContainer[PCDN_NDOE_NUM + 1];
        // for (int j = 0; j <= PCDN_NDOE_NUM; ++j) {
        //     clientP2PHelper[i][j] = new PointToPointHelper();
        // }
    }
    // [PCDN_NDOE_NUM + 1];
    // CsmaHelper HigherAllToAllHelper[CDN_NODE_NUM];
    // Ipv4InterfaceContainer HigherAllToAllinterface[CDN_NODE_NUM];

    // for (int k = 0; k < CDN_NODE_NUM; ++k)
    // {
    //     for (int i = 0; i < PCDN_NDOE_NUM; i++)
    //     {
    //         for (int j = 0; j < GROUP_NODE_NUM; j++)
    //         {
    //             clientP2PHelper[k][i][j].SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    //             clientP2PHelper[k][i][j].SetChannelAttribute("Delay",
    //                                                          TimeValue(NanoSeconds(6560 * 2)));
    //         }
    //         // 这一条用于连接client的router和PCDN节点
    //         clientP2PHelper[k][i][GROUP_NODE_NUM].SetDeviceAttribute("DataRate",
    //                                                                  StringValue("100Mbps"));
    //         clientP2PHelper[k][i][GROUP_NODE_NUM].SetChannelAttribute("Delay",
    //                                                                   TimeValue(NanoSeconds(6560)));
    //     }
    // }
    for (int i = 0; i < CDN_NODE_NUM; i++)
    {
        for (int j = 0; j < PCDN_NDOE_NUM; j++)
        {
            clientP2PHelper[i][j].SetDeviceAttribute("DataRate", StringValue("3Mbps"));
            clientP2PHelper[i][j].SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
        }
        // 这一条用于连接PCDN的router和CDN节点
        clientP2PHelper[i][PCDN_NDOE_NUM].SetDeviceAttribute("DataRate", StringValue("100Mbps"));
        clientP2PHelper[i][PCDN_NDOE_NUM].SetChannelAttribute("Delay",
                                                              TimeValue(NanoSeconds(6560)));
    }

    // for(int i = 0; i < NODE_NUM; i++)
    // {
    //     PointToPointHelper p2p;
    //     p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    //     p2p.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    //     pointToPoint[i] = p2p;

    // }
    // pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    // pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));
    // CsmaHelper csma;
    // csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    // csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    // //TODO larger scale
    // NetDeviceContainer higherDevices, lowerDevice[2];
    // higherDevices = csma.Install(HigherNodes);
    // lowerDevice[0] = csma.Install(clientNodes[0]);
    // lowerDevice[1] = csma.Install(clientNodes[1]);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    NetDeviceContainer device;

    // for (int k = 0; k < CDN_NODE_NUM; ++k)
    // {
    //     for (int i = 0; i < PCDN_NDOE_NUM; i++)
    //     {
    //         for (int j = 0; j < GROUP_NODE_NUM; j++)
    //         {
    //             clientP2PHelper[k][i][j].SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    //             clientP2PHelper[k][i][j].SetChannelAttribute("Delay",
    //                                                          TimeValue(NanoSeconds(6560 * 2)));
    //         }
    //         // 这一条用于连接client的router和PCDN节点
    //         clientP2PHelper[k][i][GROUP_NODE_NUM].SetDeviceAttribute("DataRate",
    //         StringValue("100Mbps"));
    //         clientP2PHelper[k][i][GROUP_NODE_NUM].SetChannelAttribute("Delay",
    //                                                          TimeValue(NanoSeconds(6560)));
    //     }
    // }
    // for(int k = 0; k < CDN_NODE_NUM; k++)
    // {
    //     device = higherCSMAHelper[k].Install(HigherNodes[k]);
    //     higherP2PInterface[k] = address.Assign(device);
    //     address.NewNetwork();
    //     // int idCnt = 0;
    //     // for(int i = 0; i < PCDN_NDOE_NUM; i++)
    //     // {
    //     //     device = higherCSMAHelper[k][idCnt].Install(HigherNodes[k].Get(i),
    //     HigherNodes[k].Get(PCDN_NDOE_NUM));
    //     //     higherP2PInterface[k][idCnt++] = address.Assign(device);
    //     //     std::cout << higherP2PInterface[k][idCnt - 1].GetAddress(0) << "---" <<
    //     higherP2PInterface[k][idCnt - 1].GetAddress(1) << std::endl;
    //     //     address.NewAddress();
    //     // }
    //     // exit(0);
    //     // for(int i = 0; i < PCDN_NDOE_NUM; i++)
    //     // {
    //     //     for(int j = i + 1; j < PCDN_NDOE_NUM; j++)
    //     //     {
    //     //         // if(i == j)
    //     //         //     continue;
    //     //         device = higherCSMAHelper[k][idCnt].Install(HigherNodes[k].Get(i),
    //     HigherNodes[k].Get(j));
    //     //         higherP2PInterface[k][idCnt++] = address.Assign(device);
    //     //         address.NewAddress();
    //     //     }
    //     // }

    // }
    // client to PCDN
    address.SetBase("10.1.1.0", "255.255.255.0");
    // for (int k = 0; k < CDN_NODE_NUM; ++k)
    // {
    //     for (int i = 0; i < PCDN_NDOE_NUM; i++)
    //     {
    //         for (int j = 0; j < GROUP_NODE_NUM; j++)
    //         {
    //             device = clientP2PHelper[k][i][j].Install(clientNodes[k][i].Get(j),
    //                                                       clientNodes[k][i].Get(GROUP_TMP_NODEID));
    //             clientP2PInterface[k][i][j] = address.Assign(device);
    //             NS_LOG_INFO("client to router_client : "
    //                         << clientP2PInterface[k][i][j].GetAddress(0) << "------"
    //                         << clientP2PInterface[k][i][j].GetAddress(1));
    //             address.NewNetwork();
    //         }
    //         // router to PCDN Node
    //         device = clientP2PHelper[k][i][GROUP_TMP_NODEID].Install(
    //             HigherNodes[k].Get(i),
    //             clientNodes[k][i].Get(GROUP_TMP_NODEID));
    //         clientP2PInterface[k][i][GROUP_TMP_NODEID] =
    //             address.Assign(device); // get 0.address == PCDN server address
    //         NS_LOG_INFO("PCDN to router_client: "
    //                     << clientP2PInterface[k][i][GROUP_TMP_NODEID].GetAddress(0) << "------"
    //                     << clientP2PInterface[k][i][GROUP_TMP_NODEID].GetAddress(1));

    //         address.NewNetwork();
    //     }
    // }
    // address.NewNetwork();
    for (int i = 0; i < CDN_NODE_NUM; i++)
    {
        for (int j = 0; j < PCDN_NDOE_NUM; j++)
        {
            device = clientP2PHelper[i][j].Install(clientNodes[i].Get(j),
                                                   CDNNodes.Get(CDN_TMP_NODEID(i)));
            clientP2PInterface[i][j] = address.Assign(device);
            NS_LOG_INFO("PCDN to router_CDN : " << clientP2PInterface[i][j].GetAddress(0)
                                                << "------"
                                                << clientP2PInterface[i][j].GetAddress(1));

            address.NewNetwork();
        }
        // 这一条用于连接PCDN的router和CDN节点
        device = clientP2PHelper[i][PCDN_NDOE_NUM].Install(CDNNodes.Get(CDN_SERVER_NODEID(i)),
                                                           CDNNodes.Get(CDN_TMP_NODEID(i)));
        clientP2PInterface[i][PCDN_NDOE_NUM] =
            address.Assign(device); // get 0.address == CDN server address
        NS_LOG_INFO("CDN to router_CDN : " << clientP2PInterface[i][PCDN_NDOE_NUM].GetAddress(0)
                                           << "------"
                                           << clientP2PInterface[i][PCDN_NDOE_NUM].GetAddress(1));

        address.NewNetwork();
    }
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Ipv4InterfaceContainer higherInterfaces = address.Assign(higherDevices), lowerInterface[2];
    // address.SetBase("10.1.2.0", "255.255.255.0");

    // lowerInterface[0] = address.Assign(lowerDevice[0]);
    // address.SetBase("10.1.3.0", "255.255.255.0");

    // lowerInterface[1] = address.Assign(lowerDevice[1]);

    // Ipv4Address serverAddress = higherInterfaces.GetAddress(0), PCDNAddress[2];
    // PCDNAddress[0] = lowerInterface[0].GetAddress(1);
    // PCDNAddress[1] = lowerInterface[1].GetAddress(1);
    for (int i = 0; i < CDN_NODE_NUM; i++)
    {
        VideoStreamServerHelper serverHelper(clientP2PInterface[i][PCDN_NDOE_NUM].GetAddress(0));
        ApplicationContainer serverApps = serverHelper.Install(CDNNodes.Get(CDN_SERVER_NODEID(i)));
        serverApps.Stop(Seconds(simTimeSec));
        Ptr<VideoStreamServer> httpServer = serverApps.Get(0)->GetObject<VideoStreamServer>();
        httpServer->TraceConnectWithoutContext("ConnectionEstablished",
                                               MakeCallback(&ServerConnectionEstablished));
        httpServer->TraceConnectWithoutContext("MainObject", MakeCallback(&MainObjectGenerated));
        httpServer->TraceConnectWithoutContext("EmbeddedObject",
                                               MakeCallback(&EmbeddedObjectGenerated));
        httpServer->TraceConnectWithoutContext("Tx", MakeCallback(&ServerTx));
        httpServer->SetUsingPCDN(false);
        PointerValue varPtr;
        httpServer->GetAttribute("Variables", varPtr);
        Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables>();
        httpVariables->SetMainObjectSizeMean(102400);  // 100kB
        httpVariables->SetMainObjectSizeStdDev(40960); // 40kB
        httpVariables->SetNumOfEmbeddedObjectsMax(0);
    }
    // for (int k = 0; k < CDN_NODE_NUM; k++)
    // {
    //     for (int i = 0; i < PCDN_NDOE_NUM; i++)
    //     {
    //         VideoStreamPCDNHelper PCDNHelper(
    //             clientP2PInterface[k][i][GROUP_TMP_NODEID].GetAddress(0),
    //             higherP2PInterface[k][i].GetAddress(0),
    //             higherP2PInterface[k][PCDN_NDOE_NUM].GetAddress(0));
    //         ApplicationContainer PCDNApps = PCDNHelper.Install(HigherNodes[k].Get(i));
    //         PCDNApps.Stop(Seconds(simTimeSec));
    //     }
    // }
    // const int videoRequestNumber[CDN_NODE_NUM][PCDN_NDOE_NUM] = {// CDN 1
    //                                                              {
    //                                                                  // PCDN 1
    //                                                                  1,
    //                                                                  2,
    //                                                                  3,
    //                                                                  1,
    //                                                                  2,
    //                                                                  3,
    //                                                                  1,
    //                                                                  2,
    //                                                                  3
    //                                                                  // 1
    //                                                              }};
    // int cntNode = 0;
    srand(time(0));
    for (int k = 0; k < CDN_NODE_NUM; k++)
    {
        for (int i = 0; i < PCDN_NDOE_NUM; i++)
        {
            // for (int j = 0; j < GROUP_NODE_NUM; j++)
            // {
            VideoStreamClientHelper clientHelper(
                clientP2PInterface[k][PCDN_NDOE_NUM].GetAddress(0));
            ApplicationContainer clientApps = clientHelper.Install(clientNodes[k].Get(i));
            clientApps.Stop(Seconds(simTimeSec));
            Ptr<VideoStreamClient> httpClient = clientApps.Get(0)->GetObject<VideoStreamClient>();
            // std::stringstream ss;
            // ss << "/Names/client/" << cntNode++;
            // Names::Add(ss.str(), httpClient);
            httpClient->SetVideoInfos(rand() % 3 + 1);
            clientApps.Start(Seconds(rand() % 3 + 1));
            // }
        }
    }
    // // clientApps[1].Start(Seconds(2));

    // // clientApps[0].Stop(Seconds(simTimeSec));
    // httpClient = clientApps[1].Get(0)->GetObject<VideoStreamClient>();
    // httpClient->SetVideoInfos(3);
    // clientApps[1].Start(Seconds(2));
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
    //     httpClient->TraceConnectWithoutContext("RxMainObject",
    //     MakeCallback(&ClientMainObjectReceived));
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
    AnimationInterface anim("testPureCDN.xml");
    // int clientPos = 1;
    int clientRouterPos = 1;
    // int PCDNPos = 1;
    int CDNPos = 1;
    for (int k = 0; k < CDN_NODE_NUM; ++k)
    {
        for (int i = 0; i < PCDN_NDOE_NUM; i++)
        {
            // for (int j = 0; j < GROUP_NODE_NUM; j++)
            // {
            //     anim.SetConstantPosition(clientNodes[k][i].Get(j), (clientPos++) * 4, 1 * 4);
            // // }
            // anim.SetConstantPosition(clientNodes[k][i].Get(GROUP_TMP_NODEID),
            //                          (clientRouterPos) * 4,
            //                          2 * 4);
            anim.SetConstantPosition(clientNodes[k].Get(i), (clientRouterPos++) * 4, 3 * 4);
        }
    }
    for (int i = 0; i < CDN_NODE_NUM; i++)
    {
        anim.SetConstantPosition(CDNNodes.Get(CDN_TMP_NODEID(i)), (CDNPos) * 4, 4 * 4);
        anim.SetConstantPosition(CDNNodes.Get(CDN_SERVER_NODEID(i)), (CDNPos++) * 4, 5 * 4);
    }
    std::string probeType;
    std::string tracePath;
    probeType = "ns3::Ipv4PacketProbe";
    tracePath = "/NodeList/*/$ns3::Ipv4L3Protocol/Tx";
    FileHelper fileHelper, timeCounter;

    // Configure the file to be written, and the formatting of output data.
    fileHelper.ConfigureFile("MytestCountsPureCDN", FileAggregator::TAB_SEPARATED);
    timeCounter.ConfigureFile("MytestCountsPureCDNTime", FileAggregator::TAB_SEPARATED);

    // Set the labels for this formatted output file.
    // fileHelper.Set2dFormat("Time (Seconds) = %.3e\tPacket Byte Count = %.0f");
    // timeCounter.Set2dFormat("Time (Seconds) = %.3f\tFirstscreenTime = %.3f");

    // Specify the probe type, trace source path (in configuration namespace), and
    // probe output trace source ("OutputBytes") to write.
    fileHelper.WriteProbe(probeType, tracePath, "OutputBytes");
    timeCounter.WriteProbe("ns3::TimeProbe",
                           "/NodeList/*/ApplicationList/0/$ns3::VideoStreamClient/FirstTime",
                           "Output");
    // anim.SetConstantPosition(nodes.Get(0), 1.0, 2.0);//描述节点0的坐标
    // anim.SetConstantPosition(nodes.Get(1), 2.0, 3.0);//描述节点1的坐标
    Simulator::Run();
    Simulator::Destroy();

    for (int i = 0; i < CDN_NODE_NUM; ++i)
    {
        delete[] clientP2PInterface[i];
    }
    delete[] clientP2PInterface;

    for (int i = 0; i < CDN_NODE_NUM; ++i)
    {
        delete[] clientP2PHelper[i];
    }
    delete[] clientP2PHelper;

    delete[] clientNodes;

    return 0;
}
