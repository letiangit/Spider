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
    

    

  uint32_t seed = 1;
  uint32_t Nnodes = 11;
  uint32_t RestartTimeout = 2000;
  uint32_t PacketIntervalCh = 10;
  uint32_t CycelIntervalCh = 20;
  uint32_t backoffcounter = 1000;
  uint32_t NexternalSel = 1;
  bool UniChannel = false;
  bool NominalMode = true;
  string Outputpath;
    
  string DataRate; // nominal mode, 1.1 Mbps (failure mode)
  string Delay;
  uint32_t PACKETREPEATNUMBER;
  uint32_t Nchannel = 4;

  
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


  cmd.Parse (argc, argv);
  RngSeedManager::SetSeed (seed);
    
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    
  //Outputpath = "./OptimalRawGroup/result.txt";
    
  PACKETREPEATNUMBER = Nchannel * CycelIntervalCh/PacketIntervalCh;
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
    
 uint32_t nodei, nodej;
 NetDeviceContainer devicesSet;
 for (uint32_t kk = 0; kk < Nnodes; kk++)
   {
       nodei = kk;
       nodej = (kk + 1) % Nnodes;
       NS_LOG_UNCOND (">>>>>nodei " << nodei << ", nodej " << nodej << ", kk " << kk);
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
  nodesGES.Create (1);
  NetDeviceContainer devicesSetGES;
  for (uint32_t kk = 0; kk < Nnodes; kk++)
    {
        NS_LOG_UNCOND ("install Ges, node  " << kk);
        devicesSetGES = pointToPoint.InstallGES (nodesGES.Get(0), nodes.Get(kk));
    }

  for (uint32_t kk = 0; kk < NexternalSel; kk++)
    {
      std::ostringstream STA;
      STA << kk;
      std::string strSTA = STA.str();
        
      Config::Set ("/NodeList/"+strSTA+"/DeviceList/0/$ns3::PointToPointNetDevice/StartChannelSelection", BooleanValue(true)); //node  receive external command for channel selection
    }
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ChannelSelected", MakeCallback (&ChannelSelection));


  InternetStackHelper stack;
  stack.Install (nodes);
  stack.Install (nodesGES);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devicesSet);
    
  //for GES
  address.SetBase ("10.2.0.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesGES = address.Assign (devicesSetGES);
  
  std::ofstream myfile;
  std::string dropfile=m_Outputpath;
  myfile.open (dropfile, ios::out | ios::trunc);
  myfile << "Start channel selection ----------- Nnodes " << Nnodes << ", Ndevice " << devicesSet.GetN () << ", UniChannel " << UniChannel << "\n";
  //myfile << "nodes ----------- Nnodes " << nodes.Get(0)->GetId () << ", device " << nodes.Get(0)->GetDevice(0)->GetAddress() << ",  " << nodes.Get(0)->GetDevice(1)->GetAddress() << "\n";
  myfile.close();
    
  for (uint32_t kk = 0; kk < Nnodes; kk++)
    {
        NS_LOG_UNCOND( "nodes ----------- Nnodes " << nodes.Get(kk)->GetId () << ", device " << nodes.Get(kk)->GetDevice(0)->GetAddress() << ",  " << nodes.Get(kk)->GetDevice(1)->GetAddress() << ",  " << nodes.Get(kk)->GetDevice(2)->GetAddress() );
    }
    
  for (uint32_t kk = 0; kk < Nnodes; kk++)
    {
        NS_LOG_UNCOND( "nodes ----------- Nnodes " << nodesGES.Get(0)->GetId () << ", device " << nodesGES.Get(0)->GetDevice(kk)->GetAddress()  );
    }

  
  UdpEchoServerHelper echoServer (9);
 
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (1000.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (0), 9);

  echoClient.SetAttribute ("MaxPackets", UintegerValue (1000));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (100.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (500));
  NS_LOG_UNCOND ("install devicesSet finish ....."  );


  ApplicationContainer clientApps = echoClient.Install (nodes.Get (Nnodes-4));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (1000.0));
    
  //GES
    
  UdpEchoServerHelper echoServerGES (8);
    
  ApplicationContainer serverAppsGES = echoServerGES.Install (nodes.Get (10)); //server
  serverAppsGES.Start (Seconds (1.0));
  serverAppsGES.Stop (Seconds (1000.0));
    
  UdpEchoClientHelper echoClientGES (interfacesGES.GetAddress (21), 8); //device of server
    
  echoClientGES.SetAttribute ("MaxPackets", UintegerValue (1000));
  echoClientGES.SetAttribute ("Interval", TimeValue (Seconds (100.0)));
  echoClientGES.SetAttribute ("PacketSize", UintegerValue (1000));
    
    
  ApplicationContainer clientAppsGES = echoClientGES.Install (nodesGES.Get (0)); //client
  clientAppsGES.Start (Seconds (2.0));
  clientAppsGES.Stop (Seconds (1000.0));
    
  Simulator::Stop (Seconds (1000.0));
    //
    
    for (uint32_t kk = 0; kk < 2*Nnodes; kk++)
    {
        
        NS_LOG_UNCOND( "devicesSetGES " << devicesSetGES.GetN () );

        //NS_LOG_UNCOND( "interfacesGES.GetAddress " << interfaces.GetAddress (kk)  );

        NS_LOG_UNCOND( "interfacesGES.GetAddress " << interfacesGES.GetAddress (kk)  );
    }
    
  NS_LOG_UNCOND ("install devicesSet finish ....."  );

  //PopulateArpCache ();

  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    


  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
