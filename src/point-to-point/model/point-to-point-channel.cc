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

#include "point-to-point-channel.h"
#include "point-to-point-net-device.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/boolean.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointChannel");

NS_OBJECT_ENSURE_REGISTERED (PointToPointChannel);

TypeId 
PointToPointChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PointToPointChannel")
    .SetParent<Channel> ()
    .SetGroupName ("PointToPoint")
    .AddConstructor<PointToPointChannel> ()
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&PointToPointChannel::m_delay),
                   MakeTimeChecker ())
    .AddAttribute ("UniChannel", "Channel type (uni or bi)",
                   BooleanValue (false),
                   MakeBooleanAccessor (&PointToPointChannel::m_channelUni),
                   MakeBooleanChecker ())
    .AddTraceSource ("TxRxPointToPoint",
                     "Trace source indicating transmission of packet "
                     "from the PointToPointChannel, used by the Animation "
                     "interface.",
                     MakeTraceSourceAccessor (&PointToPointChannel::m_txrxPointToPoint),
                     "ns3::PointToPointChannel::TxRxAnimationCallback")
  ;
  return tid;
}

//
// By default, you get a channel that 
// has an "infitely" fast transmission speed and zero delay.
PointToPointChannel::PointToPointChannel()
  :
    Channel (),
    m_delay (Seconds (0.)),
    m_channelUni (false),
    m_nDevices (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

/*
void
PointToPointChannel::AttachSender (Ptr<PointToPointNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT_MSG (m_nDevices == 0, "Only 1 sender devices permitted");
  NS_ASSERT (device != 0);
  m_link[0].m_src = device;
  m_nDevices++;
}
 */ 


void
PointToPointChannel::Attach (Ptr<PointToPointNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT_MSG (m_nDevices < N_DEVICES, "Only two devices permitted");
  NS_ASSERT (device != 0);
  
  if (IsChannelUni () )
    {
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
 else
  {
     m_link[m_nDevices++].m_src = device;
     if (m_nDevices == N_DEVICES)
      {
         m_link[0].m_dst = m_link[1].m_src;
         m_link[1].m_dst = m_link[0].m_src;
         m_link[0].m_state = IDLE;
         m_link[1].m_state = IDLE;
       }    
  }
}

bool
PointToPointChannel::TransmitStart (
  Ptr<Packet> p,
  Ptr<PointToPointNetDevice> src,
  Time txTime, uint32_t tx_linkchannel)
{
  NS_LOG_FUNCTION (this << p << src);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");
  if (IsChannelUni () )
    {
        NS_ASSERT (m_link[0].m_state != INITIALIZING);
        NS_LOG_DEBUG ("src " << src->GetAddress () );
        NS_LOG_DEBUG ("src " << m_link[0].m_src->GetAddress () );
        NS_LOG_DEBUG ("dst " << m_link[0].m_dst->GetAddress () );

        NS_ASSERT_MSG (src == m_link[0].m_src, "incorrect sender");

        //uint32_t wire = src == m_link[0].m_src ? 0 : 1;
        uint32_t wire = 0;
         m_link[wire].m_channel = tx_linkchannel;
        Simulator::ScheduleWithContext (m_link[wire].m_dst->GetNode ()->GetId (),
                                  txTime + m_delay, &PointToPointNetDevice::ReceiveChannel,
                                  m_link[wire].m_dst, p, m_link[wire].m_channel);

        // Call the tx anim callback on the net device
        m_txrxPointToPoint (p, src, m_link[wire].m_dst, txTime, txTime + m_delay);
    }
  else
    {
        NS_ASSERT (m_link[0].m_state != INITIALIZING);
        NS_ASSERT (m_link[1].m_state != INITIALIZING);

        uint32_t wire = src == m_link[0].m_src ? 0 : 1;
        m_link[wire].m_channel = tx_linkchannel;
        Simulator::ScheduleWithContext (m_link[wire].m_dst->GetNode ()->GetId (),
                                  txTime + m_delay, &PointToPointNetDevice::ReceiveChannel,
                                  m_link[wire].m_dst, p, m_link[wire].m_channel);

        // Call the tx anim callback on the net device
        m_txrxPointToPoint (p, src, m_link[wire].m_dst, txTime, txTime + m_delay); 
    }
  return true;
}

uint32_t 
PointToPointChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_nDevices;
}

Ptr<PointToPointNetDevice>
PointToPointChannel::GetPointToPointDevice (uint32_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (i < 2);
if (IsChannelUni () )
 {
  if (i ==0)
    {
      return m_link[0].m_src;
    }
  else
    {
      return m_link[0].m_dst;
    }  
 }
else
 {
    return m_link[i].m_src; 
 }
}

Ptr<NetDevice>
PointToPointChannel::GetDevice (uint32_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return GetPointToPointDevice (i);
}

Time
PointToPointChannel::GetDelay (void) const
{
  return m_delay;
}

Ptr<PointToPointNetDevice>
PointToPointChannel::GetSource (uint32_t i) const
{
  if (IsChannelUni () )
  {
    NS_ASSERT_MSG (i == 0, "incorrect GetSource");  
  }
  return m_link[i].m_src;
}

Ptr<PointToPointNetDevice>
PointToPointChannel::GetDestination (uint32_t i) const
{
  if (IsChannelUni () )
  {
    NS_ASSERT_MSG (i == 0, "incorrect GetSource");  
  } 
  return m_link[i].m_dst;
}

bool
PointToPointChannel::IsInitialized (void) const
{
 if (IsChannelUni () )
   {
     NS_ASSERT (m_link[0].m_state != INITIALIZING);
     //NS_ASSERT (m_link[1].m_state != INITIALIZING);
   }
  else
   {
     NS_ASSERT (m_link[0].m_state != INITIALIZING);
     NS_ASSERT (m_link[1].m_state != INITIALIZING);      
   }
  return true;
}

uint32_t
PointToPointChannel::GetLinkChannel (uint32_t i) const
{
 if (IsChannelUni () )
  {
    NS_ASSERT_MSG (i == 0, "incorrect GetSource");  
  } 
  return m_link[i].m_channel;
}

bool 
PointToPointChannel::IsChannelUni (void) const
{
    return m_channelUni;
}


} // namespace ns3
