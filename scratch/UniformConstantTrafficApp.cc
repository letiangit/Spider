/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"


#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <fstream>

using namespace std;
using namespace ns3;

uint32_t ChannelNum = 0;

int
main (int argc, char *argv[])
{
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_WARN);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_WARN);
    LogComponentEnable ("PointToPointNetDevice", LOG_LEVEL_WARN);
    LogComponentEnable ("PointToPointNetDeviceGES", LOG_LEVEL_WARN);
    LogComponentEnable ("PointToPointChannelGES", LOG_LEVEL_WARN);
    LogComponentEnable ("PointToPointChannel", LOG_LEVEL_WARN);
    LogComponentEnable ("PointToPointRemoteChannel", LOG_LEVEL_WARN);
    LogComponentEnable ("SpiderClient", LOG_LEVEL_WARN);
    LogComponentEnable ("SpiderServer", LOG_LEVEL_WARN);
    LogComponentEnable ("BulkImageClient", LOG_LEVEL_WARN);
    LogComponentEnable ("PointToPointHelper", LOG_LEVEL_WARN);
    
		
    // channel selection parameters
 	uint32_t seed = 1;
  	uint32_t Nnodes = 10;
  	uint32_t RestartTimeout = 2000;
  	uint32_t PacketIntervalCh = 10;
  	uint32_t CycelIntervalCh = 20;
  	uint32_t backoffcounter = 1000;
  	uint32_t NexternalSel = 1;
  
  	bool UniChannel = false;
  	bool NominalMode = true;
	bool LoadBalance = false;
    
  	string DataRate; // nominal mode, 1.1 Mbps (failure mode)
  	string Delay;
  	uint32_t PACKETREPEATNUMBER;
  	uint32_t Nchannel = 4;
  	uint32_t  ARQTimeout = 1000;
  	uint32_t  ARQWindow = 63;
	uint32_t PacketsPerSecond = 100;
	uint32_t SimulationTime = 30;
	uint32_t ApplicationQueueSize = 2147483648;
	uint32_t ForwardingQueueSize = 2147483648;
    uint32_t  m_InterfaceNum;
	uint32_t FirstNode = 0;
	uint32_t LastNode = Nnodes - 1;

    bool HandOver = false;
    uint32_t HandOverInterval = 31;//seconds

  
  	CommandLine cmd;
	cmd.AddValue ("Unidirectional", "true in case of unidirectional, false in case of bidirectional", UniChannel);
	cmd.AddValue ("NominalMode", "true in case of nominal mode, false otherwise", NominalMode);
  	cmd.AddValue ("ARQTimeout", "path of channel selection result", ARQTimeout);
  	cmd.AddValue ("ARQWindow", "path of channel selection result", ARQWindow);
	cmd.AddValue ("QueueSize", "size of the forwarding queues", ForwardingQueueSize);
  	cmd.AddValue ("PacketsPerSecond", "Number of packets to transmit per second", PacketsPerSecond);
  	cmd.AddValue ("SimulationTime", "maximum simulation time", SimulationTime);
    cmd.AddValue ("LoadBalance", "path of channel selextion result", LoadBalance);
    cmd.AddValue ("HandOver", "path of channel selextion result", HandOver);
    cmd.AddValue ("HandOverInterval", "path of channel selextion result", HandOverInterval);
    cmd.AddValue ("FirstNode", "First node that transmits", FirstNode);
    cmd.AddValue ("LastNode", "Last node that transmits", LastNode);
	


  cmd.Parse (argc, argv);
  RngSeedManager::SetSeed (seed);
    
  Time::SetResolution (Time::NS);
    
  //Outputpath = "./OptimalRawGroup/result.txt";
    
  PACKETREPEATNUMBER = Nchannel * CycelIntervalCh/PacketIntervalCh;
  
  /*
  if (Nnodes == 11)
    {
        NominalMode = true;
    }
  else if (Nnodes == 10)
    {
        NominalMode = false;
    }
  else
    {
        return Nnodes;
    }
    */
  
  if (NominalMode)
    {
		Nnodes = 10;
        DataRate = "1.3Mbps"; // nominal mode, 1.1 Mbps (failure mode)
        Delay = "14.4ms";
    }
    else
    {
		Nnodes = 9;
        DataRate = "1.1Mbps"; // nominal mode, 1.1 Mbps (failure mode)
        Delay = "17.2ms";
    }
  m_InterfaceNum = Nnodes * 2;

  NodeContainer nodes;
  nodes.Create (Nnodes);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (DataRate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (Delay));
  pointToPoint.SetChannelAttribute ("UniChannel", BooleanValue (UniChannel));
  
  pointToPoint.SetDeviceAttribute ("ChannelResp_delay", TimeValue (MilliSeconds (RestartTimeout)));
  pointToPoint.SetDeviceAttribute ("ChannelConf_delay", TimeValue (MilliSeconds (RestartTimeout)));
    
  pointToPoint.SetDeviceAttribute ("Channel_delay_packet", TimeValue (MilliSeconds (PacketIntervalCh)));
  pointToPoint.SetDeviceAttribute ("ChannelWaiting_Interval", TimeValue (MilliSeconds (CycelIntervalCh)));
  pointToPoint.SetDeviceAttribute ("PACKETREPEATNUMBER", UintegerValue (PACKETREPEATNUMBER));
  // channel request packet transmission time, 21 bytes/DataRate ~= 0.13 ms
  pointToPoint.SetDeviceAttribute ("backoffcounter", UintegerValue (backoffcounter)); //in the unit of ms
  pointToPoint.SetDeviceAttribute ("ARQAckTimeout", TimeValue (MilliSeconds(ARQTimeout))); //in the unit of seconds
  pointToPoint.SetDeviceAttribute ("ARQBufferSize", UintegerValue (ARQWindow) ); //in the unit of ms
  pointToPoint.SetDeviceAttribute ("m_InterfaceNum", UintegerValue (m_InterfaceNum) );
  pointToPoint.SetDeviceAttribute ("LoadBalance", BooleanValue (LoadBalance) );

    
 uint32_t nodei, nodej;
 NetDeviceContainer devicesSet;
 for (uint32_t kk = 0; kk < Nnodes; kk++)
   {
       nodei = kk;
       nodej = (kk + 1) % (Nnodes);
       if  (UniChannel)
       {
          devicesSet = pointToPoint.InstallUni (nodes.Get(nodei), nodes.Get(nodej));
       }
       else
       {
         devicesSet = pointToPoint.InstallBi (nodes.Get(nodei), nodes.Get(nodej));
       }
    }
	
    //pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    //pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
    //devicesSet = pointToPoint.InstallBi (nodes.Get(Nnodes - 2), nodes.Get(Nnodes - 1));
	
    
  //NS_LOG_DEBUG ("install devicesSet finish " << nodes.Get(Nnodes-1)->GetId () << "\t" << nodes.Get(0)->GetId ()  << ", size " << devicesSet.GetN () );
  //install GES and LE
	
  NodeContainer nodesGES;
  uint32_t NGESnodes = 2;
  nodesGES.Create (NGESnodes);
  NetDeviceContainer devicesSetGES;
    
    for (uint32_t ii = 0; ii < NGESnodes; ii++)
    {
        for (uint32_t kk = 0; kk < Nnodes; kk++)
        {
            devicesSetGES = pointToPoint.InstallGES (nodesGES.Get(ii), nodes.Get(kk));
        }
    }
    
    //NS_LOG_DEBUG ("Start channel selection ----------- Nnodes 2 " );

    uint32_t GESPos[NGESnodes];
    GESPos[0]=0b000000001;
    GESPos[1]=0b100000000;
    
    if (HandOver)
      pointToPoint.InitTopologyGES (Nnodes, NGESnodes, GESPos, Seconds (HandOverInterval));
    else
      pointToPoint.InitTopologyGES (Nnodes, NGESnodes, GESPos, Seconds (30+SimulationTime));
	


  for (uint32_t kk = 0; kk < NexternalSel; kk++)
    {
      std::ostringstream STA;
      STA << kk;
      std::string strSTA = STA.str();
        
      Config::Set ("/NodeList/"+strSTA+"/DeviceList/0/$ns3::PointToPointNetDevice/StartChannelSelection", BooleanValue(true)); //node  receive external command for channel selection
    }
  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ChannelSelected", MakeCallback (&ChannelSelection));
  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/RecPacketTrace", MakeCallback (&RecPacket));
  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/RecPacketTrace", MakeCallback (&RecPacketGES));
    //NS_LOG_DEBUG ("Start channel selection ----------- Nnodes 3" );
    
    
  Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDevice/TxQueue/MaxPackets", UintegerValue (100)); //DropTailQueue
  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ARQRecPacketTrace", MakeCallback (&RecPacketARQ));
    
  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ARQTxPacketTrace", MakeCallback (&TxPacketARQ));
    
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueueQos3/MaxPackets", UintegerValue (ApplicationQueueSize)); //DropTailQueue
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueueQos2/MaxPackets", UintegerValue (ApplicationQueueSize)); //DropTailQueue
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueueQos1/MaxPackets", UintegerValue (ApplicationQueueSize)); //DropTailQueue
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueueQos0/MaxPackets", UintegerValue (ApplicationQueueSize)); //DropTailQueue
//2147483648
    
    //Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPacketNumber", UintegerValue(10));

    
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue4/MaxPackets", UintegerValue (100)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue3/MaxPackets", UintegerValue (ForwardingQueueSize)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue2/MaxPackets", UintegerValue (ForwardingQueueSize)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue1/MaxPackets", UintegerValue (ForwardingQueueSize)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue0/MaxPackets", UintegerValue (ForwardingQueueSize)); //DropTailQueue


    Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDeviceGES/TxQueue/MaxPackets", UintegerValue (100)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/TxQueueQos3/MaxPackets", UintegerValue (UintegerValue (ApplicationQueueSize))); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/TxQueueQos2/MaxPackets", UintegerValue (UintegerValue (ApplicationQueueSize))); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/TxQueueQos1/MaxPackets", UintegerValue (UintegerValue (ApplicationQueueSize))); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/TxQueueQos0/MaxPackets", UintegerValue (UintegerValue (ApplicationQueueSize))); //DropTailQueue
    
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ForwardQueue4/MaxPackets", UintegerValue (100)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ForwardQueue3/MaxPackets", UintegerValue (UintegerValue (ForwardingQueueSize))); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ForwardQueue2/MaxPackets", UintegerValue (UintegerValue (ForwardingQueueSize))); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ForwardQueue1/MaxPackets", UintegerValue (UintegerValue (ForwardingQueueSize))); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ForwardQueue0/MaxPackets", UintegerValue (UintegerValue (ForwardingQueueSize))); //DropTailQueue

    Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDeviceGES/ARQAckTimeout", TimeValue (MilliSeconds(ARQTimeout))); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDeviceGES/ARQBufferSize", UintegerValue (ARQWindow)); //DropTailQueue

    
//***********************************************
    //GES is last node
	
	for (uint32_t kk = FirstNode; kk <= LastNode; kk++) {
    	SpiderClient * simpleApp = new SpiderClient;
    	simpleApp->SetRemote (Mac48Address::ConvertFrom (nodesGES.Get(0)->GetDevice(0)->GetAddress()));
		float interval = 1000.0 * (LastNode - FirstNode + 1) / (1.0 * PacketsPerSecond);
		cout << kk << ": " << interval << endl;
    	simpleApp->SetMaxPackets (2147483648);
		simpleApp->SetInterval (NanoSeconds(interval * 1000000)); //5s
    	simpleApp->SetPacketSize (1024); //5k
    	simpleApp->SetQos (1);
    	simpleApp->SetConstantRate (true);
    	//simpleApp->SetOutputpath (OutputpathSender);

   	 	simpleApp->InstallDevice (nodes.Get(kk)->GetDevice(0));
        
    	simpleApp->StartApplication (Seconds (30.0));
     	simpleApp->StopApplication (Seconds (30+SimulationTime));
	}
    
    
    Simulator::Stop (Seconds (30+SimulationTime));


  	Simulator::Run ();
	Simulator::Destroy ();
	return 0;
}


