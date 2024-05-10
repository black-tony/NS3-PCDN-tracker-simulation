/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010-2013, 2015 ComSys, RWTH Aachen University
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
 * Authors: Rene Glebke (principal author), Alexander Hocks
 */

///////////////////////////////////////////////////////////////////////////////
// UNCOMMENT THIS FOR BITTORRENT EMULATION CAPABILITIES
// #define BITTORRENT_EMULATION
///////////////////////////////////////////////////////////////////////////////

#include "ns3/BitTorrentTracker.h"
#include "ns3/BitTorrentVideoClient.h"
#include "ns3/GlobalMetricsGatherer.h"
#include "ns3/Story.h"
#include "ns3/VODSimBriteTopologyHelper.h"
#include "ns3/application-container.h"
#include "ns3/application.h"
#include "ns3/core-module.h"
#include "ns3/nix-vector-helper.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/realtime-simulator-impl.h"
#include "ns3/simulator.h"
#include "ns3/stats-module.h"

#ifdef NS3_MPI
#include "ns3/mpi-interface.h"

#include <mpi.h>
#endif

using namespace ns3;
using namespace bittorrent;

NS_LOG_COMPONENT_DEFINE("BitTorrentLiveSimulation");

void
ShowTimePeriodic()
{
#ifdef NS3_MPI
    if (MpiInterface::GetSystemId() == 0)
#endif
    {
        NS_LOG_INFO("It is now " << Simulator::Now().GetSeconds() << "s (" << GlobalMetricsGatherer::GetWallclockTime() << ")");
    }
    Simulator::Schedule(Seconds(1.0), ShowTimePeriodic);
}

void
ServerTx(Ptr<const Packet> packet, std::string clienttype, bool output)
{
    static std::map<std::string, double> record;
    // std::cout << "it.first" << "\t" << "it.second" << std::endl;

    if (output)
    {
        for (const auto& it : record)
        {
            std::cout << it.first << "\t" << 1.0 * it.second / 1024 / 1024 << "MB" << std::endl;
        }
        return;
    }
    if (record.find(clienttype) == record.end())
    {
        record[clienttype] = 0;
    }
    record[clienttype] += packet->GetSize();
}

int
main(int argc, char* argv[])
{
    // LogComponentEnableAll (LOG_PREFIX_FUNC);

    // LogComponentEnable("BitTorrentHttpClient", LOG_LEVEL_ALL);
    // LogComponentEnable("bittorrent::Peer", LOG_LEVEL_ALL);
    // LogComponentEnable("bittorrent::BitTorrentClient", LOG_LEVEL_INFO);
    // LogComponentEnable("bittorrent::RequestSchedulingStrategyLive", LOG_LEVEL_INFO);
    // LogComponentEnable("BitTorrentTracker", LOG_LEVEL_ALL);

    LogComponentEnable("BitTorrentLiveSimulation", LOG_LEVEL_ALL);

    // LogComponentEnable ("bittorrent::PartSelectionStrategyBase", LOG_LEVEL_ALL);
    // LogComponentEnable("bittorrent::PeerConnectorStrategyBase", LOG_LEVEL_ALL);
    // LogComponentEnable("bittorrent::PeerConnectorStrategyLive", LOG_LEVEL_INFO);
    // LogComponentEnable("bittorrent::VODSimBriteTopologyHelper", LOG_LEVEL_ALL);

#ifdef BITTORRENT_EMULATION
    // Emulation setup (part 1) -->
    //  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
    GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::SyncSimulatorImpl"));
    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));
    // <-- Emulation setup (part 1)
#endif

    std::string storyFileName = "simulation";
    std::string replacements = "";
    uint32_t simulationDuration = 25000;
    bool enableLogging = false;
    bool nullmsg = false;
    std::string comment;
    CommandLine cmd;
    cmd.AddValue("story", "Name of the story input file, without \".story\" ending. Expected to reside within the ns3 directory tree", storyFileName);
    cmd.AddValue(
        "replacements",
        "Variable replacements that shall take place while parsing the story input file, in format \"variable_1:value_1/variable2:value_2\".",
        replacements);
    cmd.AddValue("duration", "Length of the simulation in seconds", simulationDuration);
    cmd.AddValue("logging", "Full-scale logging (0 = off, 1 = on)", enableLogging);
    cmd.AddValue("comment", "A debugging comment; may be used to identify your simulation run", comment);
    cmd.AddValue("nullmsg", "Enable the use of null-message synchronization", nullmsg);

    cmd.Parse(argc, argv);
#ifdef NS3_MPI
    // MPI setup -->
    MpiInterface::Enable(&argc, &argv);
    if (nullmsg)
    {
        GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::NullMessageSimulatorImpl"));
    }
    else
    {
        GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::DistributedSimulatorImpl"));
    }
    // <-- MPI setup
#endif
    std::cout << "Setting up BitTorrent Video-on-Demand simulation..." << std::endl;

    NodeContainer routerNodes;
    NodeContainer btNodes;
    NodeContainer btTrackerNode;
    NodeContainer otherNodes;
    NodeContainer tapNodes;
    VODSimBriteTopologyHelper topologyHelper;
    ApplicationContainer nodeApplicationContainer;
    ApplicationContainer trackerApplicationContainer;

    std::cout << "Reading story file and registering simulation events..." << std::endl;

    Story* story = Story::GetInstance();
    story->SetRouterNodeContainer(&routerNodes);
    story->SetBTNodeContainer(&btNodes);
    story->SetBTTrackerContainer(&btTrackerNode);
    story->SetOtherNodeContainer(&otherNodes);
    story->SetTapNodeContainer(&tapNodes);
    story->SetBriteTopologyHelper(&topologyHelper);
    story->SetBTNodeApplicationContainer(&nodeApplicationContainer);
    story->SetBTTrackerApplicationContainer(&trackerApplicationContainer);
    story->ParseReplacements(replacements);
    story->ReadAndScheduleStory(storyFileName + ".story", simulationDuration);

    std::cout << "Configuring network topology..." << std::endl;

#ifdef NS3_MPI
    topologyHelper.DistributeRouterNodesToMPISystems();
    topologyHelper.DistributeClientNodesToMPISystems();
#endif
    topologyHelper.EstablishNetworkOnLastTopology();

    std::cout << "Configuring tracker for network topology..." << std::endl;

    BitTorrentTracker* btTracker = dynamic_cast<BitTorrentTracker*>(PeekPointer(btTrackerNode.Get(0)->GetApplication(0)));
    btTracker->AddStreamHash(BT_STREAM_DEFAULT_HASH);
    Ptr<Torrent> sharedTorrent = btTracker->AddTorrent(story->GetTorrentFolder(), story->GetTorrentFile());
    btTracker->PrepareForManyClients(sharedTorrent, story->GetBTNodeCount() + 1);
    std::cout << "	Tracker announce URL: " << btTracker->GetAnnounceURL() << std::endl;

    std::cout << "Configuring clients for network topology..." << std::endl;

    for (NodeContainer::Iterator it = btNodes.Begin(); it != btNodes.End(); ++it)
    {
#ifdef NS3_MPI
        if (PeekPointer(*it)->GetSystemId() == MpiInterface::GetSystemId())
#endif
        {
            if (PeekPointer(*it)->GetNApplications() > 0)
            { // We assume the first application to be a BitTorrentClient here
                dynamic_cast<BitTorrentVideoClient*>(PeekPointer((*it)->GetApplication(0)))->SetTorrent(PointerValue(sharedTorrent));
                PeekPointer((*it)->GetApplication(0))->GetObject<BitTorrentClient>()->setTxTrace(MakeCallback(&ServerTx));
            }
        }
    }

    std::cout << "Configuring global metrics gatherer..." << std::endl;
    GlobalMetricsGatherer* gatherer = GlobalMetricsGatherer::GetInstance();
    gatherer->SetFileNamePrefix(story->GetSimulationId(), story->GetLoggingToFile());
    gatherer->RegisterWithApplications(nodeApplicationContainer);
    gatherer->AnnounceExternalClients(tapNodes.GetN());
    gatherer->SetNulloutput(true);
#ifndef BITTORRENT_EMULATION
    gatherer->SetStopFraction(1.0, 1.0);
#else
    // Emulation setup (part 2) -->
    gatherer->SetStopFraction(-1.0, 1.0);
    // <-- Emulation setup (part 2)
#endif
    std::ofstream fout("output/NodeDistribution-part-" + lexical_cast<std::string>(MpiInterface::GetSystemId()) + ".txt");
    for (auto it = btNodes.Begin(); it != btNodes.End(); it++)
    {
#ifdef NS3_MPI
        if (PeekPointer(*it)->GetSystemId() == MpiInterface::GetSystemId())
#endif
        {
            if (PeekPointer(*it)->GetNApplications() > 0)
            {
                fout << dynamic_cast<BitTorrentVideoClient*>(PeekPointer((*it)->GetApplication(0)))->GetNode()->GetId() << "\t"
                     << dynamic_cast<BitTorrentVideoClient*>(PeekPointer((*it)->GetApplication(0)))->GetClientType() << std::endl;
            }
        }
    }
    fout.close();
    std::string probeType;
    std::string tracePath;
    // Ipv4L3Protocol
    probeType = "ns3::Ipv4PacketProbe";
    tracePath = "/NodeList/*/$ns3::Ipv4L3Protocol/Tx";
    // FileHelper fileHelper;

    // Configure the file to be written, and the formatting of output data.
    // fileHelper.ConfigureFile("output/MytestCountsMesh", FileAggregator::TAB_SEPARATED);
    // timeCounter.ConfigureFile("MytestCountsMeshTime", FileAggregator::TAB_SEPARATED);
    // Set the labels for this formatted output file.
    // fileHelper.Set2dFormat("Time (Seconds) = %.3e\tPacket Byte Count = %.0f");
    // timeCounter.Set2dFormat("Time (Seconds) = %.3f\tFirstscreenTime = %.3f");

    // Specify the probe type, trace source path (in configuration namespace), and
    // probe output trace source ("OutputBytes") to write.
    // fileHelper.WriteProbe(probeType, tracePath, "OutputBytes");
    // topologyHelper.WriteLastTopologyToGraphVizFile (story->GetSimulationId () + ".dot");
    // Ptr<OutputStreamWrapper> routingStream =
    //           Create<OutputStreamWrapper>("nix-simple-ipv4.routes", std::ios::out);
    // Ipv4NixVectorHelper::PrintRoutingTableAllAt(Seconds(8), routingStream);
    std::cout << "Starting BitTorrent Video-on-Demand simulation..." << std::endl;
    Simulator::ScheduleNow(ShowTimePeriodic);
    Simulator::Stop(Seconds(simulationDuration) + MilliSeconds(1));
    gatherer->WriteToFile("simulation-started", GlobalMetricsGatherer::GetWallclockTime(), false);
    Simulator::Run();
    Simulator::Destroy();
    std::string txCountFilename = "output/MytestCountsMesh-part-" + lexical_cast<std::string>(MpiInterface::GetSystemId()) + ".txt";
    int save_stdout_no = dup(fileno(stdout));
    std::fflush(stdout);
    std::freopen(txCountFilename.c_str(), "w", stdout);
    ServerTx(nullptr, "", true);
    // std::fflush();
    std::fflush(stdout);
    dup2(save_stdout_no, fileno(stdout));
    close(save_stdout_no);
    std::freopen(nullptr, "w", stdout);

    std::cout << GlobalMetricsGatherer::GetWallclockTime() << ": BitTorent Video-on-Demand simulation finished successfully :=)" << std::endl;
    MpiInterface::Disable();
    return 0;
}
