#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
using namespace std;
using namespace ns3;

/*
    CSMA0 --- R0 ---- R2 --- CSMA2
                \    /
                 \  /
                  R1 --- CSMA1
*/

uint n_flows=20, n_nodes=40, n_routers=3, n_csma0, n_csma1, n_csma2;
uint start_time=0, stop_time, duration=60, pkts_ps=100, pkt_sz=1024;
uint sinkPort=9;
bool verbose = false;
NodeContainer routers, csmaNodes0, csmaNodes1, csmaNodes2;
NetDeviceContainer r0r1Net, r0r2Net, r1r2Net, csma0Net, csma1Net, csma2Net;
Ipv4InterfaceContainer r0r1Addr, r0r2Addr, r1r2Addr, csma0Addr, csma1Addr, csma2Addr;
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
    cmd.Parse (argc, argv);

    if( n_nodes<10 ) {
        std::cout << "Minimum 10 Nodes Required\n";
        exit(0);
    }
    else if( n_flows<5 ) {
        std::cout << "Minimum 5 Flows Required\n";
        exit(0);
    }
    else if( duration<10 ) {
        std::cout << "Minimum 10s Duration Required\n";
        exit(0);
    }

    std::cout << "\n--------------------------------------------------------------------------\n";
    std::cout << "Nodes: " << n_nodes << ", Flows: " << n_flows << ", Packets PPS: " << pkts_ps;
    std::cout << "\n--------------------------------------------------------------------------\n";

    n_csma0 = n_csma1 = n_nodes/3;
    n_csma2 = n_nodes - (n_csma0 + n_csma1);

    if(verbose) LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    stop_time = start_time + duration + 2;

    filePrefix = "w-"+to_string(n_nodes)+"-"+to_string(n_flows)+"-"+to_string(pkts_ps); 

    congestionAlgo = "ns3::" + congestionAlgo;
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue (congestionAlgo));
    Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType", StringValue (recoveryAlgo));  
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
    Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (true));
    Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue (QueueSize ("10p")));
    Config::SetDefault ("ns3::FifoQueueDisc::MaxSize", QueueSizeValue (QueueSize ("100p")));
}

void buildTopology() {
    routers.Create (3);
    csmaNodes0.Add(routers.Get(0));
    csmaNodes0.Create(n_csma0);
    csmaNodes1.Add(routers.Get(1));
    csmaNodes1.Create(n_csma1);
    csmaNodes2.Add(routers.Get(2));
    csmaNodes2.Create(n_csma2);

    InternetStackHelper internet;
    internet.InstallAll ();

    PointToPointHelper r2rHelper;
    r2rHelper.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
    r2rHelper.SetChannelAttribute ("Delay", StringValue ("0.01ms"));
    r0r1Net = r2rHelper.Install (routers.Get(0), routers.Get(1));
    r0r2Net = r2rHelper.Install (routers.Get(0), routers.Get(2));
    r1r2Net = r2rHelper.Install (routers.Get(1), routers.Get(2));

    CsmaHelper csmaHelper;
    csmaHelper.SetChannelAttribute ("DataRate", StringValue ("2Mbps"));
    csmaHelper.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
    csma0Net = csmaHelper.Install (csmaNodes0);
    csma1Net = csmaHelper.Install (csmaNodes1);
    csma2Net = csmaHelper.Install (csmaNodes2);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.0.0.0", "255.255.255.0");

    r0r1Addr = ipv4.Assign (r0r1Net);
    ipv4.NewNetwork ();
    r0r2Addr = ipv4.Assign (r0r2Net);
    ipv4.NewNetwork ();
    r1r2Addr = ipv4.Assign (r1r2Net);
    ipv4.NewNetwork ();
    csma0Addr = ipv4.Assign(csma0Net);
    ipv4.NewNetwork ();
    csma1Addr = ipv4.Assign(csma1Net);
    ipv4.NewNetwork ();
    csma2Addr = ipv4.Assign(csma2Net);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

void setUpSinks() {
    for( uint i=1; i<n_csma0; i++ ) {
        PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
        ApplicationContainer sinkApps = sinkHelper.Install(csmaNodes0.Get(i));
        sinkApps.Start (Seconds (start_time));
        sinkApps.Stop (Seconds (stop_time));
    }
    for( uint i=1; i<n_csma1; i++ ) {
        PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
        ApplicationContainer sinkApps = sinkHelper.Install(csmaNodes1.Get(i));
        sinkApps.Start (Seconds (start_time));
        sinkApps.Stop (Seconds (stop_time));
    }
    for( uint i=1; i<n_csma2; i++ ) {
        PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
        ApplicationContainer sinkApps = sinkHelper.Install(csmaNodes2.Get(i));
        sinkApps.Start (Seconds (start_time+1));
        sinkApps.Stop (Seconds (stop_time-1));
    }
}

void startSourceApp(InetSocketAddress sinkAddress, NodeContainer sourceNode) {
    OnOffHelper source ("ns3::TcpSocketFactory", sinkAddress);
    source.SetAttribute ("PacketSize", UintegerValue (pkt_sz));
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    source.SetConstantRate (DataRate (pkt_sz*pkts_ps*8));
    ApplicationContainer sourceApps = source.Install (sourceNode);
    sourceApps.Start (Seconds (start_time+3));
    sourceApps.Stop (Seconds(stop_time-3));
}

void setUpSources(uint csmaNo, uint n_src) {
    uint sourceCsma=1, sinkCsma, sinkNode;
    if( csmaNo==0 ) {
        while(n_src>0) {
            sinkCsma = rand()%2;
            // sink is in CSMA 1
            if( sinkCsma==0 ) {
                sinkNode = rand()%(n_csma1-1)+1;
                startSourceApp(InetSocketAddress (csma1Addr.GetAddress(sinkNode), sinkPort), 
                               csmaNodes0.Get (sourceCsma));
            }
            // sink is in CSMA 2
            else if( sinkCsma==1 ) {
                sinkNode = rand()%(n_csma2-1)+1;
                startSourceApp(InetSocketAddress (csma2Addr.GetAddress(sinkNode), sinkPort), 
                               csmaNodes0.Get (sourceCsma));
            }
            
            
            if( sourceCsma==n_csma0-1 ) sourceCsma = 1;
            else sourceCsma++;
            n_src--;
        }
    }
    else if( csmaNo==1 ) {
        while(n_src>0) {
            sinkCsma = rand()%2;
            // sink is in CSMA 0
            if( sinkCsma==0 ) {
                sinkNode = rand()%(n_csma0-1)+1;
                startSourceApp(InetSocketAddress (csma0Addr.GetAddress(sinkNode), sinkPort), 
                               csmaNodes1.Get (sourceCsma));
            }
            // sink is in CSMA 2
            else if( sinkCsma==1 ) {
                sinkNode = rand()%(n_csma2-1)+1;
                startSourceApp(InetSocketAddress (csma2Addr.GetAddress(sinkNode), sinkPort), 
                               csmaNodes1.Get (sourceCsma));
            }

            if( sourceCsma==n_csma1-1 ) sourceCsma = 1;
            else sourceCsma++;
            n_src--;
        }
    }
    else if( csmaNo==2 ) {
        while(n_src>0) {
            sinkCsma = rand()%2;
            // sink is in CSMA 0
            if( sinkCsma==0 ) {
                sinkNode = rand()%(n_csma0-1)+1;
                startSourceApp(InetSocketAddress (csma0Addr.GetAddress(sinkNode), sinkPort), 
                               csmaNodes2.Get (sourceCsma));
            }
            // sink is in CSMA 1
            else if( sinkCsma==1 ) {
                sinkNode = rand()%(n_csma1-1)+1;
                startSourceApp(InetSocketAddress (csma1Addr.GetAddress(sinkNode), sinkPort), 
                               csmaNodes2.Get (sourceCsma));
            }

            if( sourceCsma==n_csma2-1 ) sourceCsma = 1;
            else sourceCsma++;
            n_src--;
        }
    }
}

void setUpFlows() {
    setUpSinks();

    uint n_src_csma0, n_src_csma1, n_src_csma2;
    n_src_csma0 = n_src_csma1 = n_flows/3;
    n_src_csma2 = n_flows - (n_src_csma0+n_src_csma1);

    srand(time(NULL));

    setUpSources(0, n_src_csma0);
    setUpSources(1, n_src_csma1);
    setUpSources(2, n_src_csma2);
}

int main(int argc, char** argv) {
    processArguments(argc, argv);
    buildTopology();
    setUpFlows();

    FlowMonitorHelper flowHelper;
    flowHelper.InstallAll ();

    Simulator::Stop (Seconds (stop_time));
    Simulator::Run ();

    flowHelper.SerializeToXmlFile (filePrefix+".flowmonitor", true, true);

    Simulator::Destroy ();

    return 0;
}