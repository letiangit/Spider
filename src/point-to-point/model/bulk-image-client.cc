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
#include "ns3/boolean.h"
#include <cstdlib>
#include <cstdio>

#include "ns3/mac48-address.h"
#include "bulk-image-client.h"
#include "point-to-point-net-device.h"
#include "ns3/net-device.h"

#include "ppp-header.h"

#include <iostream>
#include <fstream>


using namespace std;


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BulkImageClient");

NS_OBJECT_ENSURE_REGISTERED (BulkImageClient);

TypeId
BulkImageClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BulkImageClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<BulkImageClient> ()
    .AddAttribute ("ImageBundleCount",
                   "the number of image application-layer bundles",
                   UintegerValue (20),
                   MakeUintegerAccessor (&BulkImageClient::m_img_bundle_count),
                   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("ImageQoS",
				   "The QoS value of image packets",
				   UintegerValue (1),
				   MakeUintegerAccessor (&BulkImageClient::m_img_qos),
				   MakeUintegerChecker<uint32_t> (0,3))
	.AddAttribute ("ImageBundleSize",
				   "The size in bytes of each image bundle",
				   UintegerValue (102400),
				   MakeUintegerAccessor (&BulkImageClient::m_img_bundle_size),
				   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("PriorityDelay",
                   "The delay of the priority packet",
				   TimeValue(Seconds (5.0)),
                   MakeTimeAccessor (&BulkImageClient::m_prio_delay),
                   MakeTimeChecker ())
    .AddAttribute ("TMTCDelay",
                   "The delay of the TM/TC packet",
				   TimeValue(Seconds (5.0)),
                   MakeTimeAccessor (&BulkImageClient::m_tmtc_delay),
                   MakeTimeChecker ())
    .AddAttribute ("PriorityBundleCount",
                   "the number of priority bundles",
                   UintegerValue (1),
                   MakeUintegerAccessor (&BulkImageClient::m_prio_bundle_count),
                   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("PriorityQoS",
				   "The QoS value of priority packets",
				   UintegerValue (2),
				   MakeUintegerAccessor (&BulkImageClient::m_prio_qos),
				   MakeUintegerChecker<uint32_t> (0,3))
	.AddAttribute ("PriorityBundleSize",
				   "The size in bytes of each priority bundle",
				   UintegerValue (102400),
				   MakeUintegerAccessor (&BulkImageClient::m_prio_bundle_size),
				   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TMTCBundleCount",
                   "the number of priority TM/TC bundles",
                   UintegerValue (1),
                   MakeUintegerAccessor (&BulkImageClient::m_tmtc_bundle_count),
                   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("TMTCQoS",
				   "The QoS value of TM/TC packets",
				   UintegerValue (3),
				   MakeUintegerAccessor (&BulkImageClient::m_tmtc_qos),
				   MakeUintegerChecker<uint32_t> (0,3))
	.AddAttribute ("TMTCBundleSize",
				   "The size in bytes of each TM/TC bundle",
				   UintegerValue (5120),
				   MakeUintegerAccessor (&BulkImageClient::m_tmtc_bundle_size),
				   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("TMTCEnabled",
				   "True if TM/TC traffic is enabled",
				   BooleanValue (false),
				   MakeBooleanAccessor (&BulkImageClient::m_tmtc_enabled),
				   MakeBooleanChecker ())
	.AddAttribute ("ImageEnabled",
				   "True if image traffic is enabled",
				   BooleanValue (true),
				   MakeBooleanAccessor (&BulkImageClient::m_img_enabled),
				   MakeBooleanChecker ())
	.AddAttribute ("PriorityEnabled",
				   "True if priority traffic is enabled",
				   BooleanValue (true),
				   MakeBooleanAccessor (&BulkImageClient::m_prio_enabled),
				   MakeBooleanChecker ())
    .AddAttribute ("RemoteAddress", 
                   "The MAC address of remote device.",
                   Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                   MakeMac48AddressAccessor (&BulkImageClient::m_peerAddress),
                   MakeMac48AddressChecker ())
  ;
  return tid;
}

BulkImageClient::BulkImageClient ()
{
  NS_LOG_FUNCTION (this);
  m_prioritySendEvent = EventId ();
  m_imageSendEvent = EventId ();
  chunksize = 2647;
  m_img_bundle_count = 20;
  m_img_qos = 1;
  m_img_bundle_size = 102400;
  m_prio_delay = Seconds(5.0);
  m_tmtc_delay = Seconds(5.0);
  m_prio_bundle_count = 1;
  m_prio_qos = 2;
  m_prio_bundle_size = 102400;
  m_tmtc_bundle_count = 1;
  m_tmtc_qos = 3;
  m_tmtc_bundle_size = 5120;
  m_tmtc_enabled = false;
  m_img_enabled = true;
  m_prio_enabled = true;
}

BulkImageClient::~BulkImageClient ()
{
  NS_LOG_FUNCTION (this);
}

void
BulkImageClient::SetRemote (Mac48Address ip)
{
  NS_LOG_FUNCTION (this << ip);
  m_peerAddress = ip;
}



void
BulkImageClient::SetImageBundleCount (uint32_t count)
{
  NS_LOG_FUNCTION (this << count);
  m_img_bundle_count = count;
}

void
BulkImageClient::SetImageQoS (uint32_t qos)
{
  NS_LOG_FUNCTION (this << qos);
  m_img_qos = qos;
}
void
BulkImageClient::SetImageBundleSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_img_bundle_size = size;
}

void
BulkImageClient::SetPriorityDelay (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_prio_delay = interval;
}

void
BulkImageClient::SetPriorityBundleCount (uint32_t count)
{
  NS_LOG_FUNCTION (this << count);
  m_prio_bundle_count = count;
}

void
BulkImageClient::SetPriorityQoS (uint32_t qos)
{
  NS_LOG_FUNCTION (this << qos);
  m_prio_qos = qos;
}
void
BulkImageClient::SetPriorityBundleSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_tmtc_bundle_size = size;
}

void
BulkImageClient::SetTMTCBundleCount (uint32_t count)
{
  NS_LOG_FUNCTION (this << count);
  m_tmtc_bundle_count = count;
}

void
BulkImageClient::SetTMTCQoS (uint32_t qos)
{
  NS_LOG_FUNCTION (this << qos);
  m_tmtc_qos = qos;
}
void
BulkImageClient::SetTMTCBundleSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_tmtc_bundle_size = size;
}

void
BulkImageClient::SetTMTCDelay (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_tmtc_delay = interval;
}

void
BulkImageClient::EnableImage (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_img_enabled = enable;
}

void
BulkImageClient::EnablePriority (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_prio_enabled = enable;
}

void
BulkImageClient::EnableTMTC (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_tmtc_enabled = enable;
}



void
BulkImageClient::SetOutputpath (std::string path)
{
  NS_LOG_FUNCTION (this << path);
  m_Outputpath = path;
}

void
BulkImageClient::InstallDevice (Ptr<NetDevice> dev)

{
  NS_LOG_FUNCTION (this << dev);
  m_Device = dev;
}


void
BulkImageClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
BulkImageClient::StartApplication (Time start)
{
  NS_LOG_FUNCTION (this);

  //m_sendEvent = Simulator::Schedule (Seconds (0.0), &BulkImageClient::Send, this);
  if (m_img_enabled)
  	m_imageSendEvent = Simulator::Schedule (start, &BulkImageClient::SendImage, this);
  if (m_prio_enabled)
  	m_prioritySendEvent = Simulator::Schedule (start + m_prio_delay, &BulkImageClient::SendPriorityTraffic, this);
  if (m_tmtc_enabled)
  	m_tmtcSendEvent = Simulator::Schedule (start + m_tmtc_delay, &BulkImageClient::SendTMTCTraffic, this);
}

void
BulkImageClient::StopApplication (Time end)
{
  NS_LOG_FUNCTION (this);
  Simulator::Schedule (end, &BulkImageClient::Stop, this);
}


void
BulkImageClient::Stop (void)
{
  NS_LOG_FUNCTION (this);
  if (m_img_enabled)
  	Simulator::Cancel (m_imageSendEvent);
  if (m_prio_enabled)
  	Simulator::Cancel (m_prioritySendEvent);
  if (m_tmtc_enabled)  
  	Simulator::Cancel (m_tmtcSendEvent);
}
  
void
BulkImageClient::SendImage (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_imageSendEvent.IsExpired ());
  

  //uint32_t packets = std::ceil(m_img_bundle_size * m_img_bundle_count / (1.0 * chunksize));
  
  for (uint32_t bb = 0; bb < m_img_bundle_count; bb++) {
	  uint32_t packets = std::ceil(m_img_bundle_size / (1.0 * chunksize));
	  cout << "GBT\t" << Simulator::Now().GetMilliSeconds() << "\t" << m_Device->GetAddress () << "\t" << m_peerAddress << "\t" << m_img_bundle_size << "\t" << packets << "\t" << chunksize << "\t" << m_img_qos << endl;
  	for (uint32_t kk = 0; kk < packets; kk++)
  	{
        Ptr<Packet> p = Create<Packet>(chunksize);
		//(*PACKET_TIMES)[p->GetUid()] = Simulator::Now().GetMilliSeconds();
		cout << "GEN\t" << Simulator::Now().GetMilliSeconds() << "\t" << m_Device->GetAddress () << "\t" << m_peerAddress << "\t" << p->GetUid() << "\t" << chunksize <<  "\t" << m_img_qos << endl;
        m_Device->Send (p, m_peerAddress, 2048+m_img_qos);
        NS_LOG_INFO (Simulator::Now () << ", " << m_Device->GetAddress () << " client sent chunks to " << m_peerAddress  << " chunksize  " << chunksize );
  	}
  }
}

void
BulkImageClient::SendPriorityTraffic (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_prioritySendEvent.IsExpired ());
  

  //uint32_t packets = std::ceil(m_prio_bundle_size * m_prio_bundle_count / (1.0 * chunksize));
  for (uint32_t bb = 0; bb < m_prio_bundle_count; bb++) {
	  uint32_t packets = std::ceil(m_prio_bundle_size / (1.0 * chunksize));
	  cout << "GBT\t" << Simulator::Now().GetMilliSeconds() << "\t" << m_Device->GetAddress () << "\t" << m_peerAddress << "\t" << m_prio_bundle_size << "\t" << packets << "\t" << chunksize << "\t" << m_prio_qos << endl;
  	for (uint32_t kk = 0; kk < packets; kk++)
  	{
        Ptr<Packet> p = Create<Packet>(chunksize); 
        m_Device->Send (p, m_peerAddress, 2048+m_prio_qos);
		cout << "GEN\t" << Simulator::Now().GetMilliSeconds() << "\t" << m_Device->GetAddress () << "\t" << m_peerAddress << "\t" << p->GetUid() << "\t" << chunksize << "\t" << m_prio_qos << endl;
        NS_LOG_INFO (Simulator::Now () << ", " << m_Device->GetAddress () << " client sent chunks to " << m_peerAddress  << " chunksize  " << chunksize );
  	}
	}
}

void
BulkImageClient::SendTMTCTraffic (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_tmtcSendEvent.IsExpired ());
  

  //uint32_t packets = std::ceil(m_prio_bundle_size * m_prio_bundle_count / (1.0 * chunksize));
  for (uint32_t bb = 0; bb < m_tmtc_bundle_count; bb++) {
	  uint32_t packets = std::ceil(m_tmtc_bundle_size / (1.0 * chunksize));
	  cout << "GBT\t" << Simulator::Now().GetMilliSeconds() << "\t" << m_Device->GetAddress () << "\t" << m_peerAddress << "\t" << m_tmtc_bundle_size << "\t" << packets << "\t" << chunksize << "\t" << m_tmtc_qos << endl;
  	for (uint32_t kk = 0; kk < packets; kk++)
  	{
        Ptr<Packet> p = Create<Packet>(chunksize); 
        m_Device->Send (p, m_peerAddress, 2048+m_tmtc_qos);
		cout << "GEN\t" << Simulator::Now().GetMilliSeconds() << "\t" << m_Device->GetAddress () << "\t" << m_peerAddress << "\t" << p->GetUid() << "\t" << chunksize << "\t" << m_tmtc_qos << endl;
        NS_LOG_INFO (Simulator::Now () << ", " << m_Device->GetAddress () << " client sent chunks to " << m_peerAddress  << " chunksize  " << chunksize );
  	}
	}
}


} // Namespace ns3
