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

#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/llc-snap-header.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
//#include "point-to-point-net-device.h"
#include "point-to-point-net-device-ges.h"
//#include "point-to-point-channel.h"
#include "point-to-point-channel-ges.h"
#include "ppp-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointNetDeviceGES");

NS_OBJECT_ENSURE_REGISTERED (PointToPointNetDeviceGES);

TypeId 
PointToPointNetDeviceGES::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PointToPointNetDeviceGES")
    .SetParent<NetDevice> ()
    .SetGroupName ("PointToPoint")
    .AddConstructor<PointToPointNetDeviceGES> ()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (DEFAULT_MTU),
                   MakeUintegerAccessor (&PointToPointNetDeviceGES::SetMtu,
                                         &PointToPointNetDeviceGES::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Address", 
                   "The MAC address of this device.",
                   Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                   MakeMac48AddressAccessor (&PointToPointNetDeviceGES::m_address),
                   MakeMac48AddressChecker ())
    .AddAttribute ("DataRate", 
                   "The default data rate for point to point links",
                   DataRateValue (DataRate ("32768b/s")),
                   MakeDataRateAccessor (&PointToPointNetDeviceGES::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("ReceiveErrorModel", 
                   "The receiver error model used to simulate packet loss",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDeviceGES::m_receiveErrorModel),
                   MakePointerChecker<ErrorModel> ())
    .AddAttribute ("InterframeGap", 
                   "The time to wait between packet (frame) transmissions",
                   TimeValue (Seconds (0.0)),
                   MakeTimeAccessor (&PointToPointNetDeviceGES::m_tInterframeGap),
                   MakeTimeChecker ())

    //
    // Transmit queueing discipline for the device which includes its own set
    // of trace hooks.
    //
    .AddAttribute ("TxQueue", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDeviceGES::m_queue),
                   MakePointerChecker<Queue> ())

    //
    // Trace sources at the "top" of the net device, where packets transition
    // to/from higher layers.
    //
    .AddTraceSource ("MacTx", 
                     "Trace source indicating a packet has arrived "
                     "for transmission by this device",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_macTxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxDrop", 
                     "Trace source indicating a packet has been dropped "
                     "by the device before transmission",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_macTxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacPromiscRx", 
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a promiscuous trace,",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_macPromiscRxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacRx", 
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a non-promiscuous trace,",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_macRxTrace),
                     "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("MacRxDrop", 
                     "Trace source indicating a packet was dropped "
                     "before being forwarded up the stack",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_macRxDropTrace),
                     "ns3::Packet::TracedCallback")
#endif
    //
    // Trace souces at the "bottom" of the net device, where packets transition
    // to/from the channel.
    //
    .AddTraceSource ("PhyTxBegin", 
                     "Trace source indicating a packet has begun "
                     "transmitting over the channel",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_phyTxBeginTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxEnd", 
                     "Trace source indicating a packet has been "
                     "completely transmitted over the channel",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_phyTxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxDrop", 
                     "Trace source indicating a packet has been "
                     "dropped by the device during transmission",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_phyTxDropTrace),
                     "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("PhyRxBegin", 
                     "Trace source indicating a packet has begun "
                     "being received by the device",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_phyRxBeginTrace),
                     "ns3::Packet::TracedCallback")
#endif
    .AddTraceSource ("PhyRxEnd", 
                     "Trace source indicating a packet has been "
                     "completely received by the device",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_phyRxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxDrop", 
                     "Trace source indicating a packet has been "
                     "dropped by the device during reception",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")

    //
    // Trace sources designed to simulate a packet sniffer facility (tcpdump).
    // Note that there is really no difference between promiscuous and 
    // non-promiscuous traces in a point-to-point link.
    //
    .AddTraceSource ("Sniffer", 
                    "Trace source simulating a non-promiscuous packet sniffer "
                     "attached to the device",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_snifferTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PromiscSniffer", 
                     "Trace source simulating a promiscuous packet sniffer "
                     "attached to the device",
                     MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_promiscSnifferTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RecPacketTrace",
                    "Channel has been successfully Selected"
                    "The header of successfully transmitted packet",
                    MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_recPacketTrace),
                    "ns3::PointToPointNetDevice::RecPacketCallback")
  ;
  return tid;
}


PointToPointNetDeviceGES::PointToPointNetDeviceGES () 
  :
    m_txMachineState (READY),
    m_channel (0),
    m_linkUp (false),
    m_DevSameNode0Flag (false),
    m_packetId (0),
    m_currentPkt (0)
{
  NS_LOG_FUNCTION (this);
}

PointToPointNetDeviceGES::~PointToPointNetDeviceGES ()
{
  NS_LOG_FUNCTION (this);
}

void
PointToPointNetDeviceGES::AddHeader (Ptr<Packet> p, uint16_t protocolNumber)
{
   NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", AddHeader , protocolNumber " << protocolNumber );

  NS_LOG_FUNCTION (this << p << protocolNumber);
  PppHeader ppp;
  ppp.SetProtocol (EtherToPpp (protocolNumber));
  m_type =  DATATYPE; //temporary, should also includes linkstate 
  ppp.SetType (m_type);
  ppp.SetSourceAddre (Mac48Address::ConvertFrom (GetAddress ()));
  ppp.SetDestAddre (m_destAddress);
  m_Qos = 0; //temporary, should also includes linkstate 
  ppp.SetQos (m_Qos);
  ppp.SetTTL (0);
  ppp.SetID (m_packetId);
  m_packetId++;

  p->AddHeader (ppp);
  

}


bool
PointToPointNetDeviceGES::ProcessHeader (Ptr<Packet> p, uint16_t& param)
{
  NS_LOG_FUNCTION (this << p << param);
  PppHeader ppp;
  p->RemoveHeader (ppp);
  param = PppToEther (ppp.GetProtocol ());
  return true;
}

void
PointToPointNetDeviceGES::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_node = 0;
  m_channel = 0;
  m_receiveErrorModel = 0;
  m_currentPkt = 0;
  NetDevice::DoDispose ();
}

void
PointToPointNetDeviceGES::SetDataRate (DataRate bps)
{
  NS_LOG_FUNCTION (this);
  m_bps = bps;
}

void
PointToPointNetDeviceGES::SetInterframeGap (Time t)
{
  NS_LOG_FUNCTION (this << t.GetSeconds ());
  m_tInterframeGap = t;
}

bool
PointToPointNetDeviceGES::TransmitStart (Ptr<Packet> p, Mac48Address dst)
{
  NS_LOG_FUNCTION (this << p);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  //
  // This function is called to start the process of transmitting a packet.
  // We need to tell the channel that we've started wiggling the wire and
  // schedule an event that will be executed when the transmission is complete.
  //
  NS_ASSERT_MSG (m_txMachineState == READY, "Must be READY to transmit");
  m_txMachineState = BUSY;
  m_currentPkt = p;
  m_phyTxBeginTrace (m_currentPkt);

  Time txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
  Time txCompleteTime = txTime + m_tInterframeGap;

  NS_LOG_LOGIC ("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds () << "sec");
  Simulator::Schedule (txCompleteTime, &PointToPointNetDeviceGES::TransmitComplete, this);
  NS_LOG_UNCOND (GetAddress () << " send to another end " << dst);
  if ( m_DevSameNode0Flag) //attached to LEO
    {
      //input dst is not used, 24032018
      dst = m_dstGESAddr;
    }
  else //attached to GES
    { 
      dst = m_dstLEOAddr;
    }
  
    NS_LOG_UNCOND (GetAddress () << " send to another end " << dst);
  
  if (!m_DevSameNode0Flag && dst == Mac48Address ("00:00:00:00:00:00") ) //unreachable dst
    {
      NS_ASSERT ("GES is not connected to any satellite");
    }
  
   bool result;
  if ( dst == Mac48Address ("00:00:00:00:00:00") )//unreachable dst
    {
        result = false;
        NS_LOG_UNCOND ("LEO " << GetAddress () << " is not connected to any GES");
    }
  else
    {
        result = m_channel->TransmitStart (p, this, dst ,txTime);
    }  
            
  if (result == false)
    {
      m_phyTxDropTrace (p);
    }
  return result;
}

void
PointToPointNetDeviceGES::TransmitComplete (void)
{
  NS_LOG_FUNCTION (this);

  //
  // This function is called to when we're all done transmitting a packet.
  // We try and pull another packet off of the transmit queue.  If the queue
  // is empty, we are done, otherwise we need to start transmitting the
  // next packet.
  //
  NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if transmitting");
  m_txMachineState = READY;

  NS_ASSERT_MSG (m_currentPkt != 0, "PointToPointNetDeviceGES::TransmitComplete(): m_currentPkt zero");

  m_phyTxEndTrace (m_currentPkt);
  m_currentPkt = 0;

  Ptr<Packet> p = m_queue->Dequeue ();
  if (p == 0)
    {
      //
      // No packet was on the queue, so we just exit.
      //
      return;
    }
  
  PppHeader ppp;
  p->PeekHeader (ppp);

  //
  // Got another packet off of the queue, so start the transmit process agin.
  //
  m_snifferTrace (p);
  m_promiscSnifferTrace (p);
  TransmitStart (p, ppp.GetDestAddre ());
}

bool
PointToPointNetDeviceGES::Attach (Ptr<PointToPointChannelGES> ch)
{
  NS_LOG_FUNCTION (this << &ch);

  m_channel = ch;

  //m_channel->Attach (this);

  //
  // This device is up whenever it is attached to a channel.  A better plan
  // would be to have the link come up when both devices are attached, but this
  // is not done for now.
  //
  NotifyLinkUp ();
  return true;
}

void
PointToPointNetDeviceGES::SetQueue (Ptr<Queue> q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}

void
PointToPointNetDeviceGES::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void
PointToPointNetDeviceGES::Receive (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  uint16_t protocol = 0;
  
  
    PppHeader ppp;
    packet->PeekHeader (ppp);
    

      uint16_t ttl = ppp.GetTTL ();
    
      Ptr<Packet> CopyPacket = packet->Copy ();
    
    if (ppp.GetSourceAddre () == Mac48Address::ConvertFrom (GetAddress() ))
      {
          NS_ASSERT ("GES drop packet due to loop" );
          return;
      }
      
    if (m_DevSameNode0Flag && ppp.GetSourceAddre () == Mac48Address::ConvertFrom (m_DevSameNode0->GetAddress () ))
      {
          NS_ASSERT ("GES drop packet due to loop" );
          return;
      }
  
  NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", receive GES data packet from " << ppp.GetSourceAddre () << ", size " <<packet->GetSize () << ", id " << ppp.GetID () << ", ttl " << uint16_t(ppp.GetTTL ())  );
    /*
    if (ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff"))
      {

        ProcessHeader (CopyPacket, protocol);
        m_rxCallback (this, CopyPacket, protocol, GetRemote ());
        NS_LOG_UNCOND (GetAddress () << " GES send broadcast packet to upper layer");
        //return;
      } //arp request
      */ //moved to below.

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) ) 
    {
      // 
      // If we have an error model and it indicates that it is time to lose a
      // corrupted packet, don't forward this packet up, let it go.
      //
      m_phyRxDropTrace (packet);
    }
  else 
    {
     if ( m_DevSameNode0Flag)
        {

            if (ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff"))
                {
                   ProcessHeader (CopyPacket, protocol);
                   m_rxCallback (this, CopyPacket, protocol, GetRemote ());
                   m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , protocol, m_dstLEOAddr);

                   NS_LOG_UNCOND (GetAddress () << " GES upload broadcast packet to upper layer");
                }
            else if (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress ()))
                {
                   ProcessHeader (CopyPacket, protocol);
                   m_rxCallback (this, CopyPacket, protocol, GetRemote ());
                   m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , protocol, m_dstLEOAddr);
                }
            m_DevSameNode0->ReceiveFromGES (packet);
            NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", receive GES data packet 2" );
        }
      else if  (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress() ) || ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff"))

      {
      // 
      // Hit the trace hooks.  All of these hooks are in the same place in this 
      // device because it is so simple, but this is not usually the case in
      // more complicated devices.
      //
      m_snifferTrace (packet);
      m_promiscSnifferTrace (packet);
      m_phyRxEndTrace (packet);

      //
      // Trace sinks will expect complete packets, not packets without some of the
      // headers.
      //
      Ptr<Packet> originalPacket = packet->Copy ();

      //
      // Strip off the point-to-point protocol header and forward this packet
      // up the protocol stack.  Since this is a simple point-to-point link,
      // there is no difference in what the promisc callback sees and what the
      // normal receive callback sees.
      //
      ProcessHeader (packet, protocol);


      if (!m_promiscCallback.IsNull ())
        {
          m_macPromiscRxTrace (originalPacket);
          m_promiscCallback (this, packet, protocol, GetRemote (), GetAddress (), NetDevice::PACKET_HOST);
        }

      m_macRxTrace (originalPacket);
      NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", receive GES data packet 2...." << protocol );
      m_rxCallback (this, packet, protocol, GetRemote ());
      m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , protocol, m_dstLEOAddr);
          NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", after.... " );
      }
    } 
  
  
    NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", after.... " );

}


void
PointToPointNetDeviceGES::ReceiveFromLEO (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  uint16_t protocol = 0;
  
  
    PppHeader ppp;
    packet->PeekHeader (ppp);
    
      Ptr<Packet> CopyPacket = packet->Copy ();
    
    if (ppp.GetSourceAddre () == Mac48Address::ConvertFrom (GetAddress() ))
      {
          NS_ASSERT ("GES drop packet due to loop" );
          return;
      }
      
    if (m_DevSameNode0Flag && ppp.GetSourceAddre () == Mac48Address::ConvertFrom (m_DevSameNode0->GetAddress () ))
      {
          NS_ASSERT ("GES drop packet due to loop" );
          return;
      }
  
  NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", receive GES data packet from " << ppp.GetSourceAddre () << ", size " <<packet->GetSize () );
  NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", receive GES data packet from " << ppp.GetSourceAddre () << ", size " <<packet->GetSize () << ", id " << ppp.GetID () << ", ttl " << uint16_t(ppp.GetTTL ())  );

  
    /*
    if (ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff"))
      {

        ProcessHeader (CopyPacket, protocol);
        m_rxCallback (this, CopyPacket, protocol, GetRemote ());
        NS_LOG_UNCOND (GetAddress () << " GES send broadcast packet to upper layer");
        //return;
      } //arp request
      */ //moved to below.

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) ) 
    {
      // 
      // If we have an error model and it indicates that it is time to lose a
      // corrupted packet, don't forward this packet up, let it go.
      //
      m_phyRxDropTrace (packet);
    }
  else 
    {
     if ( m_DevSameNode0Flag)
        {

            if (ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff"))
                {
                   ProcessHeader (CopyPacket, protocol);
                   //m_rxCallback (this, CopyPacket, protocol, GetRemote ());
                   m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , protocol, m_dstLEOAddr);
                   NS_LOG_UNCOND (GetAddress () << " GES upload broadcast packet to upper layer");
                }
            else if (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress ()))
                {
                   ProcessHeader (CopyPacket, protocol);
                   //m_rxCallback (this, CopyPacket, protocol, GetRemote ());
                   m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , protocol, m_dstLEOAddr);
                }
            //m_DevSameNode0->ReceiveFromGES (packet);
            Forward (packet);
            NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", receive GES data packet 2" );
        }
      else
      {
            NS_ASSERT (" incorrect device");
      }
    } 
}

Ptr<Queue>
PointToPointNetDeviceGES::GetQueue (void) const
{ 
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
PointToPointNetDeviceGES::NotifyLinkUp (void)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

void
PointToPointNetDeviceGES::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this);
  m_ifIndex = index;
}

uint32_t
PointToPointNetDeviceGES::GetIfIndex (void) const
{
  return m_ifIndex;
}

Ptr<Channel>
PointToPointNetDeviceGES::GetChannel (void) const
{
  return m_channel;
}

//
// This is a point-to-point device, so we really don't need any kind of address
// information.  However, the base class NetDevice wants us to define the
// methods to get and set the address.  Rather than be rude and assert, we let
// clients get and set the address, but simply ignore them.

void
PointToPointNetDeviceGES::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
}

Address
PointToPointNetDeviceGES::GetAddress (void) const
{
  return m_address;
}

bool
PointToPointNetDeviceGES::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}

void
PointToPointNetDeviceGES::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this);
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

//
// This is a point-to-point device, so every transmission is a broadcast to
// all of the devices on the network.
//
bool
PointToPointNetDeviceGES::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

//
// We don't really need any addressing information since this is a 
// point-to-point device.  The base class NetDevice wants us to return a
// broadcast address, so we make up something reasonable.
//
Address
PointToPointNetDeviceGES::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
PointToPointNetDeviceGES::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address
PointToPointNetDeviceGES::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("01:00:5e:00:00:00");
}

Address
PointToPointNetDeviceGES::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address ("33:33:00:00:00:00");
}

bool
PointToPointNetDeviceGES::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

bool
PointToPointNetDeviceGES::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool
PointToPointNetDeviceGES::Send (
  Ptr<Packet> packet, 
  const Address &dest, 
  uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
  NS_LOG_LOGIC ("UID is " << packet->GetUid ());


  //
  // If IsLinkUp() is false it means there is no channel to send any packet 
  // over so we just hit the drop trace on the packet and return an error.
  //
  if (IsLinkUp () == false)
    {
      m_macTxDropTrace (packet);
      NS_LOG_UNCOND ( ", IsLinkUp No !! ");
      return false;
    }
  
  
  
   m_destAddress = Mac48Address::ConvertFrom (dest);

  //
  // Stick a point to point protocol header on the packet in preparation for
  // shoving it out the door.
  //
  AddHeader (packet, protocolNumber);
  
  NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", send GES data packet to " << dest << ", size " << packet->GetSize() << ", protocolNumber " << protocolNumber << ", id " << m_packetId - 1);


  m_macTxTrace (packet);
  
  if (m_DevSameNode0Flag ) //packet generated by application, sent to both LEO and GES
    {
      Ptr<Packet> CopyPacket = packet->Copy ();
      m_DevSameNode0->ReceiveFromGES (CopyPacket);

    }

  //
  // We should enqueue and dequeue the packet to hit the tracing hooks.
  //
  if (m_queue->Enqueue (packet))
    {
      //
      // If the channel is ready for transition we send the packet right now
      // 
      if (m_txMachineState == READY)
        {
          NS_LOG_UNCOND (GetAddress () << ", m_txMachineState == READY -------------------------------------------- " );

          packet = m_queue->Dequeue ();
          m_snifferTrace (packet);
          m_promiscSnifferTrace (packet);
          return TransmitStart (packet, m_destAddress);
        }

      return true;
    }
  
  NS_LOG_UNCOND (GetAddress () << ", fail to queue -------------------------------------------- " );


  // Enqueue may fail (overflow)
  m_macTxDropTrace (packet);
  return false;
}

bool
PointToPointNetDeviceGES::SendFrom (Ptr<Packet> packet, 
                                 const Address &source, 
                                 const Address &dest, 
                                 uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);
  return false;
}

Ptr<Node>
PointToPointNetDeviceGES::GetNode (void) const
{
  return m_node;
}

void
PointToPointNetDeviceGES::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
  m_node = node;
}

bool
PointToPointNetDeviceGES::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

void
PointToPointNetDeviceGES::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  m_rxCallback = cb;
}

void
PointToPointNetDeviceGES::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  m_promiscCallback = cb;
}

bool
PointToPointNetDeviceGES::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
PointToPointNetDeviceGES::DoMpiReceive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  Receive (p);
}

Address 
PointToPointNetDeviceGES::GetRemote (void) const
{
  NS_LOG_FUNCTION (this);
  //NS_ASSERT (m_channel->GetNDevices () == 2); // GetRemote () is changed, this may affect rxcallback
  NS_LOG_UNCOND (GetAddress () << " GetRemote " << m_channel->GetNDevices ()); 
  
  Mac48Address dst;
  if ( m_DevSameNode0Flag) //attached to LEO
    {
      //input dst is not used, 24032018
      dst = m_dstGESAddr;
    }
  else //attached to GES
    { 
      dst = m_dstLEOAddr;
    }
  
  if (dst == Mac48Address ("00:00:00:00:00:00") ) //unreachable dst
    {
      NS_ASSERT ("do not have remote currently");
    }
  
  return dst;
  
   // below is not used ay more.  
  for (uint32_t i = 0; i < m_channel->GetNDevices (); ++i)
    {
      Ptr<NetDevice> tmp = m_channel->GetDevice (i);
      NS_LOG_UNCOND (i << " , " << tmp->GetAddress ()); 

      if (tmp != this)
        {
          return tmp->GetAddress ();
        }
    }
  NS_ASSERT (false);
  // quiet compiler.
  return Address ();
}

bool
PointToPointNetDeviceGES::SetMtu (uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}

uint16_t
PointToPointNetDeviceGES::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}

uint16_t
PointToPointNetDeviceGES::PppToEther (uint16_t proto)
{
  NS_LOG_FUNCTION_NOARGS();
  switch(proto)
    {
    case 0x0021: return 0x0800;   //IPv4
    case 0x0057: return 0x86DD;   //IPv6
    case 0x0806: return 0x0806;   //Arp   //invented number for arp
    default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
  return 0;
}

uint16_t
PointToPointNetDeviceGES::EtherToPpp (uint16_t proto)
{
  NS_LOG_FUNCTION_NOARGS();
  switch(proto)
    {
    case 0x0800: return 0x0021;   //IPv4
    case 0x86DD: return 0x0057;   //IPv6
    case 0x0806: return 0x0806;   //invented number for arp
    default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
  return 0;
}

void
PointToPointNetDeviceGES::AddDevice0 (Ptr<PointToPointNetDevice> dev)
{
  m_DevSameNode0 = dev;
  m_DevSameNode0Flag = true;
  NS_LOG_UNCOND (GetAddress () << " has LEODEVICE 0 " << m_DevSameNode0->GetAddress() );
}

void
PointToPointNetDeviceGES::AddDevice1 (Ptr<PointToPointNetDevice> dev)
{
  m_DevSameNode1 = dev;
  NS_LOG_UNCOND (GetAddress () << " has LEODEVICE 1 " << m_DevSameNode1->GetAddress() );
}


void 
PointToPointNetDeviceGES::Forward (Ptr<Packet> packet)
{
   if (IsLinkUp () == false)
    {
      NS_ASSERT ("there is no channel");  
      return;
    } 
   
  if (m_txMachineState == READY)
     {
       PppHeader ppp;
       packet->PeekHeader (ppp);
  
       TransmitStart (packet, ppp.GetDestAddre () );
     }   
}




void 
PointToPointNetDeviceGES::InitLinkDst (uint32_t position, uint32_t InitPosLES [], std::map<uint32_t, Mac48Address> DeviceMapPosition, uint32_t NumLEO, Time interval)
{

       m_indicator = position;
       //NS_LOG_UNCOND ("m_indicator " << m_indicator);
       //m_initPosLES = &InitPosLES; 
       m_deviceMapPosition = DeviceMapPosition;

       m_NumLEO = NumLEO;
       m_dstLEOinterval = interval;
       
       
       
        uint32_t sum = 0;
        for (uint32_t kk = 0; kk < m_NumLEO; kk++)
            {
                m_initPosLES[kk] = InitPosLES[kk];
                sum = sum + m_initPosLES[kk];
            }
         NS_ASSERT (sum = pow(2,m_NumLEO - 1));
         
         updateLinkDst ();
}
//position of mine
void 
PointToPointNetDeviceGES::updateLinkDst ( )
{
        /*    
       //input but already know     
        m_InitPosLES[0] = 0b00000000001; // initial position of the LES, they are same for all the GES
        m_InitPosLES[1] = 0b00000000010; // and apparently known (it is defined in this way), then the m_InitPos_dstGES follows.
        m_InitPosLES[2] = 0b00000000100;
        ...
        m_InitPosLES[10] = 0b10000000000;
    
        //input
        Indicator = 0b00000000001; // my position, sh
        */ 
    
         //NS_LOG_UNCOND (GetAddress () << "update link at " << Simulator::Now () << "  " <<  m_NumLEO);

    
       //fixed below
        uint32_t sum = 0;
        uint32_t leo = 0;
        uint32_t leoP = 0;
        for (uint32_t  kk = 0; kk < m_NumLEO; kk++)
            {
                //NS_LOG_UNCOND ("m_linkDst " << kk << ", " << m_initPosLES[kk] << ", " << m_indicator);
                m_linkDst[kk] = m_initPosLES[kk] & m_indicator;
                if (m_linkDst[kk] != 0)
                 {
                    leoP = leo; //get dst LEO
                 }
                leo++;
                //NS_LOG_UNCOND ("m_linkDst " << m_linkDst[kk] );
                sum = sum + m_linkDst[kk] ;
            }
        //check 
        uint32_t count = 0;
        while (sum)
          {
             count += sum & 1;
             sum = sum >> 1;
          }
        NS_ASSERT (count == 1);
        
        
      
        //use DeviceMapPosition to get device

        
        m_dstLEOAddr = m_deviceMapPosition.find (leoP)->second;
        NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () << " GES links to LEO "  << m_dstLEOAddr);
        // Circular
        // example, Assuming that CHAR_BITS == 8, x = ( x << 1U ) | ( x >> 7U ) ; would do a circular bit shift left by one.
        //m_indicator << 1;
        m_indicator = (m_indicator << 1) | (m_indicator >>  (m_NumLEO - 1) ); 
        GESupdateLinkDstEvent = Simulator::Schedule (m_dstLEOinterval, &PointToPointNetDeviceGES::updateLinkDst, this);
}




//attached to LEO satellite
void 
PointToPointNetDeviceGES::LEOInitLinkDst (uint32_t position, uint32_t InitPosGES [], std::map<uint32_t, Mac48Address> DeviceMapPosition, uint32_t NumGES, uint32_t NumLEO, Time interval)
{
       m_indicator = position;
       m_initPostion = position;
       //m_initPosLES = &InitPosLES; 
       m_deviceMapPosition = DeviceMapPosition;

       m_NumGES = NumGES;
       m_NumLEO = NumLEO;
       m_dstGESinterval = interval;
       
       
       
        uint32_t sum = 0;
        uint32_t count = 0;
        for (uint32_t kk = 0; kk < m_NumGES; kk++)
            {
                m_initPosGES[kk] = InitPosGES[kk];
                //NS_LOG_UNCOND ("position " << position << ", NumGES " << NumGES);
                //NS_LOG_UNCOND ("kk " << kk << ", m_initPosGES " << m_initPosGES[kk]);
                sum = sum + m_initPosGES[kk];
            }
        
        while (sum)
          {
             count += sum & 1;
             sum = sum >> 1;
          }
        NS_ASSERT (count == m_NumGES);
         
         LEOupdateLinkDst ();
}
//position of mine
void 
PointToPointNetDeviceGES::LEOupdateLinkDst ( )
{
       //fixed below
        uint32_t sum = 0;
        uint32_t ges = 0; //the ges can be linked to
        bool connected = false;
        for (uint32_t  kk = 0; kk < m_NumGES; kk++)
            {
                //NS_LOG_UNCOND (GetAddress () << "\t" << m_indicator  << " m_initPosGES[kk] "  << m_initPosGES[kk] << ", m_NumLEO " << m_NumLEO);
                m_linkDst[kk] = m_initPosGES[kk] & m_indicator;
                sum = sum + m_linkDst[kk] ;
            }
        
           for (uint32_t  kk = 0; kk < m_NumGES; kk++)
            {
                if (m_linkDst[kk] !=0 )
                 {
                    connected = true;
                    break;
                 }
                ges++;
            }
        
        //check 
        uint32_t count = 0;
        while (sum)
          {
             count += sum & 1;
             sum = sum >> 1;
          }
        NS_ASSERT (count == 1 || count == 0);
        
       
        if (connected)
        {
          m_dstGESAddr = m_deviceMapPosition.find (ges)->second;
          NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () << " LEO links to GES "  << m_dstGESAddr);
        }
        else
        {
          m_dstGESAddr = Mac48Address ("00:00:00:00:00:00"); //unreachable dst
        }
        
        // Circular
        // example, Assuming that CHAR_BITS == 8, x = ( x << 1U ) | ( x >> 7U ) ; would do a circular bit shift left by one.
        //m_indicator << 1;
        uint32_t indicator_a = m_indicator;
        uint32_t indicator_b = m_indicator;
        m_indicator = (indicator_a >> 1) | (indicator_b <<  (m_NumLEO - 1) );
        
        LEOupdateLinkDstEvent = Simulator::Schedule (m_dstGESinterval, &PointToPointNetDeviceGES::LEOupdateLinkDst, this);
}

} // namespace ns3
