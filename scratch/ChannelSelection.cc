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

void
ChannelSelection  (std::string contex, Mac48Address addr, Time ts, uint32_t rx, uint32_t tx)
{
    ChannelNum++;
    std::ofstream myfile;
    std::string dropfile="./OptimalRawGroup/channelSelected.txt";
    myfile.open (dropfile, ios::out | ios::app);
    myfile << ChannelNum << ", device " << addr << " at time " << ts  << " select chanel rx " <<  rx << ", tx " << tx << "\n";
    myfile.close();
}



int
main (int argc, char *argv[])
{
  uint32_t seed = 1;
  uint32_t Nnodes = 11;
  bool UniChannel = false;
  bool NominalMode = true;
    
  string DataRate; // nominal mode, 1.1 Mbps (failure mode)
  string Delay;

  
  CommandLine cmd;
  cmd.AddValue ("seed", "random seed", seed);
  cmd.AddValue ("Nnodes", "nubmer of staellite", Nnodes);
  cmd.AddValue ("UniChannel", "chanel type, uni- or bi- directional", UniChannel);
  cmd.AddValue ("NominalMode", "Nominal or failure mode", NominalMode);
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


  cmd.Parse (argc, argv);
  RngSeedManager::SetSeed (seed);
    
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (Nnodes);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (DataRate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (Delay));
  pointToPoint.SetChannelAttribute ("UniChannel", BooleanValue (UniChannel));
  
  pointToPoint.SetDeviceAttribute ("ChannelResp_delay", TimeValue (MilliSeconds (2000.0)));
  pointToPoint.SetDeviceAttribute ("ChannelConf_delay", TimeValue (MilliSeconds (2000.0)));
  pointToPoint.SetDeviceAttribute ("Channel_delay_packet", TimeValue (MilliSeconds (10.0)));
  pointToPoint.SetDeviceAttribute ("ChannelWaiting_Interval", TimeValue (MilliSeconds (10.0)));
  // channel request packet transmission time, 21 bytes/DataRate ~= 0.13 ms
  pointToPoint.SetDeviceAttribute ("backoffcounter", UintegerValue (1000)); //in the unit of ms
    
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

  Config::Set ("/NodeList/0/DeviceList/0/$ns3::PointToPointNetDevice/StartChannelSelection", BooleanValue(true)); //node  receive external command for channel selection
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ChannelSelected", MakeCallback (&ChannelSelection));


  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devicesSet);
  
  std::ofstream myfile;
  std::string dropfile="./OptimalRawGroup/channelSelected.txt";
  myfile.open (dropfile, ios::out | ios::app);
  myfile << "Start channel selection ----------- Nnodes " << Nnodes << ", Ndevice " << devicesSet.GetN () << ", UniChannel " << UniChannel << "\n";
  //myfile << "nodes ----------- Nnodes " << nodes.Get(0)->GetId () << ", device " << nodes.Get(0)->GetDevice(0)->GetAddress() << ",  " << nodes.Get(0)->GetDevice(1)->GetAddress() << "\n";
  myfile.close();

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (nodes.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (1000.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (0), 9);

  echoClient.SetAttribute ("MaxPackets", UintegerValue (1000));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (100.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
    NS_LOG_UNCOND ("install devicesSet finish ....."  );


  ApplicationContainer clientApps = echoClient.Install (nodes.Get (Nnodes-1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (1000.0));
    
  pointToPoint.EnablePcapAll ("secondLe", true);
  Simulator::Stop (Seconds (1000.0));
    
    NS_LOG_UNCOND ("install devicesSet finish ....."  );



  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
