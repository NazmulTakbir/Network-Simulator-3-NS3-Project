#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-flow-classifier.h"
#include "ns3/flow-monitor-helper.h"
#include <ns3/lr-wpan-error-model.h>
#include <string>
#include <cmath>
using namespace std;
using namespace ns3;

uint n_flows=3, n_nodes=3, sinkPort=9, maxRange=10, gap=1;
uint start_time=0, stop_time, duration=100, pkts_ps=100, pkt_sz=100;
bool verbose = false;
string filePrefix, congestionAlgo = "TcpNewReno", recoveryAlgo="ns3::TcpClassicRecovery";

void processArguments(int argc, char** argv) {
    CommandLine cmd (__FILE__);
    cmd.AddValue ("n_flows", "Number of Flows", n_flows);
    cmd.AddValue ("congestionAlgo", "Congestion Control Algorithm", congestionAlgo);
    cmd.AddValue ("pkts_ps", "Packets Per Second", pkts_ps);
    cmd.AddValue ("pkt_sz", "Packets Size", pkt_sz);
    cmd.AddValue ("n_nodes", "Number of Nodes", n_nodes);
    cmd.AddValue ("verbose", "Print In Console", verbose);
    cmd.AddValue ("duration", "Duration", duration);
    cmd.AddValue ("maxRange", "Max Range", maxRange);
    cmd.AddValue ("gap", "Gap", gap);
    cmd.Parse (argc, argv);

    if( n_nodes<1 ) {
        std::cout << "Minimum 1 Node Required\n";
        exit(0);
    }
    else if( n_flows<1 ) {
        std::cout << "Minimum 1 Flow Required\n";
        exit(0);
    }
    else if( duration<10 ) {
        std::cout << "Minimum 10s Duration Required\n";
        exit(0);
    }

    std::cout << "\n--------------------------------------------------------------------------\n";
    std::cout << "Nodes: " << n_nodes << ", Flows: " << n_flows << ", Packets PPS: " << pkts_ps <<
                 ", Max Range: " << maxRange;
    std::cout << "\n--------------------------------------------------------------------------\n";

    if(verbose) LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    stop_time = start_time + duration + 2;

    filePrefix = "wpan-"+to_string(n_nodes)+"-"+to_string(n_flows)+"-"+to_string(pkts_ps)+"-"+to_string(maxRange); 

    congestionAlgo = "ns3::" + congestionAlgo;
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue (congestionAlgo));
    Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType", StringValue (recoveryAlgo));  
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 22));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 22));
    Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (true));
    Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue (QueueSize ("100p")));
    Config::SetDefault ("ns3::FifoQueueDisc::MaxSize", QueueSizeValue (QueueSize ("100p")));
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (pkt_sz));
}

int main (int argc, char** argv) {
    processArguments(argc, argv);

    NodeContainer wirelessNodes;
    wirelessNodes.Create (n_nodes+1);

    uint centerNode = 0;

    NodeContainer wiredNodes;
    wiredNodes.Create (1);
    wiredNodes.Add (wirelessNodes.Get (centerNode));

    InternetStackHelper internetv6;
    internetv6.InstallAll();

    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue (0.0),
                                    "MinY", DoubleValue (0.0),
                                    "DeltaX", DoubleValue (2),
                                    "DeltaY", DoubleValue (3),
                                    "GridWidth", UintegerValue (2),
                                    "LayoutType", StringValue ("RowFirst"));
    mobility.Install (wirelessNodes);

    LrWpanHelper lrWpanHelper;
    Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (maxRange));
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
    Ptr<RangePropagationLossModel> propModel = CreateObject<RangePropagationLossModel> ();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
    channel->AddPropagationLossModel (propModel);
    channel->SetPropagationDelayModel (delayModel);
    lrWpanHelper.SetChannel(channel);

    NetDeviceContainer lrwpanNetDevices = lrWpanHelper.Install (wirelessNodes);
    lrWpanHelper.AssociateToPan (lrwpanNetDevices, 0);

    SixLowPanHelper sixLowPanHelper;
    NetDeviceContainer sixLowPanNetDevices = sixLowPanHelper.Install (lrwpanNetDevices);

    PointToPointHelper p2pHelper;
    p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2pHelper.SetChannelAttribute ("Delay", StringValue ("0.01ms"));
    NetDeviceContainer p2pDevices = p2pHelper.Install (wiredNodes);

    Ipv6AddressHelper ipv6;
    ipv6.SetBase (Ipv6Address ("2001:cafe::"), Ipv6Prefix (64));
    Ipv6InterfaceContainer wiredDeviceInterfaces;
    wiredDeviceInterfaces = ipv6.Assign (p2pDevices);
    wiredDeviceInterfaces.SetForwarding (1, true);
    wiredDeviceInterfaces.SetDefaultRouteInAllNodes (1);

    ipv6.NewNetwork();
    Ipv6InterfaceContainer wsnDeviceInterfaces;
    wsnDeviceInterfaces = ipv6.Assign (sixLowPanNetDevices);
    wsnDeviceInterfaces.SetForwarding (centerNode, true);
    wsnDeviceInterfaces.SetDefaultRouteInAllNodes (centerNode);

    for (uint32_t i = 0; i < sixLowPanNetDevices.GetN (); i++) {
        Ptr<NetDevice> dev = sixLowPanNetDevices.Get (i);
        dev->SetAttribute ("UseMeshUnder", BooleanValue (true));
        dev->SetAttribute ("MeshUnderRadius", UintegerValue (10));
    }



    vector<uint> sources;
    for( uint i=1; i<=n_nodes; i++ ) sources.push_back(i);

    for( uint i=1, sourceNode=0; i<=n_flows; ) {
        OnOffHelper sourceApp ("ns3::TcpSocketFactory",
                                    Inet6SocketAddress (wiredDeviceInterfaces.GetAddress (0, 1), 
                                    sinkPort));
        sourceApp.SetAttribute ("PacketSize", UintegerValue (pkt_sz));
        sourceApp.SetAttribute ("MaxBytes", UintegerValue (0));
        sourceApp.SetConstantRate (DataRate (pkt_sz*pkts_ps*8));
        ApplicationContainer sourceApps = sourceApp.Install (wirelessNodes.Get (sources[sourceNode]));
        sourceApps.Start (Seconds (start_time+10));
        sourceApps.Stop (Seconds (stop_time-10));

        PacketSinkHelper sinkApp ("ns3::TcpSocketFactory",
        Inet6SocketAddress (Ipv6Address::GetAny (), sinkPort++));
        sinkApp.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
        ApplicationContainer sinkApps = sinkApp.Install (wiredNodes.Get(0));
        sinkApps.Start (Seconds (start_time+5));
        sinkApps.Stop (Seconds (stop_time-5));

        i++;
        if( sourceNode==sources.size()-1 ) sourceNode=0;
        else sourceNode++;
    }

    FlowMonitorHelper flowHelper;
    flowHelper.InstallAll ();

    Simulator::Stop (Seconds (stop_time));
    Simulator::Run ();

    flowHelper.SerializeToXmlFile (filePrefix + ".flowmonitor", true, true);

    Simulator::Destroy ();

    return 0;
}

