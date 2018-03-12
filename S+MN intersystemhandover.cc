
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("S+MN intersystem handover");

void
NotifyConnectionEstablishedUe (std::string context,
                               uint64_t imsi,
                               uint16_t cellid,
                               uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " MU " << imsi
            << ": connected to S+ eNB " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti,
                       uint16_t targetCellId)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " MU " << imsi
            << ": previously connected to S+eNB " << cellid
            << " with RNTI " << rnti
            << ", doing handover to S+eNB " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " MU " << imsi
            << ": successful handover to S+eNB " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyConnectionEstablishedEnb (std::string context,
                                uint64_t imsi,
                                uint16_t cellid,
                                uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " S+AP " << cellid
            << ": successful connection of MU " << imsi
            << " RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti,
                        uint16_t targetCellId)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " S+AP " << cellid
            << ": start handover of MU " << imsi
            << " RNTI " << rnti
            << " to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " S+AP " << cellid
            << ": completed handover of MU " << imsi
            << " RNTI " << rnti
            << std::endl;
}

int main (int argc, char *argv[])
{

  uint16_t numberOfMus = 1;
  uint16_t numberOfEnbs = 2;
  uint16_t numBearersPerMu = 2;
  double simTime = 0.0300;
  double distance = 10.0;

  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MilliSeconds (10)));
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue (1000));
  Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (false));

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("numberOfMUs", "Number of MUs", numberOfMus);
  cmd.AddValue ("numberOfEnbs", "Number of eNodeBs", numberOfEnbs);
  cmd.AddValue ("simTime", "Total duration of the simulation (in seconds)", simTime);
  cmd.Parse (argc, argv);


  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");
  lteHelper->SetHandoverAlgorithmType ("ns3::NoOpHandoverAlgorithm"); // disable automatic handover

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("10Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (100));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.000005)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);


  // Routing of the Internet Host (towards the LTE network)
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  // interface 0 is localhost, 1 is the p2p device
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer muNodes;
  NodeContainer enbNodes;
  enbNodes.Create (numberOfEnbs);
  muNodes.Create (numberOfMUs);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfEnbs; i++)
    {
      positionAlloc->Add (Vector (distance * 2 * i - distance, 0, 0));
    }
  for (uint16_t i = 0; i < numberOfMUs; i++)
    {
      positionAlloc->Add (Vector (0, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (enbNodes);
  mobility.Install (muNodes);

  // Install 5G Devices in eNB and MUs
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer muLteDevs = lteHelper->InstallMuDevice (muNodes);

  // Install the IP stack on the MUs
  internet.Install (muNodes);
  Ipv4InterfaceContainer MuIpIfaces;
  ueIpIfaces = epcHelper->AssignMuIpv4Address (NetDeviceContainer (muLteDevs));


  // Attach all MUs to the first eNodeB
  for (uint16_t i = 0; i < numberOfMus; i++)
    {
      lteHelper->Attach (muLteDevs.Get (i), enbLteDevs.Get (0));
    }


  NS_LOG_LOGIC (" application set up");
  
  // Install and start applications on MUs and remote host
  uint16_t dlPort = 10000;
  uint16_t ulPort = 20000;

  // randomize a bit start times to avoid simulation artifacts
  // (e.g., buffer overflows due to packet transmissions happening
  // exactly at the same time)
  Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
  startTimeSeconds->SetAttribute ("Min", DoubleValue (0));
  startTimeSeconds->SetAttribute ("Max", DoubleValue (0.010));

  for (uint32_t m = 0; m < numberOfMus; ++m)
    {
      Ptr<Node> mu = muNodes.Get (m);
      // Set the default gateway for the MU
      Ptr<Ipv4StaticRouting> muStaticRouting = ipv4RoutingHelper.GetStaticRouting (mu->GetObject<Ipv4> ());
     muStaticRouting->SetDefaultRoute (epcHelper->GetMuDefaultGatewayAddress (), 1);

      for (uint32_t b = 0; b < numBearersPerMu; ++b)
        {
          ++dlPort;
          ++ulPort;

          ApplicationContainer clientApps;
          ApplicationContainer serverApps;

          NS_LOG_LOGIC ("installing UDP DL app for MU " << m);
          UdpClientHelper dlClientHelper (muIpIfaces.GetAddress (m), dlPort);
          clientApps.Add (dlClientHelper.Install (remoteHost));
          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), dlPort));
          serverApps.Add (dlPacketSinkHelper.Install (mu));

          NS_LOG_LOGIC ("installing UDP UL app for MU " << m);
          UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
          clientApps.Add (ulClientHelper.Install (mu));
          PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), ulPort));
          serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

          Ptr<EpcTft> tft = Create<EpcTft> ();
          EpcTft::PacketFilter dlpf;
          dlpf.localPortStart = dlPort;
          dlpf.localPortEnd = dlPort;
          tft->Add (dlpf);
          EpcTft::PacketFilter ulpf;
          ulpf.remotePortStart = ulPort;
          ulpf.remotePortEnd = ulPort;
          tft->Add (ulpf);
          EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
          lteHelper->ActivateDedicatedEpsBearer (ueLteDevs.Get (m), bearer, tft);

          Time startTime = Seconds (startTimeSeconds->GetValue ());
          serverApps.Start (startTime);
          clientApps.Start (startTime);

        } // end for b
    }


  // Add X2 inteface
  lteHelper->AddX2Interface (enbNodes);

  // X2-based Handover
  lteHelper->HandoverRequest (Seconds (0.000100), ueLteDevs.Get (0), enbLteDevs.Get (0), enbLteDevs.Get (1));

  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-x2-handover");

  lteHelper->EnablePhyTraces ();
  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();
  lteHelper->EnablePdcpTraces ();
  Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats ();
  rlcStats->SetAttribute ("EpochDuration", TimeValue (Seconds (0.0005)));
  Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats ();
  pdcpStats->SetAttribute ("EpochDuration", TimeValue (Seconds (0.0005)));


  // connect custom trace sinks for RRC connection establishment and handover notification
  Config::Connect ("/NodeList/*/DeviceList/*/5GEnbRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/5GUeRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedUe));
  Config::Connect ("/NodeList/*/DeviceList/*/5GEnbRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/5GUeRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartUe));
  Config::Connect ("/NodeList/*/DeviceList/*/5GEnbRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/5GUeRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkUe));




  Simulator::Stop (Seconds (simTime));
  
AnimationInterface anim ("5g.xml");
  Simulator::Run ();

  // GtkConfigStore config;
  // config.ConfigureAttributes ();

  Simulator::Destroy ();
  return 0;

}
