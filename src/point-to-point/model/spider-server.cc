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
#include "spider-server.h"
#include "point-to-point-net-device.h"
#include "ns3/net-device.h"

#include "ppp-header.h"






namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SpiderServer");

NS_OBJECT_ENSURE_REGISTERED (SpiderServer);

TypeId
SpiderServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SpiderServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<SpiderServer> ()
    .AddAttribute ("RemoteAddress", 
                   "The MAC address of remote device.",
                   Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                   MakeMac48AddressAccessor (&SpiderServer::m_peerAddress),
                   MakeMac48AddressChecker ())
  ;
  return tid;
}

SpiderServer::SpiderServer ()
{
  NS_LOG_FUNCTION (this);
  m_receive = 0;
  m_receiveEvent = EventId ();
  m_count = 1000;
  m_interval = Seconds (100.0);
  m_size = 1024;
}

SpiderServer::~SpiderServer ()
{
  NS_LOG_FUNCTION (this);
}

void
SpiderServer::SetRemote (Mac48Address ip)
{
  NS_LOG_FUNCTION (this << ip);
  m_peerAddress = ip;
}


void
//SpiderServer::InstallDevice (Ptr<PointToPointNetDevice> dev)
SpiderServer::InstallDevice (Ptr<NetDevice> dev)
{
  NS_LOG_FUNCTION (this << dev);
  m_Device = dev;
      NS_LOG_UNCOND (dev->GetAddress () << " SpiderServer SetReceiveCallback " );

  m_Device->SetSpiderReceiveCallback ( MakeCallback (&SpiderServer::Receive, this) );
}


void
SpiderServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
SpiderServer::StartApplication (Time start)
{
  NS_LOG_FUNCTION (this);
  m_receiveEvent = Simulator::Schedule (start, &SpiderServer::Start, this);
}

void
SpiderServer::StopApplication (Time end)
{
  NS_LOG_FUNCTION (this);
  Simulator::Schedule (end, &SpiderServer::Stop, this);
}


void
SpiderServer::Start (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Server starts");
}
    
void
SpiderServer::Stop (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_receiveEvent);
}



bool
SpiderServer::Receive (Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t, const Address &from)
{
   NS_LOG_FUNCTION (this);
   NS_ASSERT (m_receiveEvent.IsExpired ());
    
   if (from != m_peerAddress  && from != Mac48Address ("ff:ff:ff:ff:ff:ff"))
    {
        //NS_ASSERT ("Incorrect packets to server");
    }
   ++m_receive;
    
  NS_LOG_INFO (Simulator::Now () << ", " << m_Device->GetAddress () << " server receives packet from " << from << " m_receive " << m_receive << " size " << p->GetSize () );
}

} // Namespace ns3
