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

#include "point-to-point-channel-ges.h"
#include "point-to-point-net-device-ges.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

#include <algorithm>    // std::find


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointChannelGES");

NS_OBJECT_ENSURE_REGISTERED (PointToPointChannelGES);

TypeId 
PointToPointChannelGES::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PointToPointChannelGES")
    .SetParent<Channel> ()
    .SetGroupName ("PointToPoint")
    .AddConstructor<PointToPointChannelGES> ()
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (MicroSeconds (2000)),
                   //TimeValue (MicroSeconds (14400)),
                   MakeTimeAccessor (&PointToPointChannelGES::m_delay),
                   MakeTimeChecker ())
    .AddTraceSource ("TxRxPointToPoint",
                     "Trace source indicating transmission of packet "
                     "from the PointToPointChannelGES, used by the Animation "
                     "interface.",
                     MakeTraceSourceAccessor (&PointToPointChannelGES::m_txrxPointToPoint),
                     "ns3::PointToPointChannelGES::TxRxAnimationCallback")
  ;
  return tid;
}

//
// By default, you get a channel that 
// has an "infitely" fast transmission speed and zero delay.
PointToPointChannelGES::PointToPointChannelGES()
  :
    Channel (),
    m_delay (Seconds (0.)),
    m_nDevices (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}


void
PointToPointChannelGES::Attach (Ptr<PointToPointNetDeviceGES> deviceA, Ptr<PointToPointNetDeviceGES> deviceB)
{
  NS_LOG_FUNCTION (this );
  NS_ASSERT (deviceA != 0);
  NS_ASSERT (deviceB != 0);
  bool deviceAExist = false;
  bool deviceBExist = false;
  
         NS_LOG_UNCOND ("add to list device " << deviceA->GetAddress () << ", size" << m_deviceList.size ());

         
  m_deviceListIterator = find (m_deviceList.begin(), m_deviceList.end(), deviceA);
  if (m_deviceListIterator == m_deviceList.end())
   {
       m_deviceList.push_back(deviceA);
       m_DeviceLinkMap[deviceA] = m_nDevices;
       m_nDevices++;
       deviceAExist = true;
       NS_LOG_UNCOND ("add to list device " << deviceA->GetAddress () << ", size" << m_deviceList.size ());
   }
  
  m_deviceListIterator = find (m_deviceList.begin(), m_deviceList.end(), deviceB);
  if (m_deviceListIterator == m_deviceList.end())
   {
       m_deviceList.push_back(deviceB);
       m_DeviceLinkMap[deviceB] = m_nDevices;
       m_nDevices++;
       deviceBExist = true;
       NS_LOG_UNCOND ("add to list device " << deviceB->GetAddress ());
   }
   NS_ASSERT (deviceA && deviceB);
  
  
    uint32_t a  = m_DeviceLinkMap.find (deviceA)->second;
    uint32_t b  = m_DeviceLinkMap.find (deviceB)->second;
  
   // each device has one unique number, the link denotes as [id1][id2]
      m_link[a][b].m_src = deviceA;
      m_link[b][a].m_src = deviceB;
              
      m_link[a][b].m_dst = m_link[b][a].m_src;
      m_link[b][a].m_dst = m_link[a][b].m_src;      
      m_link[a][b].m_state = IDLE;
      m_link[b][a].m_state = IDLE;
      
      
      
      NS_LOG_UNCOND ("channel between device " << deviceA->GetAddress () << " and " << deviceB->GetAddress ());
} 

bool
PointToPointChannelGES::TransmitStart (
  Ptr<Packet> p,
  Ptr<PointToPointNetDeviceGES> src,
  Mac48Address dst,
  Time txTime)
{
  NS_LOG_FUNCTION (this << p << src);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");
  
  
  
   //std::list< Ptr<PointToPointNetDeviceGES> >::iterator m_deviceListIterator;

   uint32_t a  = m_DeviceLinkMap.find (src)->second;
   uint32_t b  = N_DEVICES;

   std::list< Ptr<PointToPointNetDeviceGES> >::iterator it;;

        

  for ( it=m_deviceList.begin(); it!=m_deviceList.end(); ++it)
  {      
      // NS_LOG_UNCOND (" m_deviceList " << (*it)->GetAddress ());

         
      if ( Mac48Address::ConvertFrom ((*it)->GetAddress ()) == dst)
      {
           b  = m_DeviceLinkMap.find (*it)->second;
           break;
      }  
  }
   //NS_LOG_UNCOND ("b " << b << ", src " << src->GetAddress () << ", dst " << dst);
   NS_ASSERT (b != N_DEVICES);

  

  NS_ASSERT (m_link[a][b].m_state != INITIALIZING);
  NS_ASSERT (m_link[b][a].m_state != INITIALIZING);

  //uint32_t wire = src == m_link[0].m_src ? 0 : 1;

  Simulator::ScheduleWithContext (m_link[a][b].m_dst->GetNode ()->GetId (),
                                  txTime + m_delay, &PointToPointNetDeviceGES::Receive,
                                  m_link[a][b].m_dst, p);

  // Call the tx anim callback on the net device
  //m_txrxPointToPoint (p, src, m_link[wire].m_dst, txTime, txTime + m_delay);
  return true;
}



uint32_t 
PointToPointChannelGES::GetNDevices (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_nDevices;
}

Ptr<PointToPointNetDeviceGES>
PointToPointChannelGES::GetPointToPointDevice (uint32_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (i < 2);
  return m_link[i][0].m_src;
}

Ptr<NetDevice>
PointToPointChannelGES::GetDevice (uint32_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return GetPointToPointDevice (i);
}

Time
PointToPointChannelGES::GetDelay (void) const
{
  return m_delay;
}

Ptr<PointToPointNetDeviceGES>
PointToPointChannelGES::GetSource (uint32_t i) const
{
  return m_link[i][0].m_src;
}

Ptr<PointToPointNetDeviceGES>
PointToPointChannelGES::GetDestination (uint32_t i) const
{
  return m_link[i][0].m_dst;
}

bool
PointToPointChannelGES::IsInitialized (void) const
{
  //NS_ASSERT (m_link[0].m_state != INITIALIZING);
  //NS_ASSERT (m_link[1].m_state != INITIALIZING);
  return true;
}



/* idea
//position of mine
GES0
updateLink
{
    devicelist
    m_linkDst[*] for all leo device
            
       //input but already know     
        m_InitPos_dstLES[0] = 0b00000000001; // initial position of the LES, they are same for all the GES
        m_InitPos_dstLES[1] = 0b00000000010; // and apparently known (it is defined in this way), then the m_InitPos_dstGES follows.
        m_InitPos_dstLES[2] = 0b00000000100;
        ...
        m_InitPos_dstLES[10] = 0b10000000000;
    
        //input
        Indicator = 0b00000000001; // my position, sh
         
    
       //fixed below
        m_linkDst[dst0] = m_InitPos_dstLES[0] & Indicator;
        m_linkDst[dst1] = m_InitPos_dstLES[1] & Indicator;
        m_linkDst[dst2] = m_InitPos_dstLES[2] & Indicator;
        m_linkDst[dst3] = m_InitPos_dstLES[3] & Indicator;
        
        Indicator << 1;

        Sechedule (Interval, updateLink, this)
}


GES1
updateLink
{
    devicelist
    m_linkDst[*] for all leo device
            
       //input but already know     
        m_InitPos_dstLES[0] = 0b00000000001; // initial position of the LES, they are same for all the GES
        m_InitPos_dstLES[1] = 0b00000000010; // and apparently known (it is defined in this way), then the m_InitPos_dstGES follows.
        m_InitPos_dstLES[2] = 0b00000000100;
        ...
        m_InitPos_dstLES[10] = 0b10000000000;
    
        //input
        Indicator = 0b00000000100; // my position, sh
         
    
       //fixed below
        shift = 1;
        shift * pow(2, i);
        m_linkDst[dst0] = Indicator & 0b00000000001;
        m_linkDst[dst1] = Indicator & 0b00000000010;
        m_linkDst[dst2] = Indicator & 0b00000000100;
        m_linkDst[dst3] = Indicator & 0b00000001000;
        
        Indicator << 1;

        Sechedule (Interval, updateLink, this)
}

//most bit-least bit (right, left))


//parameters: position of GESs and mine
LEO-0
updateLink
{
    devicelist for all ges
    m_linkDst[*] for all device
    
    //input        
    m_InitPos_dstGES[0] = 0b00000000001; // initial position of the GES, ges has the same position as LEO
    m_InitPos_dstGES[1] = 0b00000001000; // they should be same for all the LEO
    
    Indicator =   0b00000000001; // my leo distance with ges, can be derived from leo id
            
     //fixed below  
        m_linkDstGES[0] = m_InitPos_dstGES[0] & Indicator;
        m_linkDstGES[1] = m_InitPos_dstGES[1] & Indicator;
      
        Indicator >> 1;
        Sechedule (Interval, updateLink, this)
}


LEO-1
updateLink
{
    devicelist for all ges
    m_linkDst[*] for all device
    
    m_InitPos_dstGES[0] = 0b00000000001;
    m_InitPos_dstGES[1] = 0b00000001000;
    Indicator =   0b00000000010;
            
    //fixed below   
        m_linkDstGES[0] = m_InitPos_dstGES[0] & Indicator;
        m_linkDstGES[1] = m_InitPos_dstGES[1] & Indicator;
     
        Indicator >> 1;
        Sechedule (Interval, updateLink, this)
}



LEO-2
updateLink
{
    devicelist for all ges
    m_linkDst[*] for all device
    
    //input        
    m_InitPos_dstGES[0] = 0b00000000001; // initial position of the GES, ges has the same position as LEO
    m_InitPos_dstGES[1] = 0b00000001000; // they should be same for all the LEO
    
    Indicator =   0b00000000100; // my leo distance with ges, can be derived from leo id
            
     //fixed below  
        m_linkDstGES[0] = m_InitPos_dstGES[0] & Indicator;
        m_linkDstGES[1] = m_InitPos_dstGES[1] & Indicator;
      
        Indicator >> 1;
        Sechedule (Interval, updateLink, this)
*/

} // namespace ns3
