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
#include "ns3/data-rate.h"


#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <fstream>

using namespace std;
using namespace ns3;

uint32_t ChannelNum = 0;
string m_Outputpath;
string m_QueueLenpath;

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
ForwadQueue (std::string contex, uint32_t txTime, Mac48Address sate, uint32_t qos, uint32_t len)
{
    std::ofstream myfile;
    std::string dropfile=m_QueueLenpath;
    myfile.open (dropfile, ios::out | ios::app);
    myfile << txTime << "\t"  << sate << "\t"  << qos  << "\t"  <<  len  << "\n";
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
  uint32_t Nnodes = 11;
  uint32_t RestartTimeout = 2000;
  uint32_t PacketIntervalCh = 10;
  uint32_t CycelIntervalCh = 20;
  uint32_t backoffcounter = 1000;
  uint32_t NexternalSel = 1;
  bool UniChannel = true;
  bool NominalMode = true;
  string Outputpath;
  string OutputpathSender;
    
  string DataRate; // nominal mode, 1.1 Mbps (failure mode)
  string Delay;
  uint32_t PACKETREPEATNUMBER;
  uint32_t Nchannel = 4;
  uint32_t  ARQTimeout = 1000;
  uint32_t  ARQBufferSize = 31;
  //uint32_t  PoissonRate = 33;
  uint32_t  TMCInterval = 300;
  uint32_t  NoAlertInterval = 8;
  uint32_t AlertInterval = 60;
  uint32_t  SimStart = 30;
  uint32_t  SimEnd = 330;
  double offset = 0;
  string QueueLenpath;
  uint32_t  m_InterfaceNum;
  bool HandOver = true;
  uint32_t HandOverTime = 150;//seconds
  bool LoadBalance = false;
  uint32_t ForwardingQueue = 2147483648;

  
  CommandLine cmd;
  cmd.AddValue ("seed", "random seed", seed);
  cmd.AddValue ("Nnodes", "nubmer of staellite", Nnodes);
  cmd.AddValue ("Unidirectional", "chanel type, uni- or bi- directional", UniChannel);
  cmd.AddValue ("NominalMode", "Nominal or failure mode", NominalMode);
  cmd.AddValue ("RestartTimeout", "ms", RestartTimeout);
  cmd.AddValue ("PacketIntervalCh", "ms", PacketIntervalCh);
  cmd.AddValue ("CycelIntervalCh", "ms", CycelIntervalCh);
  cmd.AddValue ("backoffcounter", "backoff after timeout, ms", backoffcounter);
  cmd.AddValue ("NexternalSel", "nubmer of satellite can receive external channel selection command, ms", NexternalSel);
  cmd.AddValue ("Outputpath", "path of channel selextion result", Outputpath);
  cmd.AddValue ("OutputpathSender", "path of channel selextion result", OutputpathSender);
  cmd.AddValue ("ARQTimeout", "path of channel selextion result", ARQTimeout);
  cmd.AddValue ("ARQWindow", "path of channel selextion result", ARQBufferSize);
  cmd.AddValue ("AlertInterval", "path of channel selextion result", AlertInterval);
  cmd.AddValue ("TMCInterval", "path of channel selextion result", TMCInterval);
  cmd.AddValue ("NoAlertInterval", "path of channel selextion result", NoAlertInterval);
  cmd.AddValue ("QueueLenpath", "path of channel selextion result", QueueLenpath);
  cmd.AddValue ("HandOver", "path of channel selextion result", HandOver);
  cmd.AddValue ("HandOverTime", "path of channel selextion result", HandOverTime);
  cmd.AddValue ("LoadBalance", "path of channel selextion result", LoadBalance);
  cmd.AddValue("QueueSize", "the size of the forwarding queues in packets", ForwardingQueue);
    



  cmd.Parse (argc, argv);
  RngSeedManager::SetSeed (seed);
    
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    
  //Outputpath = "./OptimalRawGroup/result.txt";
    
  PACKETREPEATNUMBER = Nchannel * CycelIntervalCh/PacketIntervalCh;
    
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
  m_Outputpath = Outputpath;
  m_QueueLenpath = QueueLenpath;
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
  pointToPoint.SetDeviceAttribute ("Outputpath", StringValue (Outputpath));
  pointToPoint.SetDeviceAttribute ("ARQAckTimeout", TimeValue (MilliSeconds(ARQTimeout))); //in the unit of seconds
  pointToPoint.SetDeviceAttribute ("ARQBufferSize", UintegerValue (ARQBufferSize) ); //in the unit of ms
  pointToPoint.SetDeviceAttribute ("m_InterfaceNum", UintegerValue (m_InterfaceNum) ); //in the unit of ms
  pointToPoint.SetDeviceAttribute ("LoadBalance", BooleanValue (LoadBalance) );


    
    
    
 uint32_t nodei, nodej;
 NetDeviceContainer devicesSet;
 for (uint32_t kk = 0; kk < Nnodes; kk++)
   {
       nodei = kk;
       nodej = (kk + 1) % Nnodes;
       if  (UniChannel)
       {
          devicesSet = pointToPoint.InstallUni (nodes.Get(nodei), nodes.Get(nodej));
       }
       else
       {
         devicesSet = pointToPoint.InstallBi (nodes.Get(nodei), nodes.Get(nodej));
       }
    }
    
  //NS_LOG_DEBUG ("install devicesSet finish " << nodes.Get(Nnodes-1)->GetId () << "\t" << nodes.Get(0)->GetId ()  << ", size " << devicesSet.GetN () );
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
    
    /* Internet stack*/
    /*
    InternetStackHelper stack;
    stack.Install (nodes);
    stack.Install (nodesGES);
    
    Ipv4AddressHelper address;
    
    address.SetBase ("192.168.0.0", "255.255.0.0");
    Ipv4InterfaceContainer staNodeInterface;
    Ipv4InterfaceContainer apNodeInterface;
    
    staNodeInterface = address.Assign (devicesSet);
    apNodeInterface = address.Assign (devicesSetGES);
     PopulateArpCache ();

    */
    
    //NS_LOG_DEBUG ("Start channel selection ----------- Nnodes 2 " );

    uint32_t GESPos[NGESnodes];
    GESPos[0]=0b0000000001;
    GESPos[1]=0b0100000000;
    
    //pointToPoint.InitTopologyGES (Nnodes, NGESnodes, GESPos, Seconds (2000));
    if (HandOver)
      pointToPoint.InitTopologyGES (Nnodes, NGESnodes, GESPos, Seconds (HandOverTime + 30));
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
    
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueueTrace", MakeCallback (ForwadQueue));
    //NS_LOG_DEBUG ("Start channel selection ----------- Nnodes 3" );
    
    
  Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDevice/TxQueue/MaxPackets", UintegerValue (100)); //DropTailQueue
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ARQRecPacketTrace", MakeCallback (&RecPacketARQ));
    
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ARQTxPacketTrace", MakeCallback (&TxPacketARQ));
    
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueueQos3/MaxPackets", UintegerValue (2147483648)); //DropTailQueue
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueueQos2/MaxPackets", UintegerValue (2147483648)); //DropTailQueue
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueueQos1/MaxPackets", UintegerValue (2147483648)); //DropTailQueue
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueueQos0/MaxPackets", UintegerValue (2147483648)); //DropTailQueue
//2147483648
    
    //Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPacketNumber", UintegerValue(10));

    
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue4/MaxPackets", UintegerValue (100)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue3/MaxPackets", UintegerValue (ForwardingQueue)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue2/MaxPackets", UintegerValue (ForwardingQueue)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue1/MaxPackets", UintegerValue (ForwardingQueue)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue0/MaxPackets", UintegerValue (ForwardingQueue)); //DropTailQueue
    
    
    Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/RecPacketTrace", MakeCallback (&RecPacketGES));
    Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ARQRecPacketTrace", MakeCallback (&RecPacketARQ));
    Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ARQTxPacketTrace", MakeCallback (&TxPacketARQ));
    
    Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDeviceGES/TxQueue/MaxPackets", UintegerValue (100)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/TxQueueQos3/MaxPackets", UintegerValue (2147483648)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/TxQueueQos2/MaxPackets", UintegerValue (2147483648)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/TxQueueQos1/MaxPackets", UintegerValue (2147483648)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/TxQueueQos0/MaxPackets", UintegerValue (2147483648)); //DropTailQueue
    
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ForwardQueue4/MaxPackets", UintegerValue (100)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ForwardQueue3/MaxPackets", UintegerValue (ForwardingQueue)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ForwardQueue2/MaxPackets", UintegerValue (ForwardingQueue)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ForwardQueue1/MaxPackets", UintegerValue (ForwardingQueue)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/ForwardQueue0/MaxPackets", UintegerValue (ForwardingQueue)); //DropTailQueue
    
    Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDeviceGES/ARQAckTimeout", TimeValue (MilliSeconds(ARQTimeout))); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDeviceGES/ARQBufferSize", UintegerValue (ARQBufferSize)); //DropTailQueue
    //Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDeviceGES/DataRate", DataRateValue (DataRate ("100000000b/s"))); //DropTailQueue
    //Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDeviceGES/DataRate", DataRateValue (DataRate ("1300000b/s")) ); //DropTailQueue




    
//***********************************************
    //GES is last node
    
     // every LEO to GES
  for (uint16_t kk = 0; kk < Nnodes ; kk++)
    {
     uint32_t offsetMs = rand() % (TMCInterval * 1000);
    //SpiderClient clientAppTMC = crate<clientAppTMC>; Create<clientAppTMC> ()
    SpiderClient * clientAppTMC = new SpiderClient;
    clientAppTMC->SetRemote (Mac48Address::ConvertFrom (nodesGES.Get(0)->GetDevice(0)->GetAddress()) );

    clientAppTMC->SetMaxPackets (2147483648);
    clientAppTMC->SetInterval (Seconds(TMCInterval)); //5s
    clientAppTMC->SetPacketSize (5120); //5k
    clientAppTMC->SetQos (3);
    clientAppTMC->SetConstantRate (true);
    clientAppTMC->SetOutputpath (OutputpathSender);


    clientAppTMC->InstallDevice (nodes.Get(kk)->GetDevice(0));
        
     clientAppTMC->StartApplication (Seconds (SimStart) + MilliSeconds(offsetMs));
     clientAppTMC->StopApplication (Seconds (SimEnd));
        
        //5k/5=1k
    }
	
	// GES to LEO
	if (HandOver) {
    	for (uint16_t kk = 0; kk < Nnodes ; kk++)
      	{
      	  //SpiderClient clientAppTMC = crate<clientAppTMC>; Create<clientAppTMC> ()
      	  SpiderClient * clientAppTMC = new SpiderClient;
      	clientAppTMC->SetRemote (Mac48Address::ConvertFrom (nodes.Get(kk)->GetDevice(0)->GetAddress()) );

      	clientAppTMC->SetMaxPackets (2147483648);
      	clientAppTMC->SetInterval (Seconds(HandOverTime + 30)); //5s
      	clientAppTMC->SetPacketSize (5120); //5k
      	clientAppTMC->SetQos (3);
      	clientAppTMC->SetConstantRate (true);
      	clientAppTMC->SetOutputpath (OutputpathSender);


      	clientAppTMC->InstallDevice (nodesGES.Get(0)->GetDevice(0));
        
       	clientAppTMC->StartApplication (Seconds (HandOverTime + 30) + MilliSeconds(501));
       	clientAppTMC->StopApplication (Seconds (SimEnd));
        
          //5k/5=1k
   		}
	}
    
   
    
    for (uint16_t kk = 0; kk < Nnodes ; kk++)
    {
        uint32_t offsetMs = rand() % (NoAlertInterval * 1000);
        
        SpiderClient * clientAppNoAlter = new SpiderClient;
        clientAppNoAlter->SetRemote (Mac48Address::ConvertFrom (nodesGES.Get(0)->GetDevice(0)->GetAddress()) );
        
        clientAppNoAlter->SetMaxPackets (2147483648);
        clientAppNoAlter->SetInterval (Seconds(NoAlertInterval)); //1
        clientAppNoAlter->SetPacketSize (30720); //30k
        clientAppNoAlter->SetQos (1);
        clientAppNoAlter->SetConstantRate (true);
        clientAppNoAlter->SetOutputpath (OutputpathSender);


        clientAppNoAlter->InstallDevice (nodes.Get(kk)->GetDevice(0));
        
        clientAppNoAlter->StartApplication (Seconds (SimStart) + MilliSeconds(offsetMs));
        clientAppNoAlter->StopApplication (Seconds (SimEnd));
        
        //30k
    }
    
    
    for (uint16_t kk = 0; kk < Nnodes ; kk++)
    {
		uint32_t offsetMs = rand() % (AlertInterval * 1000);
		
        SpiderClient * clientAppAlarm  = new SpiderClient;
        clientAppAlarm->SetRemote (Mac48Address::ConvertFrom (nodesGES.Get(0)->GetDevice(0)->GetAddress()) );
        
        clientAppAlarm->SetMaxPackets (2147483648);
        clientAppAlarm->SetInterval (Seconds(AlertInterval));
        clientAppAlarm->SetPacketSize (102400); //100,000
        clientAppAlarm->SetQos (2);
        clientAppAlarm->SetConstantRate (true);
        //clientAppAlarm->SetPoissonRate (PoissonRate); //(30/1000) packets per seconds, 30 seconds packet
        //lambda 30s (33), 60s (17), 120 (8)
        clientAppAlarm->SetOutputpath (OutputpathSender);


        clientAppAlarm->InstallDevice (nodes.Get(kk)->GetDevice(0));
        
        clientAppAlarm->StartApplication (Seconds (SimStart) + MilliSeconds(offsetMs));
        clientAppAlarm->StopApplication (Seconds (SimEnd));
        
        //3.3k
    }
    
    //total 1+30+3.3=34.3k, with 10 satellites, 343 kbps
    
    
    
    
 //***********************************************
    //trafficGeneration (nodes, Nnodes);


    //NS_LOG_DEBUG ("Start channel selection ----------- Nnodes " );


  
  std::ofstream myfile;
    
  std::string dropfile=m_Outputpath;
  myfile.open (dropfile, ios::out | ios::trunc);
  myfile << "Start channel selection ----------- Nnodes " << Nnodes << ", Ndevice " << devicesSet.GetN () << ", UniChannel " << UniChannel << "\n";
  //myfile << "nodes ----------- Nnodes " << nodes.Get(0)->GetId () << ", device " << nodes.Get(0)->GetDevice(0)->GetAddress() << ",  " << nodes.Get(0)->GetDevice(1)->GetAddress() << "\n";
  myfile.close();
    
    std::string senderfile=OutputpathSender;
    myfile.open (senderfile, ios::out | ios::trunc);
    myfile.close();
    /*
    for (uint32_t kk = 0; kk < Nnodes; kk++)
    {
        if (!UniChannel)
        NS_LOG_DEBUG( "LEO ----------- Nnodes " << nodes.Get(kk)->GetId () << ", device " << nodes.Get(kk)->GetDevice(0)->GetAddress() << ",  " << nodes.Get(kk)->GetDevice(1)->GetAddress()  << ",  " << nodes.Get(kk)->GetDevice(2)->GetAddress() );
        else
        NS_LOG_DEBUG( "LEO ----------- Nnodes " << nodes.Get(kk)->GetId () << ", device " << nodes.Get(kk)->GetDevice(0)->GetAddress() << ",  " << nodes.Get(kk)->GetDevice(1)->GetAddress()  );
    }
    
  for (uint32_t ii = 0; ii < NGESnodes; ii++)
    {
        NS_LOG_DEBUG( "GES ----------- Nnodes " << nodesGES.Get(ii)->GetId () << ", device " << nodesGES.Get(ii)->GetDevice(0)->GetAddress()  );
    }
*/


    
  //NS_LOG_DEBUG ("install devicesSet finish ....."  );
    
    
    Simulator::Stop (Seconds (SimEnd));



  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}


