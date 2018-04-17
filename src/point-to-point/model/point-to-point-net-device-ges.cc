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
                   //DataRateValue (DataRate ("1300000b/s")),
                   DataRateValue (DataRate ("100000000b/s")),
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
      .AddAttribute ("ARQAckTimeout", 
                   "The time interval for sending packets during channel selection",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&PointToPointNetDeviceGES::ARQAckTimer),
                   MakeTimeChecker ()) 
    .AddAttribute ("ARQBufferSize", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (31),
                   MakeUintegerAccessor (&PointToPointNetDeviceGES::BUFFERSIZE),
                   MakeUintegerChecker<uint32_t> ())

    //
    // Transmit queueing discipline for the device which includes its own set
    // of trace hooks.
    //
    .AddAttribute ("TxQueue", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDeviceGES::m_queue),
                   MakePointerChecker<Queue> ())
    .AddAttribute ("TxQueueQos3", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDeviceGES::m_queueQos3),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("TxQueueQos2", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDeviceGES::m_queueQos2),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("TxQueueQos1", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDeviceGES::m_queueQos1),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("TxQueueQos0", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDeviceGES::m_queueQos0),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("ForwardQueue4", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDeviceGES::m_queueForwardQos4),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("ForwardQueue3", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDeviceGES::m_queueForwardQos3),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("ForwardQueue2", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDeviceGES::m_queueForwardQos2),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("ForwardQueue1", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDeviceGES::m_queueForwardQos1),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("ForwardQueue0", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDeviceGES::m_queueForwardQos0),
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
                    "ns3::PointToPointNetDeviceGES::RecPacketCallback")
    .AddTraceSource ("ARQRecPacketTrace",
                    "Channel has been successfully Selected"
                    "The header of successfully transmitted packet",
                    MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_ARQrecPacketTrace),
                    "ns3::PointToPointNetDeviceGES::ARQRecPacketCallback")
    .AddTraceSource ("ARQTxPacketTrace",
                    "Channel has been successfully Selected"
                    "The header of successfully transmitted packet",
                    MakeTraceSourceAccessor (&PointToPointNetDeviceGES::m_ARQTxPacketTrace),
                    "ns3::PointToPointNetDeviceGES::ARQTxPacketCallback")
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
    ARQAckTimer (Seconds(10)),
    m_LEOcount (0),
    m_RemoteIdCount(0),  
    m_GESLEOAcqTime (MilliSeconds(500)),
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
  //NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", AddHeader , protocolNumber " << protocolNumber );

  NS_LOG_FUNCTION (this << p << protocolNumber);
  PppHeader ppp;
  ppp.SetProtocol (EtherToPpp (protocolNumber));
  ppp.SetType (m_type);
  ppp.SetSourceAddre (Mac48Address::ConvertFrom (GetAddress ()));
  ppp.SetDestAddre (m_destAddress);
  ppp.SetQos (m_Qos);
  ppp.SetTTL (0);  
  
  if (m_type == CHANNELACK || m_type == DATAACK || m_type == N_ACK) 
   {
        ppp.SetID (m_ackid);
   }
  else if (m_type == CHANNELRESP) 
   {
        ppp.SetID (m_respid);
        NS_FATAL_ERROR (" should not happen");
   }
  else if (m_type == CHANNELREQ) 
   {
        ppp.SetID (m_reqid);
        NS_FATAL_ERROR (" should not happen");
        m_packetId++;
   }
  else 
   {
      ppp.SetID (m_packetId);
      m_packetId++;
   }

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

void 
PointToPointNetDeviceGES::AcquisitionTimeLeft (void)
{   
    if (Simulator::Now () < m_GESLEOAcqStart + m_GESLEOAcqTime )
      {
        m_acquisition = true;
        NS_LOG_UNCOND ("AcquisitionTimeLeft");
        m_acquisitionTimeLeft = m_GESLEOAcqTime - (Simulator::Now () - m_GESLEOAcqStart );   
      }
    else
        m_acquisition = false;  
}


void 
PointToPointNetDeviceGES::TimeNextAcquisition (void)
{   
    Time GESHandOverInterval;
    if ( m_DevSameNode0Flag) //attached to LEO
      {
        GESHandOverInterval = m_dstGESinterval;
      }
   else //attached to GES
     { 
       GESHandOverInterval = m_dstLEOinterval;
     }
    
     Ptr<const Packet>  p = PeekqueueForward ();
     if (p == 0)
       {
         m_beforeNextAcquisition = false;
         return;
       }
         
     Time txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
     Time txCompleteTime = txTime + m_tInterframeGap;
     
     NS_LOG_UNCOND ("BeforeNextAcquisition");
     NS_LOG_UNCOND ("m_GESLEOAcqStart " << m_GESLEOAcqStart << " m_dstLEOinterval " << GESHandOverInterval << " now " << Simulator::Now ());
     m_timeNextAcquisition = m_GESLEOAcqStart + GESHandOverInterval -  Simulator::Now ();
     if (m_timeNextAcquisition < txCompleteTime)
         m_beforeNextAcquisition = true;
     else
         m_beforeNextAcquisition = false;
}


bool 
PointToPointNetDeviceGES::CancelTransmission (void)
{   
    AcquisitionTimeLeft ();
    TimeNextAcquisition ();
    if (m_acquisition)
        {
              Simulator::Schedule (m_acquisitionTimeLeft, &PointToPointNetDeviceGES::ForwardDown, this);
              return true;        
        }
    else if ( m_beforeNextAcquisition)
        {
            Simulator::Schedule (m_timeNextAcquisition, &PointToPointNetDeviceGES::ForwardDown, this);
            return true;
        }
    return false;
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
  //NS_LOG_UNCOND (GetAddress () << " send to another end " << dst);
  if ( m_DevSameNode0Flag) //attached to LEO
    {
      //input dst is not used, 24032018
      dst = m_dstGESAddr;
    }
  else //attached to GES
    { 
      dst = m_dstLEOAddr;
    }
  
    //NS_LOG_UNCOND (GetAddress () << " send to another end " << dst);
    PppHeader ppp;
    p->PeekHeader (ppp);
    //NS_LOG_UNCOND(Simulator::Now() << "\t" << GetAddress () << " send packet size  " << p->GetSize()  << "  dst " << ppp.GetDestAddre () <<  "  src " << ppp.GetSourceAddre ());
  
  if (!m_DevSameNode0Flag && dst == Mac48Address ("00:00:00:00:00:00") ) //unreachable dst
    {
      NS_FATAL_ERROR ("GES is not connected to any satellite");
    }
  
   if (ppp.GetSourceAddre() == Mac48Address::ConvertFrom (GetAddress() ) )
        NS_LOG_UNCOND("TX \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre()  << "\t" << ppp.GetDestAddre() << "\t"  << ppp.GetID () << "\t" << p->GetSize()  << "\t"  <<  Simulator::Now().GetMilliSeconds()  );
    else
        NS_LOG_UNCOND("FW \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre()  << "\t" << ppp.GetDestAddre() << "\t"  << ppp.GetID () << "\t" << p->GetSize()  << "\t"  <<  Simulator::Now().GetMilliSeconds()  );
       

    
   bool result;
  if ( dst == Mac48Address ("00:00:00:00:00:00") )//unreachable dst
    {
        result = false;
        NS_FATAL_ERROR  ("LEO " << GetAddress () << " is not connected to any GES");
    }
  else
    {
         PppHeader ppp;
         p->PeekHeader (ppp);
         //NS_LOG_UNCOND(Simulator::Now() << " tx time " << txCompleteTime );
         //NS_LOG_UNCOND(Simulator::Now() << "\t" << GetAddress () << " send packet size  " << p->GetSize()  << "  dst " << ppp.GetDestAddre () <<  "  src " << ppp.GetSourceAddre ());
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

  //Ptr<Packet> p = m_queue->Dequeue ();
  if ( CancelTransmission  () )
  {
        NS_LOG_UNCOND (Simulator::Now ().GetMilliSeconds () << "\t" << GetAddress () <<" cancel transmission due to Acquisition." );
        return;
  }
  

  Ptr<Packet>  p = DequeueForward ();
  if (p == 0)
    {
      //
      // No packet was on the queue, so we just exit.
      //
      NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () << " has no packet to sent");
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
PointToPointNetDeviceGES::ForwardDown (void)
{
   PppHeader ppp;
   /*
   if (m_loadBalance) 
       goto LinkedDevice;
   if (!m_channel->IsChannelUni ())
    {
      Ptr<const Packet>  packetPeek = PeekqueueForward ();
      if (packetPeek != 0)
       {
          packetPeek->PeekHeader (ppp);
          uint16_t nextHop = LookupRoutingTable (ppp.GetDestAddre());
          Mac48Address addr = Mac48Address::ConvertFrom(m_DevSameNode->GetAddress ());
          uint8_t mac[6];
          addr.CopyTo (mac);
          uint8_t aid_l = mac[5];
          uint8_t aid_h = mac[4] & 0x1f;
          uint16_t aid = (aid_h << 8) | (aid_l << 0); 
                     
          if (nextHop == aid - 1 )
            {
              Ptr<Packet> packetsend = DequeueForward ();
              m_DevSameNode->Forward (packetsend, PACKETREPEATNUMBER-1); 
              return true;
            }
          else
            {
              goto LinkedDevice;
            }
        }
  }
   
   */
      
 
     LinkedDevice:
      
      if ( CancelTransmission  () )
        {
         NS_LOG_UNCOND (Simulator::Now ().GetMilliSeconds () << "\t" << GetAddress () <<" cancel transmission due to Acquisition." );
         return false;
        } 

     
      if (m_txMachineState == READY)
        {
          Ptr<Packet>  packetsend =  DequeueForward ();
          if (packetsend == 0)
            {
              return false;
            }
          
          packetsend->PeekHeader (ppp);
          NS_LOG_DEBUG (GetAddress () <<" send m_Qos " << uint16_t(ppp.GetQos ()) << ", id " << ppp.GetID ());

          m_snifferTrace (packetsend);
          m_promiscSnifferTrace (packetsend);
          
          return TransmitStart (packetsend, ppp.GetDestAddre ());
        }
     return false; 
    
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
PointToPointNetDeviceGES::EnqueueForward (Ptr<Packet> packet)
{
   PppHeader ppp;
   packet->PeekHeader (ppp);  //enqueue to forward queue
   bool IsQueued = false;
   if ( ppp.GetType () == DATATYPE || ppp.GetType () == DATAACK || ppp.GetType () == N_ACK )
    {
       IsQueued = m_queueForward.find (ppp.GetQos ())->second->Enqueue (packet);
       //NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () <<" forwardqueue enqueue m_Qos " << uint16_t(ppp.GetQos ()) << ", original id " << ppp.GetID () << ", size " << packet->GetSize ());
    }
   else
    {
       IsQueued = m_queueForward[4]->Enqueue(packet);
    }
   
}

Ptr<const Packet>  
PointToPointNetDeviceGES::PeekqueueForward (void)
{
   Ptr<const Packet>  packet;
   for (uint8_t qos = 5; qos > 0; qos--)
     {
        packet = m_queueForward.find (qos-1)->second->Peek();
        if (packet != 0)
         {    
           PppHeader ppp;
           packet->PeekHeader (ppp); 
           if (qos < 5)
             {
              //NS_LOG_DEBUG ( "size " << m_queueForward.find (qos-1)->second->GetNPackets () );
              //NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () <<" forwardqueue dequeue m_Qos " << uint16_t(ppp.GetQos ()) << ", id " << ppp.GetID () << ", size " << packet->GetSize () << " src " << ppp.GetSourceAddre ());  
             }
           if (Simulator::Now().GetSeconds () > 14900)
             {
              //NS_LOG_DEBUG ( "size " << m_queueForward.find (qos-1)->second->GetNPackets () );
              //NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () <<" forwardqueue dequeue m_Qos " << uint16_t(ppp.GetQos ()) << ", id " << ppp.GetID () << ", size " << packet->GetSize () << " src " << ppp.GetSourceAddre ());  
             }
           return packet;
         }
     } 
   return 0;
}

Ptr<Packet>  
PointToPointNetDeviceGES::DequeueForward (void)
{
   Ptr<Packet>  packet;
   for (uint8_t qos = 5; qos > 0; qos--)
     {
        packet = m_queueForward.find (qos-1)->second->Dequeue ();
        if (packet != 0)
         {    
           PppHeader ppp;
           packet->PeekHeader (ppp); 
           if (qos < 5)
             {
              //NS_LOG_UNCOND ( "size " << m_queueForward.find (qos-1)->second->GetNPackets () );
              //NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () <<" forwardqueue dequeue m_Qos " << uint16_t(ppp.GetQos ()) << ", id " << ppp.GetID () << ", size " << packet->GetSize () << " src " << ppp.GetSourceAddre ());  
             }
           return packet;
         }
     } 
   return 0;
}


Ptr<Packet>
PointToPointNetDeviceGES::PreparePacketToSend ()
{
  Ptr<Packet> packet;  
  Ptr<Packet> packetSend;  
  
  packet = m_queue->Dequeue ();
  if (packet != 0)
  {
      return packet;
  }
  

  //add packets to the buffer if there is room
  Ptr<const Packet>  packetkPeek;
  bool queueEmpyt = true;
  for (uint8_t qos = 4; qos > 0; qos--)
   {
      packetkPeek = m_queueMap.find (qos-1)->second->Peek ();
      if (packetkPeek != 0)
       {    
            queueEmpyt = false;
            PppHeader ppp;

            packetkPeek->PeekHeader (ppp);

            if (ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff") || ppp.GetType () != DATATYPE)
            {
                return m_queueMap.find (qos-1)->second->Dequeue ();
            }

            RemoteSatelliteTx * Satellite = ARQLookupTx ( ppp.GetDestAddre (), ppp.GetID () );
            RemoteSatelliteARQBufferTx * buffer  = Satellite->m_buffer;
            
             //NS_LOG_UNCOND (GetAddress () << " buffer->m_bufferSize " << buffer->m_bufferSize <<  ", buffer->m_listPacket->size " << buffer->m_listPacket->size ());

            if (buffer->m_bufferSize == buffer->m_listPacket->size ()  )
             {
               //NS_LOG_UNCOND (GetAddress () <<  " ARQ is full, packets are not dequeued" << " buffer->m_bufferSize " << buffer->m_bufferSize << ", buffer->m_listPacket->size ()  " << buffer->m_listPacket->size () ); 
               packet = ARQSend (buffer);
             }
            else 
             {
               NS_ASSERT(buffer->m_bufferSize > buffer->m_listPacket->size ());
               packetSend = m_queueMap.find (qos-1)->second->Dequeue (); 
               //NS_LOG_UNCOND (GetAddress () << " ARQ is not full, packets are not dequeued"); 
              packet = ARQSend (buffer, packetSend);               
             }
            //NS_ASSERT (packet != 0); this could happen if all packet are sent but not receive ack
            if (packet !=0)
            {
            packet->PeekHeader (ppp);
            //NS_LOG_UNCOND (GetAddress () <<  " get ARQ packet id " <<  ppp.GetID () << ", size  " << packet->GetSize () << ", qos  " << ppp.GetQos () ); 
            }
            else
            {
               // NS_LOG_UNCOND (GetAddress () <<  " get ARQ packet 0 "  ); 

            }

            return  packet;
        }
    } //put packet to the buffer
           
  
  
  
  if (queueEmpyt)
   {
    //scan all the remote staellite
    for (StatDeviceTx::const_iterator i = m_statDeviceTx.begin (); i != m_statDeviceTx.end (); i++)
      {
        RemoteSatelliteARQBufferTx * buffer  = (*i)->m_buffer;
        packet = ARQSend (buffer);
        if (packet != 0)
        {
            return packet;
        }
     } 
   }
  
  return 0;
  
}

void
PointToPointNetDeviceGES::Receive (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  uint16_t protocol = 0;
  
  double correctrate = (float) random() / (RAND_MAX);
  if (correctrate < 1e-6 )
  {
      NS_LOG_UNCOND (GetAddress ()  << " drop packet " << correctrate);
      return; 
  }
  
  
    PppHeader ppp;
    packet->PeekHeader (ppp);
    uint16_t ttl = ppp.GetTTL ();
    NS_LOG_UNCOND("RX \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre()  << "\t" << ppp.GetDestAddre() << "\t"  << ppp.GetID () << "\t" << packet->GetSize()  << "\t"  <<  Simulator::Now().GetMilliSeconds()  );
    if (ttl > TTLmax)
      {
          NS_LOG_DEBUG ("PACKET DROPPED DUE TO TTL ");
          return;
      }
      else
      {
          PppHeader newppp;
          packet->RemoveHeader (newppp);
          //NS_LOG_DEBUG ("set TTL " << ttl + 1);
          newppp.SetTTL (ttl + 1);
          packet->AddHeader (newppp);
      }
    
      Ptr<Packet> CopyPacket = packet->Copy ();
      ProcessHeader (packet, protocol);
    
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
     //NS_LOG_UNCOND("RX \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre()  << "\t" << ppp.GetDestAddre() << "\t"  << ppp.GetID () << "\t" << packet->GetSize()  << "\t"  <<  Simulator::Now().GetMilliSeconds()  ); 
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
                   NS_FATAL_ERROR ("ges should not get packets"); 
                   ProcessHeader (CopyPacket, protocol);
                   m_rxCallback (this, CopyPacket, protocol, GetRemote ());
                   m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , protocol, m_dstLEOAddr);
                }     
            //NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", receive GES data packet 2, size " << packet->GetSize ());
            m_DevSameNode0->ReceiveFromGES (CopyPacket);
            //NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", receive GES data packet 2" );
        }
      else if (ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff"))
        {
          Ptr<Packet> ARQRxPacket = packet->Copy ();
          //m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , ppp.GetID (), m_dstLEOAddr);
          m_rxCallback (this, packet, protocol, GetRemote ());
        }
      else if  (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress() ) )
      {
        //NS_LOG_UNCOND("RX \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre()  << "\t" << ppp.GetDestAddre() << "\t"  << ppp.GetID () << "\t" << packet->GetSize()  << "\t"  <<  Simulator::Now().GetMilliSeconds()  );
        if (ppp.GetType () == DATATYPE )
           {
                Ptr<Packet> ARQRxPacket = packet->Copy ();
                //NS_LOG_UNCOND(Simulator::Now() << "\t"  << GetAddress () << " receives packet from "  <<ppp.GetSourceAddre() << " to " << ppp.GetDestAddre ()  << ", size "  <<  packet->GetSize() << ",  protocol " << protocol );
                m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , ppp.GetID (), m_dstLEOAddr);
                ARQReceive (ppp.GetSourceAddre(), ppp.GetDestAddre(), Simulator::Now (), packet->GetSize() ,  ppp.GetQos (), ppp.GetID (), ARQRxPacket);
                m_rxCallback (this, packet, protocol, GetRemote ());
            }
        else if (ppp.GetType () == DATAACK && ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress() ) )
           {
             ARQACKRecevie (ppp.GetSourceAddre(), ppp.GetID ());  
           }
        else if ( ppp.GetType () == N_ACK && ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress() ))
           {
             ARQN_ACKRecevie (ppp.GetSourceAddre(), ppp.GetID () );   
           }
      }
    } 
}


void
PointToPointNetDeviceGES::ReceiveFromLEO (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  uint16_t protocol = 0;
  
    NS_FATAL_ERROR  ("ReceiveFromLEO ");
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
    
    NS_LOG_UNCOND("RX \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre()  << "\t" << ppp.GetDestAddre() << "\t"  << ppp.GetID () << "\t" << packet->GetSize()  << "\t"  <<  Simulator::Now().GetMilliSeconds()  );

  
  //NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", receive GES data packet from " << ppp.GetSourceAddre () << ", size " <<packet->GetSize () << ", id " << ppp.GetID () << ", ttl " << uint16_t(ppp.GetTTL ())  );

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
                   //ProcessHeader (CopyPacket, protocol);
                   //m_rxCallback (this, CopyPacket, protocol, GetRemote ());
                   //m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , ppp.GetID(), m_dstLEOAddr);
                   //NS_LOG_UNCOND (GetAddress () << " GES upload broadcast packet to upper layer");
                }
            else if (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress ()))
                {
                   NS_FATAL_ERROR ("ges should not get packets"); 
                   ProcessHeader (CopyPacket, protocol);
                   //m_rxCallback (this, CopyPacket, protocol, GetRemote ());
                   m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , ppp.GetID(), m_dstLEOAddr);
                }
            //m_DevSameNode0->ReceiveFromGES (packet);
            Forward (packet);
            //NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", receive GES data packet 2" );
        }
      else
      {
            NS_FATAL_ERROR (" incorrect device");
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
  uint16_t protocolNumberARQtemp)
{
   uint8_t QosTempARQ = protocolNumberARQtemp -2048;
    uint16_t protocolNumber = 2048;

      
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
  m_type =  DATATYPE;
  m_Qos = QosTempARQ;
  AddHeader (packet, protocolNumber);
  
  //NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", send GES data packet to " << dest << ", size " << packet->GetSize() << ", protocolNumber " << protocolNumber << ", id " << m_packetId - 1);


  m_macTxTrace (packet);
  
  if (m_DevSameNode0Flag ) //packet generated by application, sent to both LEO and GES
    {
      NS_FATAL_ERROR ("ges should not send packets"); 
    }

  
   PppHeader ppp;
   packet->PeekHeader (ppp);
    
  m_Qos = ppp.GetQos ();
  // NS_LOG_UNCOND ("send data m_Qos " << uint16_t(m_Qos));
  m_queueMap.find (m_Qos)->second->Enqueue (packet); //uppper layer queue
  NS_LOG_UNCOND(Simulator::Now() << "\t" << GetAddress () << " src  " << ppp.GetSourceAddre ()  << " dst  " << ppp.GetDestAddre () );
  NS_LOG_UNCOND(Simulator::Now() << "\t" << GetAddress () << " send packet size  " << packet->GetSize()  << " original id " << ppp.GetID () <<  "  qos " << uint16_t(m_Qos));
  Ptr<Packet> packetARQ;
  packetARQ = PreparePacketToSend (); //enqueue packets to ARQ and dequeue packet from ARQ.
  if (packetARQ == 0)
  {
      NS_LOG_DEBUG (GetAddress () <<" no packet from arq " );
      return false;
  }
  
  packetARQ->PeekHeader (ppp);
  Ptr<Packet> PacketForwardtest = packetARQ->Copy ();
  EnqueueForward (PacketForwardtest);
  
  
  return ForwardDown ();
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
  //NS_LOG_UNCOND (GetAddress () << " GetRemote " << m_channel->GetNDevices ()); 
  
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
      //NS_LOG_UNCOND (i << " , " << tmp->GetAddress ()); 

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
  //NS_LOG_UNCOND (Simulator::Now () << "\t" <<GetAddress () <<" PointToPointNetDeviceGES::Forward packet");
  EnqueueForward (packet);
  ForwardDown ();  
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

        m_GESLEOAcqStart = Simulator::Now ();
        m_dstLEOAddr = m_deviceMapPosition.find (leoP)->second;
        NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () << " GES links to LEO "  << m_dstLEOAddr << " m_dstLEOinterval " << m_dstLEOinterval);
        // Circular
        // example, Assuming that CHAR_BITS == 8, x = ( x << 1U ) | ( x >> 7U ) ; would do a circular bit shift left by one.
        //m_indicator << 1;
        m_indicator = (m_indicator << 1) | (m_indicator >>  (m_NumLEO - 1) ); 
        GESupdateLinkDstEvent = Simulator::Schedule (m_dstLEOinterval, &PointToPointNetDeviceGES::updateLinkDst, this);
}

/*
void 
PointToPointNetDeviceGESAll::LEOInitLinkDstAll (Mac48Address addr, uint32_t position, uint32_t InitPosGES [], std::map<uint32_t, Mac48Address> DeviceMapPosition, uint32_t NumGES, uint32_t NumLEO, Time interval)
{
      m_LEOAddrMap[m_LEOcount] = addr;
     
       
       m_LEOindicator[m_LEOcount] = position;
       m_LEOinitPostion[m_LEOcount] = position;
       m_deviceMapPosition = DeviceMapPosition;

       m_NumGES = NumGES;
       m_NumLEO = NumLEO;
       m_dstGESinterval = interval;
       
        m_LEOcount++;
       
       
       
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
        
        if (m_LEOcount == NumLEO)
        {
           LEOupdateLinkDstAll ();
        }
         
}

void 
PointToPointNetDeviceGES::LEOupdateLinkDstAll ( )
{
    for (uint32_t ii = 0; ii < NumLEO; ii++) 
    {
        //fixed below
        uint32_t sum = 0;
        uint32_t ges = 0; //the ges can be linked to
        bool connected = false;
        for (uint32_t  kk = 0; kk < m_NumGES; kk++)
            {
                //NS_LOG_UNCOND (GetAddress () << "\t" << m_indicator  << " m_initPosGES[kk] "  << m_initPosGES[kk] << ", m_NumLEO " << m_NumLEO);
                m_LEOlinkDst[ii][kk] = m_initPosGES[kk] & m_LEOindicator[ii];
                sum = sum + m_LEOlinkDst[ii][kk] ;
            }
        
           for (uint32_t  kk = 0; kk < m_NumGES; kk++)
            {
                if (m_linkDst[ii][kk] !=0 )
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
          m_LEOdstGESAddr = m_deviceMapPosition.find (ges)->second;
          NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () << " LEO links to GES "  << m_LEOdstGESAddr);
        }
        else
        {
          m_LEOdstGESAddr = Mac48Address ("00:00:00:00:00:00"); //unreachable dst
        }
        
        m_LEORouting[m_LEOAddrMap.find(ii)->second] = m_LEOdstGESAddr;
        // Circular
        // example, Assuming that CHAR_BITS == 8, x = ( x << 1U ) | ( x >> 7U ) ; would do a circular bit shift left by one.
        //m_indicator << 1;
        uint32_t indicator_a = m_LEOindicator[ii];
        uint32_t indicator_b = m_LEOindicator[ii];
        m_LEOindicator[ii] = (indicator_a >> 1) | (indicator_b <<  (m_NumLEO - 1) );
    }
        LEOupdateLinkDstEvent = Simulator::Schedule (m_dstGESinterval, &PointToPointNetDeviceGES::LEOupdateLinkDst, this);
}
*/

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
          m_GESLEOAcqStart = Simulator::Now ();
          m_dstGESAddr = m_deviceMapPosition.find (ges)->second;
          NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () << " LEO links to GES "  << m_dstGESAddr);
        }
        else
        {
          m_dstGESAddr = Mac48Address ("00:00:00:00:00:00"); //unreachable dst
        }
        NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () << " m_dstGESinterval "  << m_dstGESinterval);
        // Circular
        // example, Assuming that CHAR_BITS == 8, x = ( x << 1U ) | ( x >> 7U ) ; would do a circular bit shift left by one.
        //m_indicator << 1;
        uint32_t indicator_a = m_indicator;
        uint32_t indicator_b = m_indicator;
        m_indicator = (indicator_a >> 1) | (indicator_b <<  (m_NumLEO - 1) );
        
        LEOupdateLinkDstEvent = Simulator::Schedule (m_dstGESinterval, &PointToPointNetDeviceGES::LEOupdateLinkDst, this);
}



void
PointToPointNetDeviceGES::SetQueue (Ptr<Queue> q, Ptr<Queue> queueCritical, Ptr<Queue> queuehighPri, Ptr<Queue> queueBestEff, Ptr<Queue> queueBackGround, 
        Ptr<Queue> queueForward, Ptr<Queue> queueCriticalForward, Ptr<Queue> queuehighPriForward, Ptr<Queue> queueBestEffForward, Ptr<Queue> queueBackGroundForward)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
  
  //Ptr<Queue> queueCritical = m_queueFactory.Create<Queue> ();
  //Ptr<Queue> queuehighPri = m_queueFactory.Create<Queue> ();
  //Ptr<Queue> queueBestEff = m_queueFactory.Create<Queue> ();
  //Ptr<Queue> queueBackGround = m_queueFactory.Create<Queue> ();  
  /*
  Ptr<Queue> queueCritical;
  Ptr<Queue> queuehighPri;
  Ptr<Queue> queueBestEff;
  Ptr<Queue> queueBackGround; */
  m_queueQos3 =   queueCritical; 
  m_queueQos2 =   queuehighPri; 
  m_queueQos1 =   queueBestEff; 
  m_queueQos0 =   queueBackGround; 
  
  m_queueMap[3] = queueCritical;
  m_queueMap[2] = queuehighPri;
  m_queueMap[1] = queueBestEff;
  m_queueMap[0] = queueBackGround; 
  
  
  m_queueForwardQos4 = queueForward;
  m_queueForwardQos3 = queueCriticalForward;
  m_queueForwardQos2 = queuehighPriForward;
  m_queueForwardQos1 = queueBestEffForward;
  m_queueForwardQos0 = queueBackGroundForward;

  m_queueForward[4] = queueForward;
  m_queueForward[3] = queueCriticalForward;
  m_queueForward[2] = queuehighPriForward;
  m_queueForward[1] = queueBestEffForward;
  m_queueForward[0] = queueBackGroundForward;
   

}



void
PointToPointNetDeviceGES::CheckAvailablePacket (RemoteSatelliteARQBuffer * buffer, Mac48Address src)
{
    NS_ASSERT (buffer->m_listPacket->size () < buffer->m_bufferSize);
    ARQRxBufferCheck (buffer);
    

    
    std::list<uint32_t >::const_iterator i = buffer->m_listPacketId->begin ();
    
    if (buffer->m_listPacketId->size () == 0)
     {
        return;
     }
    

    if ( buffer->m_bufferStart == (*i) )
    {
       Ptr<Packet> packet =  buffer->m_listPacket->front ();
       Time rxtime =  buffer->m_listRecTime->front ();
       uint32_t size  =   buffer->m_listSize->front ();
       uint32_t protocol = buffer->m_listProtocol->front ();
       uint32_t packetid = buffer->m_listPacketId->front ();
       //deliver to upper layer 
        m_ARQrecPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), src , packet, rxtime, size, protocol,  packetid);
        uint32_t id = buffer->m_listPacketId->front ();
        buffer->m_listPacket->pop_front ();
        buffer->m_listRecTime->pop_front ();
        buffer->m_listSize->pop_front ();
        buffer->m_listProtocol->pop_front ();
        buffer->m_listPacketId->pop_front ();
        
        buffer->m_bufferStart++;
        buffer->m_NACKNumStart = 0;
        
        //NS_LOG_DEBUG (GetAddress () << " pop_front rx packet upload to upper layer, with id " << id << ", buffer->m_bufferStart " << buffer->m_bufferStart);


        Simulator::ScheduleNow (&PointToPointNetDeviceGES::CheckAvailablePacket, this, buffer, src);
    }
    else 
    {
        NS_ASSERT ( buffer->m_bufferStart < (*i) );
    }
    
}


void
PointToPointNetDeviceGES::ARQReceive (Mac48Address src, Mac48Address dst, Time rxtime, uint32_t size , uint32_t protocol, uint32_t packetid, Ptr<Packet> packet)
{  
    //protocol is qos acuralllyl, to do
    NS_ASSERT (Mac48Address::ConvertFrom(GetAddress ()) == dst);
    RemoteSatellite * Satellite = ARQLookup ( src,  rxtime,  size ,  protocol,  packetid);
    RemoteSatelliteARQBuffer * buffer = Satellite->m_buffer;
    
    ARQRxBufferCheck (buffer);
    
    NS_ASSERT (buffer->m_listPacket->size () < buffer->m_bufferSize);
            
    std::list<Time >::const_iterator RxTimeiterator = buffer->m_listRecTime->begin ();
    std::list<uint32_t >::const_iterator Sizeiterator = buffer->m_listSize->begin ();
    std::list<uint32_t >::const_iterator Protocoliterator = buffer->m_listProtocol->begin ();
    std::list<Ptr<Packet> >::const_iterator Packetiterator = buffer->m_listPacket->begin ();
    //std::list<uint32_t >::const_iterator NAackNumiterator = buffer->m_listNAackNum->begin (); //NACKNUM2903

  
    bool packetBuffered = false;   
    uint32_t start = buffer->m_bufferStart;
    
     //NS_LOG_UNCOND(GetAddress () << " ARQReceive id " << packetid << ", buffer start " << start << " for " << src);

    if (packetid < start)
      {
         // drop the packet
        //send ack 
        //SendAck (src, 2048,  packetid, DATAACK, protocol); // //protocol is qos acuralllyl, to do
        return;
      }
    else if (packetid > start  && packetid < start + buffer->m_bufferSize)
      {
        //send ack 
        //SendAck (src, 2048,  packetid, DATAACK, protocol);
        goto bufferpacket; //buffer the packet 
      } 
    else if (packetid > start + buffer->m_bufferSize || packetid == start + buffer->m_bufferSize)
     {
        //drop
        goto preTopacketid;
     }
   else if (packetid == start)
      {
        //deliver to upper layer
        m_ARQrecPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), src , packet, rxtime, size, protocol,  packetid);
        //SendAck (src, 2048,  packetid, DATAACK, protocol);
        //start++;
        buffer->m_bufferStart++; 
        buffer->m_NACKNumStart = 0;

        //check the next packet
        SendAck (src, 2048,  buffer->m_bufferStart, DATAACK, protocol);
        CheckAvailablePacket (buffer, src);
        return;
      }
  
   
    //NS_LOG_DEBUG (GetAddress () << "2517 PointToPointNetDevice::ARQReceive " <<  src << "\t" <<  dst  << "\t" <<  rxtime  << "\t" << size  << "\t" << protocol  << "\t" <<  packetid);


  bufferpacket:
    
    if ( buffer->m_listPacket->empty())
      {
        buffer->m_listPacket->push_back (packet);
        buffer->m_listRecTime->push_back (rxtime);
        buffer->m_listSize->push_back (size);
        buffer->m_listProtocol->push_back (protocol);
        buffer->m_listPacketId->push_back (packetid);
        //buffer->m_listNAackNum->push_back (0);
        packetBuffered = true;
        //NS_LOG_DEBUG(GetAddress () << " ARQReceive push back id " << packetid);
      }
    else
     {        
        for (std::list<uint32_t >::const_iterator i = buffer->m_listPacketId->begin (); i != buffer->m_listPacketId->end (); i++)
         {
           //  (packetid > start  || packetid < start + buffer->m_bufferSize)
           if (packetid == (*i))
             {
               packetBuffered = true;
               break;
             }
           if (packetid <(*i) )
              {
                buffer->m_listPacketId->insert (i,packetid);  
                buffer->m_listRecTime->insert (RxTimeiterator,rxtime);
                buffer->m_listSize->insert (Sizeiterator,size);
                buffer->m_listProtocol->insert (Protocoliterator,protocol);
                buffer->m_listPacket->insert (Packetiterator,packet);
                    
                packetBuffered = true;
                //NS_LOG_DEBUG (GetAddress () << " ARQReceive push back(insert) id " << packetid);

                break;
              }
            RxTimeiterator++;
            Sizeiterator++;
            Protocoliterator++;
            Packetiterator++;
            //NAackNumiterator++;
         }
     }
   
  if (! packetBuffered)
   {
     buffer->m_listPacket->push_back (packet);
     buffer->m_listRecTime->push_back (rxtime);
     buffer->m_listSize->push_back (size);
     buffer->m_listProtocol->push_back (protocol);
     buffer->m_listPacketId->push_back (packetid);
     //NS_LOG_DEBUG (GetAddress () << " ARQReceive push back id " << packetid);

     //buffer->m_listNAackNum->push_back (0);
   }
  
  
  
 preTopacketid: 
   buffer->m_NACKNumStart++; // start packet is sent
  if (buffer->m_NACKNumStart > NACKNUMSTART)
   {
      NS_LOG_DEBUG (GetAddress () << " exceeds N_ACK number for " << src);
      buffer->m_NACKNumStart = 0;
      buffer->m_bufferStart++;
      CheckAvailablePacket (buffer, src);
   }
   start = buffer->m_bufferStart; 
   //SendAck (src, 2048,  start, N_ACK, protocol);
   SendAck (src, 2048,  start, DATAACK, protocol);
   
   
    //std::list<uint32_t >::const_iterator NAackNumiterator = buffer->m_listNAackNum->begin (); //NACKNUM2903
   
   /*
   for (uint32_t M = start; M <= start; M++)
    {
       bool packetReceived = false;
        //If M is marked as not received: send a NACK with ID = M
        //check all the packet [M, packetid), if not received, send NACK with M
         for (std::list<uint32_t >::const_iterator i = buffer->m_listPacketId->begin (); i != buffer->m_listPacketId->end (); i++)
            // (uint32_t M = start; M < packetid; M++)   
            {
               if (M  ==(*i) )
                 {
                   packetReceived = true;
                   break;
                 } 
            }
       if (!packetReceived)
          {
            SendAck (src, 2048,  M, N_ACK, protocol);
          }
           //send NACK with M
    }
   */
  
}

RemoteSatellite * 
PointToPointNetDeviceGES::ARQLookup (Mac48Address src, Time rece, uint32_t size , uint32_t protocol, uint32_t packetid)  const
{ 
   for (StatDevice::const_iterator i = m_statDevice.begin (); i != m_statDevice.end (); i++)
    {
      if ((*i)->m_remoteAddress == src)
        {
          return (*i);
        }
    } 
   
  // create state RemoteSatelliteARQBuffer
  RemoteSatelliteARQBuffer * buffer = new RemoteSatelliteARQBuffer ();
  
  std::list<Ptr<Packet> > * listPacket = new std::list<Ptr<Packet> >;
  std::list<Time > * listRecTime = new std::list<Time >;
  std::list<uint32_t > * listSize = new std::list<uint32_t >;
  std::list<uint32_t > * listProtocol = new std::list<uint32_t >;
  std::list<uint32_t > * listPacketId = new std::list<uint32_t >;
  std::list<uint32_t > * listNAackNum = new std::list<uint32_t >; //NACKNUM2903


  buffer->m_listPacket = listPacket;
  buffer->m_listRecTime = listRecTime;
  buffer->m_listSize = listSize;
  buffer->m_listProtocol = listProtocol; 
  buffer->m_listPacketId = listPacketId; 
  //buffer->m_listNAackNum = listNAackNum; //NACKNUM2903

  
  buffer->m_bufferStart = 0;
  buffer->m_bufferSize = BUFFERSIZE;
  buffer->m_NACKNumStart = 0;

  
  //create new RemoteSatellite
  RemoteSatellite *Satellite = new RemoteSatellite ();
  Satellite->m_buffer = buffer;
  Satellite->m_retryCount = ARQCOUNT;   //not needed
  Satellite->m_remoteAddress = src;
  
  const_cast<PointToPointNetDeviceGES *> (this)->m_statDevice.push_back (Satellite);
  
  ARQRxBufferCheck (buffer);
  
  return Satellite;
}

            
            



bool
PointToPointNetDeviceGES::ARQRxBufferCheck (RemoteSatelliteARQBuffer * buffer) const
{
    uint32_t size = buffer->m_listPacket->size ();
    NS_ASSERT (buffer->m_listRecTime->size () == size);
    NS_ASSERT (buffer->m_listSize->size () == size);
    NS_ASSERT (buffer->m_listProtocol->size () == size);
    NS_ASSERT (buffer->m_listPacketId->size () == size);
    return true;
}


bool
PointToPointNetDeviceGES::ARQTxBufferCheck (RemoteSatelliteARQBufferTx * buffer) const
{
    uint32_t size = buffer->m_listPacket->size ();
    NS_ASSERT (buffer->m_listTxTime->size () == size);
    NS_ASSERT (buffer->m_listPacketId->size () == size);
    NS_ASSERT (buffer->m_listPacketStatus->size () == size);
    NS_ASSERT (buffer->m_listPacketRetryNum->size () == size);
    return true;
}


void
PointToPointNetDeviceGES::ARQAckTimeout (RemoteSatelliteARQBufferTx * buffer)
{
    ARQTxBufferCheck (buffer);
    uint32_t packetidstart = buffer->m_listPacketId->front ();
    //NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress ()  << ", packetidstart " << packetidstart  );
    bool packetInbuffer = false;
    uint32_t txTimeUs;
    uint32_t currentTimeUs;
    bool IsPacketTimeOut;
   // std::list<uint32_t >::const_iterator PacketStatusiterator = buffer->m_listPacketStatus->begin ();
   // std::list<uint32_t >::const_iterator PacketRetryNumiterator = buffer->m_listPacketRetryNum->begin ();

    std::list<uint32_t >::const_iterator PacketStatusiterator = buffer->m_listPacketStatus->begin ();
    std::list<uint32_t >::const_iterator PacketRetryNumiterator = buffer->m_listPacketRetryNum->begin ();
    std::list<Time >::const_iterator TxTimeiterator = buffer->m_listTxTime->begin ();

    uint32_t CheckbufferStart_ACK = buffer->m_listPacketId->front ();
   for (uint32_t packetid = packetidstart; packetid <= buffer->m_listPacketId->back (); packetid++)
   {
      std::list<uint32_t >::const_iterator PacketStatusiterator = buffer->m_listPacketStatus->begin ();
      std::list<uint32_t >::const_iterator PacketRetryNumiterator = buffer->m_listPacketRetryNum->begin ();
      std::list<Time >::const_iterator TxTimeiterator = buffer->m_listTxTime->begin ();
      uint32_t CheckbufferStart_ACK = buffer->m_listPacketId->front ();
      
    for (std::list<uint32_t >::const_iterator it = buffer->m_listPacketId->begin (); it != buffer->m_listPacketId->end (); it++)
     {
        
         if ( (*it) == CheckbufferStart_ACK &&  (*PacketStatusiterator) == ACK_REC ) //for packets with REC status
          {
             NS_FATAL_ERROR  ("should not happen");
            //NS_ASSERT ( packetid == buffer->m_bufferStart); // assume this packet should be at front of the window. to do
            //NS_ASSERT ( it == buffer->m_listPacketId->begin ()); // assume this packet should be at front of the window. to do
            uint32_t id = buffer->m_listPacketId->front ();
            Ptr<Packet> packettrace = buffer->m_listPacket->front ();

            Ptr<Packet> Copypackettrace = packettrace->Copy ();
            buffer->m_listPacket->pop_front ();
            buffer->m_listTxTime->pop_front ();
            buffer->m_listPacketId->pop_front ( );
            buffer->m_listPacketStatus->pop_front ();
            uint32_t retrynum = buffer->m_listPacketRetryNum->front ();
            buffer->m_listPacketRetryNum->pop_front ();
            buffer->m_bufferStart++;
            //packetInbuffer = true;
            
            PppHeader ppp;
            Copypackettrace->PeekHeader (ppp);
            NS_LOG_DEBUG (GetAddress () << " pop_front tx packet dropped due to previous ACK_REC , with id " << id << ", transmission time " <<  retrynum << ", " << ppp.GetDestAddre ());
            Ptr<Packet> packetSend = PreparePacketToSend ();
            if (packetSend == 0)
                {
                    NS_LOG_DEBUG (GetAddress () << " Timeout, no packet got from ARQ "  );
                    return;
                }
            EnqueueForward (packetSend);
         
            m_ARQTxPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetDestAddre (), id, ppp.GetQos (), retrynum, false );
            break;
          } 
        
         txTimeUs = (*TxTimeiterator).GetMicroSeconds ();
         currentTimeUs = Simulator::Now().GetMicroSeconds ();
         if (currentTimeUs - txTimeUs > ARQAckTimer.GetMicroSeconds ())
           {
             IsPacketTimeOut = true;
           }
        
         
         if (  (*it) == packetid && (*PacketStatusiterator) == TRANSMITTED && (*PacketRetryNumiterator) == ARQCOUNT && IsPacketTimeOut ) 
          {
            //NS_ASSERT ( packetid == buffer->m_bufferStart); // assume this packet should be at front of the window. 
            //NS_ASSERT ( it == buffer->m_listPacketId->begin ()); // assume this packet should be at front of the window. 
            uint32_t id = buffer->m_listPacketId->front ();
            Ptr<Packet> packettrace = buffer->m_listPacket->front ();

            Ptr<Packet> Copypackettrace = packettrace->Copy ();
            buffer->m_listPacket->pop_front ();
            buffer->m_listTxTime->pop_front ();
            buffer->m_listPacketId->pop_front ( );
            buffer->m_listPacketStatus->pop_front ();
            uint32_t retrynum = buffer->m_listPacketRetryNum->front ();
            buffer->m_listPacketRetryNum->pop_front ();
            buffer->m_bufferStart++;
            //packetInbuffer = true;
            
            PppHeader ppp;
            Copypackettrace->PeekHeader (ppp);
            
            NS_LOG_DEBUG (GetAddress () << " pop_front tx packet dropped due to acktimeout, with id " << id << ", transmission time " <<  retrynum << ", " << ppp.GetDestAddre ());
            m_ARQTxPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetDestAddre (), id, ppp.GetQos (), retrynum, false );
            Ptr<Packet> packetSend = PreparePacketToSend ();
           if (packetSend == 0)
             {
               NS_LOG_DEBUG (GetAddress () << "  no packet got from ARQ "  );
               return;
             }
            EnqueueForward (packetSend);
            NS_LOG_DEBUG (GetAddress () << "  enqueue packets to forwarding  table "  );
            break;
          }
        else if (  (*it) == packetid && (*PacketStatusiterator) == TRANSMITTED && (*PacketRetryNumiterator) > 0 && IsPacketTimeOut )
          {
            uint32_t retrynum = buffer->m_listPacketRetryNum->front ();
             //NS_LOG_DEBUG (GetAddress () << " not pop_front tx packet dropped due to acktimeout, with id " << packetid << ", transmission time " <<  retrynum );

            //NS_ASSERT ( (*PacketStatusiterator) == TRANSMITTED);
            PacketStatusiterator = buffer->m_listPacketStatus->erase (PacketStatusiterator);
            buffer->m_listPacketStatus->insert (PacketStatusiterator, NO_TRANSMITTED);
            //packetInbuffer = true; 
            //ARQSend (buffer);
            break;
          }
        PacketStatusiterator++;
        PacketRetryNumiterator++;
        TxTimeiterator++;
     }
   }
    /*
      if (!m_channel->IsChannelUni ())
    {
      PppHeader ppp; 
      Ptr<const Packet>  packetPeek = PeekqueueForward ();
      if (packetPeek != 0)
       {
          packetPeek->PeekHeader (ppp);
          uint16_t nextHop = LookupRoutingTable (ppp.GetDestAddre());
          Mac48Address addr = Mac48Address::ConvertFrom(m_DevSameNode->GetAddress ());
          uint8_t mac[6];
          addr.CopyTo (mac);
          uint8_t aid_l = mac[5];
          uint8_t aid_h = mac[4] & 0x1f;
          uint16_t aid = (aid_h << 8) | (aid_l << 0); 
          
          if (nextHop == aid - 1 )
            {
              //Ptr<Packet> packetforward = DequeueForward ();
              //m_DevSameNode->Forward (packetforward, PACKETREPEATNUMBER-1); 
              goto LinkedDevice;
              NS_LOG_DEBUG (GetAddress () << " send from other end "  );
           //return true;
           //return m_DevSameNode->Froward (packet, dest, protocolNumber, true);
           //return m_DevSameNode->SendFromInside (packet, dest, protocolNumber, true);
            }
          else
            {
               NS_LOG_DEBUG (GetAddress () << " send from this end "  );
              goto LinkedDevice;
            }
          return;
        }
    }
  
LinkedDevice:
   if (m_txMachineState == READY)
   {
         Ptr<Packet> packetforward = DequeueForward ();
         if (packetforward !=0)
         {
          TransmitStart (packetforward);
         }
   }
   */
    ForwardDown ();
    ARQAckTimeoutEvent = Simulator::Schedule(ARQAckTimer, &PointToPointNetDeviceGES::ARQAckTimeout, this, buffer );
    //NS_ASSERT (packetInbuffer);
} 

void
PointToPointNetDeviceGES::ARQN_ACKRecevie (Mac48Address dst, uint32_t packetid) 
{
    RemoteSatelliteTx * Satellite = ARQLookupTx (dst, packetid); 
    RemoteSatelliteARQBufferTx * buffer = Satellite->m_buffer;
    
    ARQTxBufferCheck (buffer);

    //ARQAckTimeout (buffer, packetid);
}
       
  
  
void
PointToPointNetDeviceGES::ARQACKRecevie (Mac48Address dst, uint32_t packetid)  
{
    RemoteSatelliteTx * Satellite = ARQLookupTx (dst, packetid); 
    RemoteSatelliteARQBufferTx * buffer = Satellite->m_buffer;
    
    NS_LOG_DEBUG (GetAddress () << " receives ack, with id " << packetid  << ", from  " <<  dst );
    //NS_LOG_DEBUG ("buffer->m_bufferSize " << buffer->m_bufferSize <<  ", buffer->m_listPacket->size " << buffer->m_listPacket->size () );
    
    ARQTxBufferCheck (buffer);
    
    if (buffer->m_listPacketId->size () == 0)
        {
             Ptr<Packet> packetSend = PreparePacketToSend ();
                          if (packetSend == 0)
                           {
                                NS_LOG_DEBUG (GetAddress () << " ARQACKRecevie, no packet got from ARQ "  );
                                return;
                           }
                          EnqueueForward (packetSend);
            return;
        }
    
  NS_ASSERT (buffer->m_listPacketId->front () == buffer->m_bufferStart);
  for (uint32_t kk = buffer->m_bufferStart; kk < packetid; kk++)
   {
    if (buffer->m_listPacketId->front () == kk ) // double check
      {
         uint32_t id = buffer->m_listPacketId->front ();
         Ptr<Packet> packettrace = buffer->m_listPacket->front ();
         Ptr<Packet> Copypackettrace = packettrace->Copy ();
         buffer->m_listPacket->pop_front ();
         buffer->m_listTxTime->pop_front ();
         buffer->m_listPacketId->pop_front ( );
         buffer->m_listPacketStatus->pop_front ();
         uint32_t retrynum = buffer->m_listPacketRetryNum->front ();
         buffer->m_listPacketRetryNum->pop_front ();
         buffer->m_bufferStart++;
         
         PppHeader ppp;
         Copypackettrace->PeekHeader (ppp);
         NS_LOG_DEBUG (GetAddress () << " pop_front tx packet receives ack, with id " << id  << ", transmission time " <<  retrynum << "\t" << ppp.GetDestAddre () << " id " << ppp.GetID ());
         m_ARQTxPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetDestAddre (), id, ppp.GetQos (), retrynum, true);
         
         Ptr<Packet>  packetARQ = Create<Packet> ();
         m_ARQrecPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), dst , packetARQ, Simulator::Now (), 19, 2048,  packetid);

         
         Ptr<const Packet>  packetkPeek;
         //Ptr<Packet>  packetSend;
         Ptr<Packet>  packet;  
                      
         Ptr<Packet> packetSend = PreparePacketToSend ();
         if (packetSend == 0)
             {
                NS_LOG_DEBUG (GetAddress () << " no packet got from ARQ, ack received "  );
                return;
             }
         EnqueueForward (packetSend);
           
        
         //return;
      }
    else
      { 
        NS_FATAL_ERROR  ("should not happen");
      }
   }
  
   ARQAckTimeout (buffer);
   
 


   
   


 

    
   /*
    std::list<uint32_t >::const_iterator PacketStatusiterator = buffer->m_listPacketStatus->begin ();
    std::list<Ptr<Packet> >::const_iterator Packetiterator = buffer->m_listPacket->begin ();
    std::list<Time >::const_iterator TxTimeiterator = buffer->m_listTxTime->begin ();
    std::list<uint32_t >::const_iterator PacketRetryNumiterator = buffer->m_listPacketRetryNum->begin ();
    
    for (std::list<uint32_t >::const_iterator it = buffer->m_listPacketId->begin (); it != buffer->m_listPacketId->end (); it++)
    {
        if ( (*it) == packetid &&  (*PacketStatusiterator) == TRANSMITTED )
         {
            PacketStatusiterator = buffer->m_listPacketStatus->erase (PacketStatusiterator);
            buffer->m_listPacketStatus->insert (PacketStatusiterator, ACK_REC);
            break; 
         }
        PacketStatusiterator++;
        Packetiterator++;
        TxTimeiterator++;
    }
    */
}


Ptr<Packet> 
PointToPointNetDeviceGES::ARQSend (RemoteSatelliteARQBufferTx * buffer)  
{   
     ARQTxBufferCheck (buffer);
       
    if (buffer->m_listPacket->size () == 0)
      {
         return 0;
      }
    
    Ptr<Packet> packet;
    Ptr<Packet> packetReturn;
    
   NS_LOG_DEBUG (GetAddress () << "  ARQSend am_listPacketId size " << buffer->m_listPacketId->size () << ", front " << buffer->m_listPacketId->front ());


    
    std::list<Ptr<Packet> >::const_iterator Packetiterator = buffer->m_listPacket->begin ();
    std::list<Time >::const_iterator TxTimeiterator = buffer->m_listTxTime->begin ();
    std::list<uint32_t >::const_iterator PacketIditerator = buffer->m_listPacketId->begin ();
    std::list<uint32_t >::const_iterator PacketRetryNumiterator = buffer->m_listPacketRetryNum->begin ();

    ARQTxBufferCheck (buffer);

    uint32_t retry_num = 0;        
    for (std::list<uint32_t >::const_iterator it = buffer->m_listPacketStatus->begin (); it != buffer->m_listPacketStatus->end (); it++)
    {
        if ((*it) == NO_TRANSMITTED)
        {
            packet =  (*Packetiterator);

            TxTimeiterator = buffer->m_listTxTime->erase (TxTimeiterator);
            buffer->m_listTxTime->insert (TxTimeiterator, Simulator::Now ());

            it = buffer->m_listPacketStatus->erase (it);

            buffer->m_listPacketStatus->insert (it, TRANSMITTED);

            retry_num = (* PacketRetryNumiterator);

            PacketRetryNumiterator = buffer->m_listPacketRetryNum->erase (PacketRetryNumiterator);

            retry_num++;
            buffer->m_listPacketRetryNum->insert (PacketRetryNumiterator, retry_num);

             
            NS_LOG_DEBUG (GetAddress () << "   aaARQSend am_listPacketId size " << buffer->m_listPacketId->size () << ", front " << buffer->m_listPacketId->front ());

                 
            packetReturn = packet->Copy ();
            if ( !ARQAckTimeoutEvent.IsRunning () )
              {
                  ARQAckTimeoutEvent = Simulator::Schedule(ARQAckTimer, &PointToPointNetDeviceGES::ARQAckTimeout, this, buffer);
               } 
            return packetReturn;
        }
        
        Packetiterator++;
        TxTimeiterator++;
        PacketIditerator++;
        PacketRetryNumiterator++;
    }
    
     NS_LOG_DEBUG (GetAddress () << "  after ARQSend am_listPacketId size " << buffer->m_listPacketId->size () << ", front " << buffer->m_listPacketId->front ());

    if ( !ARQAckTimeoutEvent.IsRunning () )
       {
           ARQAckTimeoutEvent = Simulator::Schedule(ARQAckTimer, &PointToPointNetDeviceGES::ARQAckTimeout, this, buffer);
       } 

    return 0;
}

Ptr<Packet> 
PointToPointNetDeviceGES::ARQSend (RemoteSatelliteARQBufferTx * buffer, Ptr<Packet> packet)
{
    uint32_t index;
     NS_LOG_DEBUG  (GetAddress () <<"buffer->m_listPacket->size () " <<  buffer->m_listPacket->size ()  << ", buffer->m_bufferSize " << buffer->m_bufferSize); // || buffer->m_listPacket->size () < buffer->m_bufferSize);

    NS_ASSERT (buffer->m_listPacket->size () < buffer->m_bufferSize || buffer->m_listPacket->size () == buffer->m_bufferSize );
    ARQTxBufferCheck (buffer);
    
    PppHeader ppp;
    packet->PeekHeader (ppp);
    Time txTime = Seconds (0);
    
    Mac48Address addrRemote = ppp.GetDestAddre ();    
    m_RemoteIdMapIterator = m_RemoteIdMap.find(addrRemote);
    if (m_RemoteIdMapIterator == m_RemoteIdMap.end())
      {
         m_RemoteIdMap[addrRemote] = m_RemoteIdCount;
         ARQ_Packetid[m_RemoteIdCount]=0;
         m_RemoteIdCount++;
      }
    index = m_RemoteIdMap.find(addrRemote)->second;
    
    NS_LOG_UNCOND(GetAddress () << " original id  " << ppp.GetID()  << " new id " << ARQ_Packetid[index]);
        
    uint16_t protocol = 0;
     ProcessHeader (packet, protocol);//t0 do
     ppp.SetID (ARQ_Packetid[index]);
     packet->AddHeader (ppp);
     
     PppHeader pppTest;
     packet->PeekHeader (pppTest);
     


    
    buffer->m_listPacket->push_back (packet);
    buffer->m_listTxTime->push_back (txTime);
    //buffer->m_listPacketId->push_back (ppp.GetID ());
    buffer->m_listPacketId->push_back (ARQ_Packetid[index]);
    buffer->m_listPacketStatus->push_back (NO_TRANSMITTED);
    buffer->m_listPacketRetryNum->push_back (0);
    
    ARQ_Packetid[index]++;
    //NS_LOG_DEBUG (GetAddress () << "  ARQSend am_listPacketId size " << buffer->m_listPacketId->size () << ", id " << pppTest.GetID () << ", " << pppTest.GetDestAddre ());

    
    return ARQSend (buffer); 
}


RemoteSatelliteTx * 
PointToPointNetDeviceGES::ARQLookupTx (Mac48Address dst, uint32_t packetid)  const
{ 
     // NS_LOG_DEBUG (GetAddress () << " create buffer for Tx " << dst );

   for (StatDeviceTx::const_iterator i = m_statDeviceTx.begin (); i != m_statDeviceTx.end (); i++)
    {
      if ((*i)->m_remoteAddress == dst)
        {
          return (*i);
        }
    } 
   //first received packet start the window 
   
  // create state RemoteSatelliteARQBuffer
  RemoteSatelliteARQBufferTx * buffer = new RemoteSatelliteARQBufferTx ();
  
  std::list<Ptr<Packet> > * listPacket = new std::list<Ptr<Packet> >;
  std::list<Time > * listTxTime = new std::list<Time >;
  std::list<uint32_t > * listPacketId = new std::list<uint32_t >;
  std::list<uint32_t > * listPacketStatus = new std::list<uint32_t >;
  std::list<uint32_t > * listPacketRetryNum = new std::list<uint32_t >;

  buffer->m_listPacket = listPacket;
  buffer->m_listTxTime = listTxTime;
  buffer->m_listPacketId = listPacketId; 
  buffer->m_listPacketStatus = listPacketStatus; 
  buffer->m_listPacketRetryNum = listPacketRetryNum; 
  
  buffer->m_bufferStart = 0;
  buffer->m_bufferSize = BUFFERSIZE;
  
  //create new RemoteSatellite
  RemoteSatelliteTx *Satellite = new RemoteSatelliteTx ();
  Satellite->m_buffer = buffer;
  Satellite->m_retryCount = ARQCOUNT;   //not needed
  Satellite->m_remoteAddress = dst;
  
  const_cast<PointToPointNetDeviceGES *> (this)->m_statDeviceTx.push_back (Satellite);

  
  NS_LOG_DEBUG (GetAddress () << " create buffer for Tx " << dst );

    ARQTxBufferCheck (buffer);
  
  return Satellite;
}



bool
PointToPointNetDeviceGES::SendAck (const Address &dest, 
  uint16_t protocolNumber, uint32_t packetid, uint32_t acktype, uint32_t QosARQ)
{
  NS_ASSERT (acktype == DATAACK || acktype == N_ACK);
  
  Ptr<Packet> packet = Create<Packet> ();;

  m_destAddress = Mac48Address::ConvertFrom (dest);

  
  if (m_DevSameNode0Flag)
  {
      int aa = 1;
      NS_LOG_UNCOND ("GES device on LEO cannot send ack");
      NS_ASSERT (aa=2);
  }
 
  if  (dest == GetBroadcast () )
     {
        m_destAddress =  Mac48Address::ConvertFrom (GetBroadcast() );
    }
  
          
  

  

  m_type =  acktype;
  

  


  
  //
  // If IsLinkUp() is false it means there is no channel to send any packet 
  // over so we just hit the drop trace on the packet and return an error.
  //
  if (IsLinkUp () == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  //
  // Stick a point to point protocol header on the packet in preparation for
  // shoving it out the door.
  //
  //m_Qos = m_rng->GetInteger (0, 3);
  m_Qos = QosARQ;
  //NS_LOG_UNCOND ("set ack m_Qos " << uint16_t(m_Qos));
  
  m_ackid = packetid;
  AddHeader (packet, protocolNumber);

  m_macTxTrace (packet);
  
   PppHeader ppp;
   packet->PeekHeader (ppp);
    
  m_Qos = ppp.GetQos ();
  //NS_LOG_UNCOND ("send ack m_Qos " << uint16_t(m_Qos));
  
  Ptr<Packet> PacketForwardtest = packet->Copy ();
  EnqueueForward (PacketForwardtest);
  
  return ForwardDown ();
}


} // namespace ns3
