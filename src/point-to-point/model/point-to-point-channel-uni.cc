/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2008 University of Washington
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
 */

//#include "point-to-point-channel.h"
#include "point-to-point-channel-uni.h"
#include "point-to-point-net-device.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointChannelUni");

NS_OBJECT_ENSURE_REGISTERED (PointToPointChannelUni);

TypeId 
PointToPointChannelUni::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PointToPointChannelUni")
    .SetParent<Channel> ()
    .SetGroupName ("PointToPoint")
    .AddConstructor<PointToPointChannelUni> ()
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&PointToPointChannelUni::m_delay),
                   MakeTimeChecker ())
    .AddTraceSource ("TxRxPointToPoint",
                     "Trace source indicating transmission of packet "
                     "from the PointToPointChannelUni, used by the Animation "
                     "interface.",
                     MakeTraceSourceAccessor (&PointToPointChannelUni::m_txrxPointToPoint),
                     "ns3::PointToPointChannelUni::TxRxAnimationCallback")
  ;
  return tid;
}

//
// By default, you get a channel that 
// has an "infitely" fast transmission speed and zero delay.
PointToPointChannelUni::PointToPointChannelUni()
  :
    Channel (),
    m_delay (Seconds (0.)),
    m_nDevices (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

/*
void
PointToPointChannelUni::AttachSender (Ptr<PointToPointNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT_MSG (m_nDevices == 0, "Only 1 sender devices permitted");
  NS_ASSERT (device != 0);
  m_link[0].m_src = device;
  m_nDevices++;
}
 */ 


void
PointToPointChannelUni::Attach (Ptr<PointToPointNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT_MSG (m_nDevices < N_DEVICES, "Only two devices permitted");
  NS_ASSERT (device != 0);
  
  if (m_nDevices == 0)
    {
       m_link[0].m_src = device;
    }
  else
    {
       m_link[0].m_dst = device;
       m_link[0].m_state = IDLE;
    }
  m_nDevices++;
}

bool
PointToPointChannelUni::TransmitStart (
  Ptr<Packet> p,
  Ptr<PointToPointNetDevice> src,
  Time txTime, uint32_t tx_linkchannel)
{
  NS_LOG_FUNCTION (this << p << src);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  NS_ASSERT (m_link[0].m_state != INITIALIZING);
  NS_LOG_UNCOND ("src " << src->GetAddress () );
  NS_LOG_UNCOND ("src " << m_link[0].m_src->GetAddress () );
  NS_LOG_UNCOND ("dst " << m_link[0].m_dst->GetAddress () );

    NS_ASSERT_MSG (src == m_link[0].m_src, "incorrect sender");


  
  NS_ASSERT_MSG (src == m_link[0].m_src, "incorrect sender");

  //uint32_t wire = src == m_link[0].m_src ? 0 : 1;
  uint32_t wire = 0;
  m_link[wire].m_channel = tx_linkchannel;
  Simulator::ScheduleWithContext (m_link[wire].m_dst->GetNode ()->GetId (),
                                  txTime + m_delay, &PointToPointNetDevice::ReceiveChannel,
                                  m_link[wire].m_dst, p, m_link[wire].m_channel);

  // Call the tx anim callback on the net device
  m_txrxPointToPoint (p, src, m_link[wire].m_dst, txTime, txTime + m_delay);
  return true;
}

uint32_t 
PointToPointChannelUni::GetNDevices (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_nDevices;
}

Ptr<PointToPointNetDevice>
PointToPointChannelUni::GetPointToPointDevice (uint32_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (i < 2);
  if (i ==0)
    {
      return m_link[0].m_src;
    }
  else
    {
      return m_link[0].m_dst;
    }  
}

Ptr<NetDevice>
PointToPointChannelUni::GetDevice (uint32_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return GetPointToPointDevice (i);
}

Time
PointToPointChannelUni::GetDelay (void) const
{
  return m_delay;
}

Ptr<PointToPointNetDevice>
PointToPointChannelUni::GetSource (uint32_t i) const
{
  NS_ASSERT_MSG (i == 0, "incorrect GetSource");  
  return m_link[i].m_src;
}

Ptr<PointToPointNetDevice>
PointToPointChannelUni::GetDestination (uint32_t i) const
{
  NS_ASSERT_MSG (i == 0, "incorrect GetDestination");  
  return m_link[i].m_dst;
}

bool
PointToPointChannelUni::IsInitialized (void) const
{
  NS_ASSERT (m_link[0].m_state != INITIALIZING);
  //NS_ASSERT (m_link[1].m_state != INITIALIZING);
  return true;
}

uint32_t
PointToPointChannelUni::GetLinkChannel (uint32_t i) const
{
  NS_ASSERT_MSG (i == 0, "incorrect GetLinkChannel");  
  return m_link[i].m_channel;
}


} // namespace ns3
