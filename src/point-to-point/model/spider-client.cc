/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <cstdlib>
#include <cstdio>

#include "ns3/mac48-address.h"
#include "spider-client.h"
#include "point-to-point-net-device.h"
#include "ns3/net-device.h"





namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SpiderClient");

NS_OBJECT_ENSURE_REGISTERED (SpiderClient);

TypeId
SpiderClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SpiderClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<SpiderClient> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&SpiderClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (100.0)),
                   MakeTimeAccessor (&SpiderClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress", 
                   "The MAC address of remote device.",
                   Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                   MakeMac48AddressAccessor (&SpiderClient::m_peerAddress),
                   MakeMac48AddressChecker ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&SpiderClient::m_size),
                   MakeUintegerChecker<uint32_t> (12,1500))
  ;
  return tid;
}

SpiderClient::SpiderClient ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_sendEvent = EventId ();
  m_count = 1000;
  m_interval = Seconds (100.0);
  m_size = 1024;
}

SpiderClient::~SpiderClient ()
{
  NS_LOG_FUNCTION (this);
}

void
SpiderClient::SetRemote (Mac48Address ip)
{
  NS_LOG_FUNCTION (this << ip);
  m_peerAddress = ip;
}



void
SpiderClient::SetMaxPackets (uint32_t max)
{
  NS_LOG_FUNCTION (this << max);
  m_count = max;
}

void
SpiderClient::SetInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_interval = interval;
}

void
SpiderClient::SetPacketSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_size = size;
}


void
//SpiderClient::InstallDevice (Ptr<PointToPointNetDevice> dev)
SpiderClient::InstallDevice (Ptr<NetDevice> dev)

{
  NS_LOG_FUNCTION (this << dev);
  m_Device = dev;
}


void
SpiderClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
SpiderClient::StartApplication (Time start)
{
  NS_LOG_FUNCTION (this);

  //m_sendEvent = Simulator::Schedule (Seconds (0.0), &SpiderClient::Send, this);
  m_sendEvent = Simulator::Schedule (start, &SpiderClient::Send, this);
}

void
SpiderClient::StopApplication (Time end)
{
  NS_LOG_FUNCTION (this);
  Simulator::Schedule (end, &SpiderClient::Stop, this);
}


void
SpiderClient::Stop (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
}



  
void
SpiderClient::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());
  
  Ptr<Packet> p = Create<Packet> (m_size); 
  m_Device->Send (p, m_peerAddress, 2048);
  ++m_sent;
  
  NS_LOG_INFO (Simulator::Now () << ", " << m_Device->GetAddress () << " client sent packet to " << m_peerAddress << " m_sent " << m_sent << " size " << m_size );
  
  if (m_sent < m_count)
    {
      m_sendEvent = Simulator::Schedule (m_interval, &SpiderClient::Send, this);
    }
}

} // Namespace ns3
