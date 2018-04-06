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
  uint32_t  ARQTimeout = 500;
  uint32_t  ARQBufferSize = 7;
  uint32_t  PoissonRate = 33;

  
  CommandLine cmd;
  cmd.AddValue ("seed", "random seed", seed);
  cmd.AddValue ("Nnodes", "nubmer of staellite", Nnodes);
  cmd.AddValue ("UniChannel", "chanel type, uni- or bi- directional", UniChannel);
  cmd.AddValue ("NominalMode", "Nominal or failure mode", NominalMode);
  cmd.AddValue ("RestartTimeout", "ms", RestartTimeout);
  cmd.AddValue ("PacketIntervalCh", "ms", PacketIntervalCh);
  cmd.AddValue ("CycelIntervalCh", "ms", CycelIntervalCh);
  cmd.AddValue ("backoffcounter", "backoff after timeout, ms", backoffcounter);
  cmd.AddValue ("NexternalSel", "nubmer of satellite can receive external channel selection command, ms", NexternalSel);
  cmd.AddValue ("Outputpath", "path of channel selextion result", Outputpath);
  cmd.AddValue ("OutputpathSender", "path of channel selextion result", OutputpathSender);
  cmd.AddValue ("ARQTimeout", "path of channel selextion result", ARQTimeout);
  cmd.AddValue ("ARQBufferSize", "path of channel selextion result", ARQBufferSize);
  cmd.AddValue ("PoissonRate", "path of channel selextion result", PoissonRate);


  cmd.Parse (argc, argv);
  RngSeedManager::SetSeed (seed);
    
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    
  //Outputpath = "./OptimalRawGroup/result.txt";
    
  PACKETREPEATNUMBER = Nchannel * CycelIntervalCh/PacketIntervalCh;
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
    
  if (NominalMode)
    {
        DataRate = "1.3Mbps"; // nominal mode, 1.1 Mbps (failure mode)
        Delay = "14.4ms";
    }
    else
    {
        DataRate = "1.1Mbps"; // nominal mode, 1.1 Mbps (failure mode)
        Delay = "17.2ms";
    }
  m_Outputpath = Outputpath;

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
    
  NS_LOG_UNCOND ("install devicesSet finish " << nodes.Get(Nnodes-1)->GetId () << "\t" << nodes.Get(0)->GetId ()  << ", size " << devicesSet.GetN () );
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
    
    NS_LOG_UNCOND ("Start channel selection ----------- Nnodes 2 " );

    uint32_t GESPos[NGESnodes];
    GESPos[0]=0b000000001;
    GESPos[1]=0b100000000;
    
    pointToPoint.InitTopologyGES (Nnodes, NGESnodes, GESPos, Seconds (2000));


  for (uint32_t kk = 0; kk < NexternalSel; kk++)
    {
      std::ostringstream STA;
      STA << kk;
      std::string strSTA = STA.str();
        
      Config::Set ("/NodeList/"+strSTA+"/DeviceList/0/$ns3::PointToPointNetDevice/StartChannelSelection", BooleanValue(true)); //node  receive external command for channel selection
    }
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ChannelSelected", MakeCallback (&ChannelSelection));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/RecPacketTrace", MakeCallback (&RecPacket));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDeviceGES/RecPacketTrace", MakeCallback (&RecPacketGES));
    NS_LOG_UNCOND ("Start channel selection ----------- Nnodes 3" );
    
    
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
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue3/MaxPackets", UintegerValue (2147483648)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue2/MaxPackets", UintegerValue (2147483648)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue1/MaxPackets", UintegerValue (2147483648)); //DropTailQueue
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ForwardQueue0/MaxPackets", UintegerValue (2147483648)); //DropTailQueue
    
    


    
//***********************************************
    //GES is last node
    
     // every LEO to GES
  for (uint16_t kk = 0; kk < Nnodes - 1; kk++)
    {
    //SpiderClient clientAppTMC = crate<clientAppTMC>; Create<clientAppTMC> ()
    SpiderClient * clientAppTMC = new SpiderClient;
    clientAppTMC->SetRemote (Mac48Address::ConvertFrom (nodes.Get(Nnodes - 1)->GetDevice(0)->GetAddress()) );

    clientAppTMC->SetMaxPackets (2147483648);
    clientAppTMC->SetInterval (Seconds(300)); //5s
    clientAppTMC->SetPacketSize (5000); //5k
    clientAppTMC->SetQos (3);
    clientAppTMC->SetConstantRate (true);
    clientAppTMC->SetOutputpath (OutputpathSender);


    clientAppTMC->InstallDevice (nodes.Get(kk)->GetDevice(0));
        
     clientAppTMC->StartApplication (Seconds (302.0));
     clientAppTMC->StopApplication (Seconds (1500.0));
        
        //5k/5=1k
    }
    
    
    for (uint16_t kk = 0; kk < Nnodes - 1; kk++)
    {
        SpiderClient * clientAppNoAlter = new SpiderClient;
        clientAppNoAlter->SetRemote (Mac48Address::ConvertFrom (nodes.Get(Nnodes - 1)->GetDevice(0)->GetAddress()) );
        
        clientAppNoAlter->SetMaxPackets (2147483648);
        clientAppNoAlter->SetInterval (Seconds(3)); //1
        clientAppNoAlter->SetPacketSize (30000); //30k
        clientAppNoAlter->SetQos (1);
        clientAppNoAlter->SetConstantRate (true);
        clientAppNoAlter->SetOutputpath (OutputpathSender);


        clientAppNoAlter->InstallDevice (nodes.Get(kk)->GetDevice(0));
        
        clientAppNoAlter->StartApplication (Seconds (300.0));
        clientAppNoAlter->StopApplication (Seconds (1500.0));
        
        //30k
    }
    
    
    for (uint16_t kk = 0; kk < Nnodes - 1; kk++)
    {
        SpiderClient * clientAppAlarm  = new SpiderClient;
        clientAppAlarm->SetRemote (Mac48Address::ConvertFrom (nodes.Get(Nnodes - 1)->GetDevice(0)->GetAddress()) );
        
        clientAppAlarm->SetMaxPackets (2147483648);
        //clientAppAlarm->SetInterval (Seconds(30));
        clientAppAlarm->SetPacketSize (100000); //100,000
        clientAppAlarm->SetQos (2);
        clientAppAlarm->SetConstantRate (false);
        clientAppAlarm->SetPoissonRate (PoissonRate); //(30/1000) packets per seconds, 30 seconds packet
        //lambda 30s (33), 60s (17), 120 (8)
        clientAppAlarm->SetOutputpath (OutputpathSender);


        clientAppAlarm->InstallDevice (nodes.Get(kk)->GetDevice(0));
        
        clientAppAlarm->StartApplication (Seconds (313.0));
        clientAppAlarm->StopApplication (Seconds (1500.0));
        
        //3.3k
    }
    
    //total 1+30+3.3=34.3k, with 10 satellites, 343 kbps
    
    
    
    
 //***********************************************
    //trafficGeneration (nodes, Nnodes);


    NS_LOG_UNCOND ("Start channel selection ----------- Nnodes " );


  
  std::ofstream myfile;
    
  std::string dropfile=m_Outputpath;
  myfile.open (dropfile, ios::out | ios::trunc);
  myfile << "Start channel selection ----------- Nnodes " << Nnodes << ", Ndevice " << devicesSet.GetN () << ", UniChannel " << UniChannel << "\n";
  //myfile << "nodes ----------- Nnodes " << nodes.Get(0)->GetId () << ", device " << nodes.Get(0)->GetDevice(0)->GetAddress() << ",  " << nodes.Get(0)->GetDevice(1)->GetAddress() << "\n";
  myfile.close();
    
    std::string senderfile=OutputpathSender;
    myfile.open (senderfile, ios::out | ios::trunc);
    myfile.close();
    
  for (uint32_t kk = 0; kk < Nnodes; kk++)
    {
        NS_LOG_UNCOND( "LEO ----------- Nnodes " << nodes.Get(kk)->GetId () << ", device " << nodes.Get(kk)->GetDevice(0)->GetAddress() << ",  " << nodes.Get(kk)->GetDevice(1)->GetAddress()  );
    }
    
  for (uint32_t ii = 0; ii < NGESnodes; ii++)
    {
        NS_LOG_UNCOND( "GES ----------- Nnodes " << nodesGES.Get(ii)->GetId () << ", device " << nodesGES.Get(ii)->GetDevice(0)->GetAddress()  );
    }



    
  NS_LOG_UNCOND ("install devicesSet finish ....."  );
    
    
    Simulator::Stop (Seconds (1500.0));



  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}


