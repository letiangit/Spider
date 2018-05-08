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

#include <stdlib.h>
using namespace std;
using namespace ns3;

uint32_t ChannelNum = 0;
string m_Outputpath;

void
ChannelSelection  (std::string contex, Mac48Address addr, Time ts, uint32_t rx, uint32_t tx)
{
    ChannelNum++;
    std::ofstream myfile;
    std::string dropfile=m_Outputpath;
    myfile.open (dropfile, ios::out | ios::app);
    myfile << ChannelNum << ", device " << addr << " at time " << ts  << " select chanel rx " <<  rx << ", tx " << tx << "\n";
    myfile.close();
}

void
RecPacket  (std::string contex, Mac48Address addr, Mac48Address from, Mac48Address to, Time ts, uint32_t size, uint32_t qos, uint32_t id)
{
    std::ofstream myfile;
    std::string dropfile=m_Outputpath;
    myfile.open (dropfile, ios::out | ios::app);
    myfile << ts << "\t"  << addr << " ------- receives packet from "  << from  << " to " << to  << ", size "  <<  size << ",  qos " << qos << ", id " << id<< "\n" ;
    myfile.close();
}




void
RecPacketARQ  (std::string contex, Mac48Address addr, Mac48Address from, Ptr<Packet> packet, Time ts, uint32_t size, uint32_t protocol,  uint32_t packetid)
{
    std::ofstream myfile;
    std::string dropfile=m_Outputpath;
    myfile.open (dropfile, ios::out | ios::app);
    myfile << ts << "\t"  << addr << " receives ARQ packet from "  << from  << ", size "  <<  size << ",  qos " << protocol << ", packet id " << packetid << "\n";
    myfile.close();
}

void
TxPacketARQ  (std::string contex, Mac48Address addr, Mac48Address to, uint32_t id, uint32_t qos,  uint32_t retransnum, bool success)
{
    std::ofstream myfile;
    std::string dropfile=m_Outputpath;
    myfile.open (dropfile, ios::out | ios::app);
    myfile << Simulator::Now () << "\t"  << addr << " send ARQ packet to "  << to  << ", id "  <<  id << ",  qos " << qos << ", retransnum " << retransnum << " success " << success << "\n";
    myfile.close();
}

void
RecPacketGES  (std::string contex, Mac48Address addr, Mac48Address from, Mac48Address to, Time ts, uint32_t size, uint32_t protocol, Mac48Address linkedLEO)
{
    std::ofstream myfile;
    std::string dropfile=m_Outputpath;
    myfile.open (dropfile, ios::out | ios::app);
    myfile << ts << "\t GES "  << addr << " receives packet from "  << from  << " to " << to  << ", size "  <<  size << ",  protocol " << protocol << ", visalbe to " <<  linkedLEO << "\n";
    myfile.close();
}


void trafficGeneration (NodeContainer nodes, uint32_t Nnodes)
{
    SpiderClient clientApp;
    
    clientApp.SetRemote (Mac48Address::ConvertFrom (nodes.Get(0)->GetDevice(0)->GetAddress()) );
    
    clientApp.SetMaxPackets (1000);
    clientApp.SetInterval (Seconds(100));
    clientApp.SetPacketSize (2000);
    clientApp.InstallDevice (nodes.Get(8)->GetDevice(0));
    
    //NGESnodes; nodesGES
    
    SpiderServer ServerApp;
    ServerApp.SetRemote (Mac48Address::ConvertFrom (nodes.Get(8)->GetDevice(0)->GetAddress()) );
    ServerApp.InstallDevice (nodes.Get(0)->GetDevice(0));
    
    clientApp.StartApplication (Seconds (0.0));
    clientApp.StopApplication (Seconds (1000.0));
    
    ServerApp.StartApplication (Seconds (0.0));
    ServerApp.StopApplication (Seconds (1000.0));
}


void
PopulateArpCache ()
{
    Ptr<ArpCache> arp = CreateObject<ArpCache> ();
    arp->SetAliveTimeout (Seconds(3600 * 24 * 365));
    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
        NS_ASSERT(ip !=0);
        ObjectVectorValue interfaces;
        ip->GetAttribute("InterfaceList", interfaces);
        for(ObjectVectorValue::Iterator j = interfaces.Begin(); j !=
            interfaces.End (); j ++)
        {
            Ptr<Ipv4Interface> ipIface = (j->second)->GetObject<Ipv4Interface> ();
            NS_ASSERT(ipIface != 0);
            Ptr<NetDevice> device = ipIface->GetDevice();
            NS_ASSERT(device != 0);
            Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress ());
            for(uint32_t k = 0; k < ipIface->GetNAddresses (); k ++)
            {
                Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal();
                if(ipAddr == Ipv4Address::GetLoopback())
                continue;
                ArpCache::Entry * entry = arp->Add(ipAddr);
                entry->MarkWaitReply(0);
                entry->MarkAlive(addr);
                std::cout << "Arp Cache: Adding the pair (" << addr << "," << ipAddr << ")" << std::endl;
            }
        }
    }
    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
        NS_ASSERT(ip !=0);
        ObjectVectorValue interfaces;
        ip->GetAttribute("InterfaceList", interfaces);
        for(ObjectVectorValue::Iterator j = interfaces.Begin(); j !=interfaces.End (); j ++)
        {
            Ptr<Ipv4Interface> ipIface = (j->second)->GetObject<Ipv4Interface> ();
            ipIface->SetAttribute("ArpCache", PointerValue(arp));
        }
    }
}


int
main (int argc, char *argv[])
{
    //LogComponentEnable ("Ipv4Interface", LOG_LOGIC);
    //LogComponentEnable ("ArpL3Protocol", LOG_LOGIC);
    //LogComponentEnable ("Ipv4L3Protocol", LOG_LOGIC);
    //LogComponentEnable ("SpiderClient", LOG_INFO);

 	uint32_t seed = 1;
  	uint32_t Nnodes = 10;
  	uint32_t RestartTimeout = 2000;
  	uint32_t PacketIntervalCh = 10;
  	uint32_t CycelIntervalCh = 20;
  	uint32_t backoffcounter = 1000;
  	uint32_t NexternalSel = 1;
  
  	bool Unidirectional = false;
  	bool NominalMode = true;
	
  string Outputpath = "output.txt";
  string OutputpathSender;
    
  string DataRate; // nominal mode, 1.1 Mbps (failure mode)
  string Delay;
  uint32_t PACKETREPEATNUMBER;
  uint32_t Nchannel = 4;
  uint32_t  ARQTimeout = 1000;
  uint32_t  ARQBufferSize = 63;
  uint32_t  SimStart = 30;
  uint32_t  SimEnd = 60;
  uint32_t  ImageSateID = 5;

  uint32_t ImageBundles = 20;
  uint32_t PriorityBundles = 1;
  uint32_t PriorityDelay = 5;
  uint32_t ApplicationQueueSize = 2147483648;
  uint32_t ForwardingQueueSize = 2147483648;
  uint32_t  m_InterfaceNum;
  bool LoadBalance = false;
  bool HandOver = false;
  uint32_t HandOverInterval = 31;//seconds
  bool TMTCEnabled = false;
  uint32_t TMTCMaxDelay = 5;
  bool ImageEnabled = true;
    

  
  CommandLine cmd;
	cmd.AddValue ("Unidirectional", "true in case of unidirectional, false in case of bidirectional", Unidirectional);
	cmd.AddValue ("NominalMode", "true in case of nominal mode, false otherwise", NominalMode);
	cmd.AddValue ("ARQTimeout", "path of channel selection result", ARQTimeout);
	cmd.AddValue ("ARQWindow", "path of channel selection result", ARQBufferSize);
	cmd.AddValue ("QueueSize", "size of the forwarding queues", ForwardingQueueSize);

  	cmd.AddValue ("ImageSateID", "path of channel selextion result", ImageSateID);
  	cmd.AddValue ("ImageBundles", "the number of image bundles to transmit", ImageBundles);
  	cmd.AddValue ("PriorityBundles", "the number of priority bundles to transmit", PriorityBundles);
  	cmd.AddValue ("PriorityDelay", "the delay for sending the priority bundles in ms", PriorityDelay);
    
    cmd.AddValue ("Outputpath", "path of channel selextion result", Outputpath);
    cmd.AddValue ("OutputpathSender", "path of channel selextion result", OutputpathSender);
    //cmd.AddValue ("QueueLenpath", "path of channel selextion result", QueueLenpath);
    cmd.AddValue ("LoadBalance", "path of channel selextion result", LoadBalance);
    cmd.AddValue ("HandOver", "path of channel selextion result", HandOver);
    cmd.AddValue ("HandOverInterval", "path of channel selextion result", HandOverInterval);
	cmd.AddValue("TMTCEnabled", "Enable or disable TMTC traffic", TMTCEnabled);
	cmd.AddValue("ImageEnabled", "Enable or disable image and priority traffic", ImageEnabled);
	cmd.AddValue("TMTCMaxDelay", "Maximum delay from start of image generation until when TMTC traffic can be generated", TMTCMaxDelay);


    



  cmd.Parse (argc, argv);
  RngSeedManager::SetSeed (seed);
    
  Time::SetResolution (Time::MS);
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
  m_Outputpath = Outputpath;
  //m_QueueLenpath = QueueLenpath;

  NodeContainer nodes;
  nodes.Create (Nnodes);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (DataRate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (Delay));
  pointToPoint.SetChannelAttribute ("UniChannel", BooleanValue (Unidirectional));
  
  pointToPoint.SetDeviceAttribute ("ChannelResp_delay", TimeValue (MilliSeconds (RestartTimeout)));
  pointToPoint.SetDeviceAttribute ("ChannelConf_delay", TimeValue (MilliSeconds (RestartTimeout)));
    
  pointToPoint.SetDeviceAttribute ("Channel_delay_packet", TimeValue (MilliSeconds (PacketIntervalCh)));
  pointToPoint.SetDeviceAttribute ("ChannelWaiting_Interval", TimeValue (MilliSeconds (CycelIntervalCh)));
  pointToPoint.SetDeviceAttribute ("PACKETREPEATNUMBER", UintegerValue (PACKETREPEATNUMBER));
  // channel request packet transmission time, 21 bytes/DataRate ~= 0.13 ms
  pointToPoint.SetDeviceAttribute ("backoffcounter", UintegerValue (backoffcounter)); //in the unit of ms
  pointToPoint.SetDeviceAttribute ("ARQAckTimeout", TimeValue (MilliSeconds(ARQTimeout))); //in the unit of seconds
  pointToPoint.SetDeviceAttribute ("ARQBufferSize", UintegerValue (ARQBufferSize) ); //in the unit of ms
  pointToPoint.SetDeviceAttribute ("m_InterfaceNum", UintegerValue (m_InterfaceNum) );
  pointToPoint.SetDeviceAttribute ("LoadBalance", BooleanValue (LoadBalance) );


    
    
    
 uint32_t nodei, nodej;
 NetDeviceContainer devicesSet;
 for (uint32_t kk = 0; kk < Nnodes; kk++)
   {
       nodei = kk;
       nodej = (kk + 1) % Nnodes;
       if  (Unidirectional)
       {
          devicesSet = pointToPoint.InstallUni (nodes.Get(nodei), nodes.Get(nodej));
       }
       else
       {
         devicesSet = pointToPoint.InstallBi (nodes.Get(nodei), nodes.Get(nodej));
       }
    }
    
  //NS_LOG_UNCOND ("install devicesSet finish " << nodes.Get(Nnodes-1)->GetId () << "\t" << nodes.Get(0)->GetId ()  << ", size " << devicesSet.GetN () );
  //install GES and LEO
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
    

    uint32_t GESPos[NGESnodes];
    GESPos[0]=0b0000000001;
    GESPos[1]=0b0100000000;
    if (HandOver)
      pointToPoint.InitTopologyGES (Nnodes, NGESnodes, GESPos, Seconds (HandOverInterval));
    else
      pointToPoint.InitTopologyGES (Nnodes, NGESnodes, GESPos, Seconds (SimEnd));



  for (uint32_t kk = 0; kk < NexternalSel; kk++)
    {
      std::ostringstream STA;
      STA << kk;
      std::string strSTA = STA.str();
        
      Config::Set ("/NodeList/"+strSTA+"/DeviceList/0/$ns3::PointToPointNetDevice/StartChannelSelection", BooleanValue(true)); //node  receive external command for channel selection
    }
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ChannelSelected", MakeCallback (&ChannelSelection));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/RecPacketTrace", MakeCallback (&RecPacket));
  
    //NS_LOG_UNCOND ("Start channel selection ----------- Nnodes 3" );
    
    
  Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDevice/TxQueue/MaxPackets", UintegerValue (100)); //DropTailQueue
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ARQRecPacketTrace", MakeCallback (&RecPacketARQ));
    
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ARQTxPacketTrace", MakeCallback (&TxPacketARQ));
    
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
    
    
    
    
    Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/RecPacketTrace", MakeCallback (&RecPacketGES));
    Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ARQRecPacketTrace", MakeCallback (&RecPacketARQ));
    Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ARQTxPacketTrace", MakeCallback (&TxPacketARQ));
    
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
    Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDeviceGES/ARQBufferSize", UintegerValue (ARQBufferSize)); //DropTailQueue
    
    
    


    
//***********************************************
    //GES is last node
    
   //For the low proority, burst of 20
	uint32_t tmtcOffsetMs = 0;
	if (TMTCMaxDelay > 0)
		tmtcOffsetMs = rand() % (TMTCMaxDelay * 1000);
	
	BulkImageClient* client = new BulkImageClient;
   	client->SetRemote (Mac48Address::ConvertFrom (nodesGES.Get(0)->GetDevice(0)->GetAddress()));
	client->SetOutputpath (OutputpathSender);
	client->SetImageBundleCount(ImageBundles);
	client->SetPriorityBundleCount(PriorityBundles);
	client->SetPriorityDelay(Seconds(PriorityDelay));
	client->SetTMTCDelay(MilliSeconds(tmtcOffsetMs));
	client->EnableTMTC(TMTCEnabled);
	client->EnableImage(ImageEnabled);
	client->EnablePriority(ImageEnabled);
   	client->StartApplication (Seconds (SimStart));
   	client->StopApplication (Seconds (SimStart + PriorityDelay + 1));
	client->InstallDevice (nodes.Get(ImageSateID)->GetDevice(0));
	
	if (TMTCEnabled) {
	    for (uint16_t kk = 0; kk < Nnodes ; kk++)
	    {
			if (kk != ImageSateID) {
	 	       uint32_t offsetMs = 0;
			   if (TMTCMaxDelay > 0)
			   		offsetMs = rand() % (TMTCMaxDelay * 1000);
			   cout << "offset " << kk << ": " << offsetMs << endl;
	 	      //SpiderClient clientAppTMC = crate<clientAppTMC>; Create<clientAppTMC> ()
	 	      SpiderClient * clientAppTMC = new SpiderClient;
	 	      clientAppTMC->SetRemote (Mac48Address::ConvertFrom (nodesGES.Get(0)->GetDevice(0)->GetAddress()) );

	 	      clientAppTMC->SetMaxPackets (2147483648);
	 	      clientAppTMC->SetInterval (Seconds(SimEnd)); //5s
	 	      clientAppTMC->SetPacketSize (5120); //5k
	 	      clientAppTMC->SetQos (3);
	 	      clientAppTMC->SetConstantRate (true);
	 	      clientAppTMC->SetOutputpath (OutputpathSender);

	 	      clientAppTMC->InstallDevice (nodes.Get(kk)->GetDevice(0));
        
	 	       clientAppTMC->StartApplication (Seconds (SimStart) + MilliSeconds(offsetMs));
	 	       clientAppTMC->StopApplication (Seconds (SimEnd));
			}
	      }
	}
  
  std::ofstream myfile;
  std::string dropfile=m_Outputpath;
  myfile.open (dropfile, ios::out | ios::trunc);
  myfile << "Start channel selection ----------- Nnodes " << Nnodes << ", Ndevice " << devicesSet.GetN () << ", UniChannel " << Unidirectional << "\n";
  myfile.close();
  /*
  for (uint32_t kk = 0; kk < Nnodes; kk++)
    {
        if (!UniChannel)
          NS_LOG_UNCOND( "LEO ----------- Nnodes " << nodes.Get(kk)->GetId () << ", device " << nodes.Get(kk)->GetDevice(0)->GetAddress() << ",  " << nodes.Get(kk)->GetDevice(1)->GetAddress()  << ",  " << nodes.Get(kk)->GetDevice(2)->GetAddress() );
        else
          NS_LOG_UNCOND( "LEO ----------- Nnodes " << nodes.Get(kk)->GetId () << ", device " << nodes.Get(kk)->GetDevice(0)->GetAddress() << ",  " << nodes.Get(kk)->GetDevice(1)->GetAddress()  );
    }
    
  for (uint32_t ii = 0; ii < NGESnodes; ii++)
    {
        NS_LOG_UNCOND( "GES ----------- Nnodes " << nodesGES.Get(ii)->GetId () << ", device " << nodesGES.Get(ii)->GetDevice(0)->GetAddress()  );
    }

    
  NS_LOG_UNCOND ("install devicesSet finish ....."  );
    */
    
  Simulator::Stop (Seconds (SimEnd));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}


