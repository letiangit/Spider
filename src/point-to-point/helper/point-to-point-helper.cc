/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-remote-channel.h"
#include "ns3/queue.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"
#include "ns3/mpi-interface.h"
#include "ns3/mpi-receiver.h"

#include "ns3/trace-helper.h"
#include "point-to-point-helper.h"

#include <algorithm>    // std::find
#include "ns3/point-to-point-net-device-ges.h"
#include "ns3/point-to-point-channel-ges.h"
#include "ns3/ppp-header.h"
#include "ns3/spf.h"






namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointHelper");

PointToPointHelper::PointToPointHelper ()
{
  m_queueFactory.SetTypeId ("ns3::DropTailQueue");
  m_deviceFactory.SetTypeId ("ns3::PointToPointNetDevice");
  m_deviceGESFactory.SetTypeId ("ns3::PointToPointNetDeviceGES");
  m_channelFactory.SetTypeId ("ns3::PointToPointChannel");
  m_channelGESFactory.SetTypeId ("ns3::PointToPointChannelGES");
  m_remoteChannelFactory.SetTypeId ("ns3::PointToPointRemoteChannel");
}

void 
PointToPointHelper::SetQueue (std::string type,
                              std::string n1, const AttributeValue &v1,
                              std::string n2, const AttributeValue &v2,
                              std::string n3, const AttributeValue &v3,
                              std::string n4, const AttributeValue &v4)
{
  m_queueFactory.SetTypeId (type);
  m_queueFactory.Set (n1, v1);
  m_queueFactory.Set (n2, v2);
  m_queueFactory.Set (n3, v3);
  m_queueFactory.Set (n4, v4);
}

void 
PointToPointHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

void 
PointToPointHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  m_channelFactory.Set (n1, v1);
  m_remoteChannelFactory.Set (n1, v1);
}

void 
PointToPointHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename)
{
  //
  // All of the Pcap enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type PointToPointNetDevice.
  //
  Ptr<PointToPointNetDevice> device = nd->GetObject<PointToPointNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("PointToPointHelper::EnablePcapInternal(): Device " << device << " not of type ns3::PointToPointNetDevice");
      return;
    }

  PcapHelper pcapHelper;

  std::string filename;
  if (explicitFilename)
    {
      filename = prefix;
    }
  else
    {
      filename = pcapHelper.GetFilenameFromDevice (prefix, device);
    }

  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out, 
                                                     PcapHelper::DLT_PPP);
  pcapHelper.HookDefaultSink<PointToPointNetDevice> (device, "PromiscSniffer", file);
}

void 
PointToPointHelper::EnableAsciiInternal (
  Ptr<OutputStreamWrapper> stream, 
  std::string prefix, 
  Ptr<NetDevice> nd,
  bool explicitFilename)
{
  //
  // All of the ascii enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type PointToPointNetDevice.
  //
  Ptr<PointToPointNetDevice> device = nd->GetObject<PointToPointNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("PointToPointHelper::EnableAsciiInternal(): Device " << device << 
                   " not of type ns3::PointToPointNetDevice");
      return;
    }

  //
  // Our default trace sinks are going to use packet printing, so we have to 
  // make sure that is turned on.
  //
  Packet::EnablePrinting ();

  //
  // If we are not provided an OutputStreamWrapper, we are expected to create 
  // one using the usual trace filename conventions and do a Hook*WithoutContext
  // since there will be one file per context and therefore the context would
  // be redundant.
  //
  if (stream == 0)
    {
      //
      // Set up an output stream object to deal with private ofstream copy 
      // constructor and lifetime issues.  Let the helper decide the actual
      // name of the file given the prefix.
      //
      AsciiTraceHelper asciiTraceHelper;

      std::string filename;
      if (explicitFilename)
        {
          filename = prefix;
        }
      else
        {
          filename = asciiTraceHelper.GetFilenameFromDevice (prefix, device);
        }

      Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream (filename);

      //
      // The MacRx trace source provides our "r" event.
      //
      asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<PointToPointNetDevice> (device, "MacRx", theStream);

      //
      // The "+", '-', and 'd' events are driven by trace sources actually in the
      // transmit queue.
      //
      Ptr<Queue> queue = device->GetQueue ();
      asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue> (queue, "Enqueue", theStream);
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue> (queue, "Drop", theStream);
      asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue> (queue, "Dequeue", theStream);

      // PhyRxDrop trace source for "d" event
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<PointToPointNetDevice> (device, "PhyRxDrop", theStream);

      return;
    }

  //
  // If we are provided an OutputStreamWrapper, we are expected to use it, and
  // to providd a context.  We are free to come up with our own context if we
  // want, and use the AsciiTraceHelper Hook*WithContext functions, but for 
  // compatibility and simplicity, we just use Config::Connect and let it deal
  // with the context.
  //
  // Note that we are going to use the default trace sinks provided by the 
  // ascii trace helper.  There is actually no AsciiTraceHelper in sight here,
  // but the default trace sinks are actually publicly available static 
  // functions that are always there waiting for just such a case.
  //
  uint32_t nodeid = nd->GetNode ()->GetId ();
  uint32_t deviceid = nd->GetIfIndex ();
  std::ostringstream oss;

  oss << "/NodeList/" << nd->GetNode ()->GetId () << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/MacRx";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/TxQueue/Enqueue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/TxQueue/Dequeue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/TxQueue/Drop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/PhyRxDrop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}

NetDeviceContainer 
PointToPointHelper::Install (NodeContainer c)
{
  NS_ASSERT (c.GetN () == 2);
  return Install (c.Get (0), c.Get (1));
}

NetDeviceContainer 
PointToPointHelper::Install (Ptr<Node> a, Ptr<Node> b)
{
  NetDeviceContainer container;

  Ptr<PointToPointNetDevice> devA = m_deviceFactory.Create<PointToPointNetDevice> ();
  devA->SetAddress (Mac48Address::Allocate ());
  a->AddDevice (devA);
  Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
  devA->SetQueue (queueA);
  Ptr<PointToPointNetDevice> devB = m_deviceFactory.Create<PointToPointNetDevice> ();
  devB->SetAddress (Mac48Address::Allocate ());
  b->AddDevice (devB);
  Ptr<Queue> queueB = m_queueFactory.Create<Queue> ();
  devB->SetQueue (queueB);
  // If MPI is enabled, we need to see if both nodes have the same system id 
  // (rank), and the rank is the same as this instance.  If both are true, 
  //use a normal p2p channel, otherwise use a remote channel
  bool useNormalChannel = true;
  //Ptr<PointToPointChannel> channel = 0;
  Ptr<PointToPointChannel> channel = 0;

  if (MpiInterface::IsEnabled ())
    {
      uint32_t n1SystemId = a->GetSystemId ();
      uint32_t n2SystemId = b->GetSystemId ();
      uint32_t currSystemId = MpiInterface::GetSystemId ();
      if (n1SystemId != currSystemId || n2SystemId != currSystemId) 
        {
          useNormalChannel = false;
        }
      NS_LOG_UNCOND ("MpiInterface::IsEnabled");
    }
  if (useNormalChannel)
    {
      //channel = m_channelFactory.Create<PointToPointChannel> ();
      channel = m_channelFactory.Create<PointToPointChannel> ();
    }
  /*else
    {
      channel = m_remoteChannelFactory.Create<PointToPointRemoteChannel> ();
      Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver> ();
      Ptr<MpiReceiver> mpiRecB = CreateObject<MpiReceiver> ();
      mpiRecA->SetReceiveCallback (MakeCallback (&PointToPointNetDevice::Receive, devA));
      mpiRecB->SetReceiveCallback (MakeCallback (&PointToPointNetDevice::Receive, devB));
      devA->AggregateObject (mpiRecA);
      devB->AggregateObject (mpiRecB);
    } */

  devA->Attach (channel);
  devB->Attach (channel);
  container.Add (devA);
  container.Add (devB);

  return container;
}

NetDeviceContainer 
PointToPointHelper::InstallBi (Ptr<Node> a, Ptr<Node> b)
{
  //NetDeviceContainer container;
  std::cout << ".................InstallBi : a  " << a->GetId () << ", b " <<  b->GetId () << '\n';

    
  m_nodeListIterator = find (m_nodeList.begin(), m_nodeList.end(), a);
  if (m_nodeListIterator != m_nodeList.end())
    {
     devAFirst  = m_nodeDeviceMap.find (a->GetId ())->second;
     std::cout << "...node a already in the list : " << devAFirst->GetAddress ()<< '\n';
     devASecond = m_deviceFactory.Create<PointToPointNetDevice> ();
     devASecond->SetAddress (Mac48Address::Allocate ());
     a->AddDevice (devASecond);
     m_nodeDeviceMap[a->GetId ()] = devASecond;
     std::cout << "node " << a->GetId () << " install device, number " << a->GetNDevices () << '\n';
     Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueCritical = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queuehighPri = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueBestEff = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueBackGround = m_queueFactory.Create<Queue> (); 
     devASecond->SetQueue (queueA, queueCritical, queuehighPri, queueBestEff, queueBackGround);

     //build connection between two device
     devAFirst->AddDevice (devASecond);
     devASecond->AddDevice (devAFirst);
    }
  else
    {
     std::cout << "...node a not in the list : \n";
     
     devAFirst = m_deviceFactory.Create<PointToPointNetDevice> ();
     devAFirst->SetAddress (Mac48Address::Allocate ());
     a->AddDevice (devAFirst);
     m_nodeDeviceMap[a->GetId ()] = devAFirst;
     std::cout << "node " << a->GetId () << " install device, number " << a->GetNDevices () << '\n';
     Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueCritical = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queuehighPri = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueBestEff = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueBackGround = m_queueFactory.Create<Queue> ();
     devAFirst->SetQueue (queueA, queueCritical, queuehighPri, queueBestEff, queueBackGround);
    }
  
  m_nodeListIterator = find (m_nodeList.begin(), m_nodeList.end(), b);
  if (m_nodeListIterator != m_nodeList.end())
    {
     devBFirst  = m_nodeDeviceMap.find (b->GetId ())->second;
     std::cout << "...node a already in the list : " << devBFirst->GetAddress ()<< '\n';
     devBSecond = m_deviceFactory.Create<PointToPointNetDevice> ();
     devBSecond->SetAddress (Mac48Address::Allocate ());
     b->AddDevice (devBSecond);
     m_nodeDeviceMap[a->GetId ()] = devBSecond;
     std::cout << "node " << b->GetId () << " install device, number " << b->GetNDevices () << '\n';
     Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueCritical = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queuehighPri = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueBestEff = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueBackGround = m_queueFactory.Create<Queue> ();
     devBSecond->SetQueue (queueA, queueCritical, queuehighPri, queueBestEff, queueBackGround);
     //build connection between two device
     devBFirst->AddDevice (devBSecond);
     devBSecond->AddDevice (devBFirst);
    }
  else
    {
     std::cout << "...node b not in the list : \n";
     
     devBFirst = m_deviceFactory.Create<PointToPointNetDevice> ();
     devBFirst->SetAddress (Mac48Address::Allocate ());
     b->AddDevice (devBFirst);
     m_nodeDeviceMap[b->GetId ()] = devBFirst;
     std::cout << "node " << b->GetId () << " install device, number " << b->GetNDevices () << '\n';
     Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueCritical = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queuehighPri = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueBestEff = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueBackGround = m_queueFactory.Create<Queue> ();
     devBFirst->SetQueue (queueA, queueCritical, queuehighPri, queueBestEff, queueBackGround);
    }

  bool useNormalChannel = true;
  Ptr<PointToPointChannel> channel = 0;

  if (MpiInterface::IsEnabled ())
    {
      uint32_t n1SystemId = a->GetSystemId ();
      uint32_t n2SystemId = b->GetSystemId ();
      uint32_t currSystemId = MpiInterface::GetSystemId ();
      if (n1SystemId != currSystemId || n2SystemId != currSystemId) 
        {
          useNormalChannel = false;
        }
      NS_LOG_UNCOND ("MpiInterface::IsEnabled");
    }
  if (useNormalChannel)
    {
      //channel = m_channelFactory.Create<PointToPointChannel> ();
      channel = m_channelFactory.Create<PointToPointChannel> ();
    }
  /*else
    {
      channel = m_remoteChannelFactory.Create<PointToPointRemoteChannel> ();
      Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver> ();
      Ptr<MpiReceiver> mpiRecB = CreateObject<MpiReceiver> ();
      mpiRecA->SetReceiveCallback (MakeCallback (&PointToPointNetDevice::Receive, devA));
      mpiRecB->SetReceiveCallback (MakeCallback (&PointToPointNetDevice::Receive, devB));
      devA->AggregateObject (mpiRecA);
      devB->AggregateObject (mpiRecB);
    } */

  
  m_nodeListIterator = find (m_nodeList.begin(), m_nodeList.end(), a);
  if (m_nodeListIterator != m_nodeList.end())
    {
     std::cout << "===node a already in the list : " << *m_nodeListIterator << '\n';
     devASecond->Attach (channel);
     container.Add (devASecond);
    }
  else
    {
     std::cout << "===node a not in the list : \n";
     m_nodeList.push_back(a);
     devAFirst->Attach (channel);
     container.Add (devAFirst);
    }
  
  m_nodeListIterator = find (m_nodeList.begin(), m_nodeList.end(), b);
  if (m_nodeListIterator != m_nodeList.end())
    {
     std::cout << "===node b already in the list : " << *m_nodeListIterator << '\n';
     devBSecond->Attach (channel);
     container.Add (devBSecond);
    }
  else
    {
     std::cout << "===node b not in the list : \n";
     container.Add (devBFirst);
     m_nodeList.push_back(b);
     devBFirst->Attach (channel);
    }
  return container;
}



//installation happens after the LEO installation is done.



NetDeviceContainer 
PointToPointHelper::InstallUni (Ptr<Node> a, Ptr<Node> b)
{
  //NetDeviceContainer container;
  std::cout << ".................InstallUni : a  " << a->GetId () << ", b " <<  b->GetId () << '\n';

    
  m_nodeListIterator = find (m_nodeList.begin(), m_nodeList.end(), a);
  if (m_nodeListIterator != m_nodeList.end())
    {
     //Ptr<PointToPointNetDevice> devATemp;
     //devATemp = m_nodeDeviceMap.find (a->GetId ())->second;
     devA  = m_nodeDeviceMap.find (a->GetId ())->second;
     //std::cout << "...node a already in the list : " << devATemp->GetAddress ()<< '\n';
     std::cout << "...node a already in the list : " << devA->GetAddress ()<< '\n';
    }
  else
    {
     std::cout << "...node a not in the list : \n";
     
     devA = m_deviceFactory.Create<PointToPointNetDevice> ();
     devA->SetAddress (Mac48Address::Allocate ());
     a->AddDevice (devA);
     m_nodeDeviceMap[a->GetId ()] = devA;
     std::cout << "node " << a->GetId () << " install device, number " << a->GetNDevices () << '\n';
     Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueCritical = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queuehighPri = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueBestEff = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueBackGround = m_queueFactory.Create<Queue> ();
     devA->SetQueue (queueA, queueCritical, queuehighPri, queueBestEff, queueBackGround);
    }
  
  m_nodeListIterator = find (m_nodeList.begin(), m_nodeList.end(), b);
  if (m_nodeListIterator != m_nodeList.end())
    {
     //devB = b->GetDevice(0);
     devB  = m_nodeDeviceMap.find (b->GetId ())->second;
     std::cout << "...node b already in the list : " << devB->GetAddress () << '\n';
    }
  else
    {
     std::cout << "....node b not in the list : \n";
     
     devB = m_deviceFactory.Create<PointToPointNetDevice> ();
     devB->SetAddress (Mac48Address::Allocate ());
     b->AddDevice (devB);
     m_nodeDeviceMap[b->GetId ()] = devB;
     std::cout << "node " << b->GetId () << " install device, number " << b->GetNDevices () << '\n';
     Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueCritical = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queuehighPri = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueBestEff = m_queueFactory.Create<Queue> ();
     Ptr<Queue> queueBackGround = m_queueFactory.Create<Queue> ();
     devB->SetQueue (queueA, queueCritical, queuehighPri, queueBestEff, queueBackGround);
    }

  //Ptr<PointToPointNetDevice> devA = m_deviceFactory.Create<PointToPointNetDevice> ();
  /*devA =  m_deviceFactory.Create<PointToPointNetDevice> ();
  devA->SetAddress (Mac48Address::Allocate ());
  a->AddDevice (devA);
  Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
  devA->SetQueue (queueA); */
  //Ptr<PointToPointNetDevice> devB = m_deviceFactory.Create<PointToPointNetDevice> ();
  /*devB = m_deviceFactory.Create<PointToPointNetDevice> ();
  devB->SetAddress (Mac48Address::Allocate ());
  b->AddDevice (devB);
  Ptr<Queue> queueB = m_queueFactory.Create<Queue> ();
  devB->SetQueue (queueB); */
  // If MPI is enabled, we need to see if both nodes have the same system id 
  // (rank), and the rank is the same as this instance.  If both are true, 
  //use a normal p2p channel, otherwise use a remote channel
  bool useNormalChannel = true;
  //Ptr<PointToPointChannel> channel = 0;
  Ptr<PointToPointChannel> channel = 0;

  if (MpiInterface::IsEnabled ())
    {
      uint32_t n1SystemId = a->GetSystemId ();
      uint32_t n2SystemId = b->GetSystemId ();
      uint32_t currSystemId = MpiInterface::GetSystemId ();
      if (n1SystemId != currSystemId || n2SystemId != currSystemId) 
        {
          useNormalChannel = false;
        }
      NS_LOG_UNCOND ("MpiInterface::IsEnabled");
    }
  if (useNormalChannel)
    {
      //channel = m_channelFactory.Create<PointToPointChannel> ();
      channel = m_channelFactory.Create<PointToPointChannel> ();
    }
  /*else
    {
      channel = m_remoteChannelFactory.Create<PointToPointRemoteChannel> ();
      Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver> ();
      Ptr<MpiReceiver> mpiRecB = CreateObject<MpiReceiver> ();
      mpiRecA->SetReceiveCallback (MakeCallback (&PointToPointNetDevice::Receive, devA));
      mpiRecB->SetReceiveCallback (MakeCallback (&PointToPointNetDevice::Receive, devB));
      devA->AggregateObject (mpiRecA);
      devB->AggregateObject (mpiRecB);
    } */

  devA->Attach (channel, 0); //0 transmitter
  devB->Attach (channel, 1); //1 receiver 
  
  m_nodeListIterator = find (m_nodeList.begin(), m_nodeList.end(), a);
  if (m_nodeListIterator != m_nodeList.end())
    {
     //std::cout << "===node a already in the list : " << *m_nodeListIterator << '\n';
    }
  else
    {
     //std::cout << "===node a not in the list : \n";
     m_nodeList.push_back(a);
     m_containerUni.Add (devA);
    }
  
  m_nodeListIterator = find (m_nodeList.begin(), m_nodeList.end(), b);
  if (m_nodeListIterator != m_nodeList.end())
    {
     //std::cout << "===node b already in the list : " << *m_nodeListIterator << '\n';
    }
  else
    {
     //std::cout << "===node b not in the list : \n";
     m_nodeList.push_back(b);
     m_containerUni.Add (devB);
    }
  return m_containerUni;
}


//happens after leo device is installed
NetDeviceContainer
PointToPointHelper::InstallGES (Ptr<Node> a, Ptr<Node> b)
{            
  Ptr<PointToPointNetDeviceGES> devA = m_deviceGESFactory.Create<PointToPointNetDeviceGES> ();
  devA->IsBroadcast ();
  devA->SetAddress (Mac48Address::Allocate ());
  a->AddDevice (devA);

  uint16_t Ndevice; 
  m_nodeListIterator = find (m_nodeList.begin(), m_nodeList.end(), a);
  if (m_nodeListIterator == m_nodeList.end())
   {
       m_nodeList.push_back(a);
   }
  Ndevice = a->GetNDevices ();
  NS_LOG_UNCOND (a->GetId () << " GES station has device " << Ndevice);
 
  if (Ndevice > 0) //add leo device to ges
    {
        // node a is GES station, only has GES device
    }
  else
   {
      NS_ASSERT ("incorrect device number");
   } 
  
  Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
  devA->SetQueue (queueA);

  
  Ptr<PointToPointNetDeviceGES> devB = m_deviceGESFactory.Create<PointToPointNetDeviceGES> ();
  devB->SetAddress (Mac48Address::Allocate ());
  b->AddDevice (devB);
  
  Ndevice = b->GetNDevices ();
  NS_LOG_UNCOND (b->GetId () << " leo station has device " << Ndevice);
  
  m_nodeListIterator = find (m_nodeList.begin(), m_nodeList.end(), b);
  if (m_nodeListIterator == m_nodeList.end())
   {
       m_nodeList.push_back(b);
   }
  
  if (Ndevice > 0) //add leo device to ges
    {
        // node b is LEO station, only has GES device and LEO device
        Ptr<PointToPointNetDevice> dev0  = m_nodeDeviceMap.find (b->GetId ())->second;
        devB->AddDevice0 (dev0);
        dev0->AddGESDevice (devB);
    }
  else
   {
      NS_ASSERT ("incorrect device number");
   } 
  
    
  Ptr<Queue> queueB = m_queueFactory.Create<Queue> ();
  devB->SetQueue (queueB);
  // If MPI is enabled, we need to see if both nodes have the same system id 
  // (rank), and the rank is the same as this instance.  If both are true, 
  //use a normal p2p channel, otherwise use a remote channel
  bool useNormalChannel = true;
  //Ptr<PointToPointChannel> channel = 0;
  Ptr<PointToPointChannelGES> channel = 0;

  if (MpiInterface::IsEnabled ())
    {
      uint32_t n1SystemId = a->GetSystemId ();
      uint32_t n2SystemId = b->GetSystemId ();
      uint32_t currSystemId = MpiInterface::GetSystemId ();
      if (n1SystemId != currSystemId || n2SystemId != currSystemId) 
        {
          useNormalChannel = false;
        }
      NS_LOG_UNCOND ("MpiInterface::IsEnabled");
    }
  if (useNormalChannel)
    {
      //channel = m_channelFactory.Create<PointToPointChannel> ();
      channel = m_channelGESFactory.Create<PointToPointChannelGES> ();
    }
  /*else
    {
      channel = m_remoteChannelFactory.Create<PointToPointRemoteChannel> ();
      Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver> ();
      Ptr<MpiReceiver> mpiRecB = CreateObject<MpiReceiver> ();
      mpiRecA->SetReceiveCallback (MakeCallback (&PointToPointNetDevice::Receive, devA));
      mpiRecB->SetReceiveCallback (MakeCallback (&PointToPointNetDevice::Receive, devB));
      devA->AggregateObject (mpiRecA);
      devB->AggregateObject (mpiRecB);
    } */

    
  devA->Attach (channel);
  devB->Attach (channel);
  m_containerGES.Add (devA);
  m_containerGES.Add (devB);
  
  NS_LOG_UNCOND ("... ...");

  return m_containerGES; 
}


NetDeviceContainer 
PointToPointHelper::Install (Ptr<Node> a, std::string bName)
{
  Ptr<Node> b = Names::Find<Node> (bName);
  return Install (a, b);
}

NetDeviceContainer 
PointToPointHelper::Install (std::string aName, Ptr<Node> b)
{
  Ptr<Node> a = Names::Find<Node> (aName);
  return Install (a, b);
}

NetDeviceContainer 
PointToPointHelper::Install (std::string aName, std::string bName)
{
  Ptr<Node> a = Names::Find<Node> (aName);
  Ptr<Node> b = Names::Find<Node> (bName);
  return Install (a, b);
}

} // namespace ns3
