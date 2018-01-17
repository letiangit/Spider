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
#include "point-to-point-net-device.h"
#include "point-to-point-channel.h"
#include "ppp-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointNetDevice");

NS_OBJECT_ENSURE_REGISTERED (PointToPointNetDevice);

TypeId 
PointToPointNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PointToPointNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName ("PointToPoint")
    .AddConstructor<PointToPointNetDevice> ()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (DEFAULT_MTU),
                   MakeUintegerAccessor (&PointToPointNetDevice::SetMtu,
                                         &PointToPointNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Address", 
                   "The MAC address of this device.",
                   Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                   MakeMac48AddressAccessor (&PointToPointNetDevice::m_address),
                   MakeMac48AddressChecker ())
    .AddAttribute ("DataRate", 
                   "The default data rate for point to point links",
                   DataRateValue (DataRate ("32768b/s")),
                   MakeDataRateAccessor (&PointToPointNetDevice::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("ReceiveErrorModel", 
                   "The receiver error model used to simulate packet loss",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDevice::m_receiveErrorModel),
                   MakePointerChecker<ErrorModel> ())
    .AddAttribute ("InterframeGap", 
                   "The time to wait between packet (frame) transmissions",
                   TimeValue (Seconds (0.0)),
                   MakeTimeAccessor (&PointToPointNetDevice::m_tInterframeGap),
                   MakeTimeChecker ())

    //
    // Transmit queueing discipline for the device which includes its own set
    // of trace hooks.
    //
    .AddAttribute ("TxQueue", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDevice::m_queue),
                   MakePointerChecker<Queue> ())

    //
    // Trace sources at the "top" of the net device, where packets transition
    // to/from higher layers.
    //
    .AddTraceSource ("MacTx", 
                     "Trace source indicating a packet has arrived "
                     "for transmission by this device",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_macTxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxDrop", 
                     "Trace source indicating a packet has been dropped "
                     "by the device before transmission",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_macTxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacPromiscRx", 
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a promiscuous trace,",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_macPromiscRxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacRx", 
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a non-promiscuous trace,",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_macRxTrace),
                     "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("MacRxDrop", 
                     "Trace source indicating a packet was dropped "
                     "before being forwarded up the stack",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_macRxDropTrace),
                     "ns3::Packet::TracedCallback")
#endif
    //
    // Trace souces at the "bottom" of the net device, where packets transition
    // to/from the channel.
    //
    .AddTraceSource ("PhyTxBegin", 
                     "Trace source indicating a packet has begun "
                     "transmitting over the channel",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyTxBeginTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxEnd", 
                     "Trace source indicating a packet has been "
                     "completely transmitted over the channel",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyTxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxDrop", 
                     "Trace source indicating a packet has been "
                     "dropped by the device during transmission",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyTxDropTrace),
                     "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("PhyRxBegin", 
                     "Trace source indicating a packet has begun "
                     "being received by the device",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyRxBeginTrace),
                     "ns3::Packet::TracedCallback")
#endif
    .AddTraceSource ("PhyRxEnd", 
                     "Trace source indicating a packet has been "
                     "completely received by the device",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyRxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxDrop", 
                     "Trace source indicating a packet has been "
                     "dropped by the device during reception",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")

    //
    // Trace sources designed to simulate a packet sniffer facility (tcpdump).
    // Note that there is really no difference between promiscuous and 
    // non-promiscuous traces in a point-to-point link.
    //
    .AddTraceSource ("Sniffer", 
                    "Trace source simulating a non-promiscuous packet sniffer "
                     "attached to the device",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_snifferTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PromiscSniffer", 
                     "Trace source simulating a promiscuous packet sniffer "
                     "attached to the device",
                     MakeTraceSourceAccessor (&PointToPointNetDevice::m_promiscSnifferTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}


PointToPointNetDevice::PointToPointNetDevice () 
  :
    m_txMachineState (READY),
    m_channel (0),
    m_channelRx (0),
    m_linkUp (false),
    m_linkchannelTx (0),
    //m_linkchannelRx (0),
    //CCmin (0),
    //CCmax (10),
    m_state (NO_CHANNEL_CONNECTED), //Initiation
    m_channel0_used (CHANNELNOTDEFINED),
    m_channel1_used (CHANNELNOTDEFINED),
    m_packetId (0),
    m_currentPkt (0)
{
  NS_LOG_FUNCTION (this);
  WaitChannel ();
  m_rng = CreateObject<UniformRandomVariable> ();

  uint64_t delay = m_rng->GetInteger (CCmin, CCmax);
  Simulator::Schedule (Seconds (delay), &PointToPointNetDevice::TryToSetLinkChannel, this);
}

PointToPointNetDevice::~PointToPointNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
PointToPointNetDevice::AddHeader (Ptr<Packet> p, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << p << protocolNumber);
  PppHeader ppp;
  ppp.SetProtocol (EtherToPpp (protocolNumber));
  ppp.SetType (m_type);
  ppp.SetSourceAddre (Mac48Address::ConvertFrom (GetAddress ()));
  NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () << " add header to " << m_destAddress);
  ppp.SetDestAddre (m_destAddress);
  NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () << " add header to " << m_destAddress);
  if (m_type != CHANNELACK)
   {
      m_packetId++;
      ppp.SetID (m_packetId);
   }
  else
   {
      ppp.SetID (m_ackid);
   }
  
  if (m_type == CHANNELREQ)
   {
      ppp.SetUsedChannel0 (m_channel0_used);
      ppp.SetUsedChannel1 (m_channel1_used);
   }
  
  p->AddHeader (ppp);
}

bool
PointToPointNetDevice::ProcessHeader (Ptr<Packet> p, uint16_t& param)
{
  NS_LOG_FUNCTION (this << p << param);
  PppHeader ppp;
  p->RemoveHeader (ppp);
  param = PppToEther (ppp.GetProtocol ());
  return true;
}

void
PointToPointNetDevice::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_node = 0;
  m_channel = 0;
  m_channelRx = 0;
  m_receiveErrorModel = 0;
  m_currentPkt = 0;
  NetDevice::DoDispose ();
}

void
PointToPointNetDevice::SetDataRate (DataRate bps)
{
  NS_LOG_FUNCTION (this);
  m_bps = bps;
}

void
PointToPointNetDevice::SetInterframeGap (Time t)
{
  NS_LOG_FUNCTION (this << t.GetSeconds ());
  m_tInterframeGap = t;
}

bool
PointToPointNetDevice::TransmitStart (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");
  NS_LOG_UNCOND (GetAddress () << " transmit to  ");


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
  Simulator::Schedule (txCompleteTime, &PointToPointNetDevice::TransmitComplete, this);
  bool result = m_channel->TransmitStart (p, this, txTime, m_linkchannelTx);
  //NS_LOG_UNCOND (Simulator::Now() <<  "\t" << GetAddress () << " TransmitStart at channel " << m_linkchannelTx);


  if (result == false)
    {
      m_phyTxDropTrace (p);
    }
  return result;
}

void
PointToPointNetDevice::TransmitComplete (void)
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

  NS_ASSERT_MSG (m_currentPkt != 0, "PointToPointNetDevice::TransmitComplete(): m_currentPkt zero");

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

  //
  // Got another packet off of the queue, so start the transmit process agin.
  //
  m_snifferTrace (p);
  m_promiscSnifferTrace (p);
  NS_LOG_UNCOND ("TransmitComplete");
  TransmitStart (p);
}
/*
bool
PointToPointNetDevice::Attach (Ptr<PointToPointChannel> ch)
{
  NS_LOG_FUNCTION (this << &ch);

  m_channel = ch;

  m_channel->Attach (this);

  //
  // This device is up whenever it is attached to a channel.  A better plan
  // would be to have the link come up when both devices are attached, but this
  // is not done for now.
  //
  NotifyLinkUp ();
  return true;
} */


bool
PointToPointNetDevice::Attach (Ptr<PointToPointChannel> ch)
{
  NS_LOG_FUNCTION (this << &ch);
  m_channel = ch;
  m_channel->Attach (this);


  //
  // This device is up whenever it is attached to a channel.  A better plan
  // would be to have the link come up when both devices are attached, but this
  // is not done for now.
  //
  NotifyLinkUp ();
  return true;
}

bool
PointToPointNetDevice::Attach (Ptr<PointToPointChannel> ch, uint8_t rx)
{

  NS_LOG_FUNCTION (this << &ch);
  if (rx == 0)
   {
        m_channel = ch;
        m_channel->Attach (this);
        std::cout << GetAddress () << " set to  m_channel "  << '\n';
   }
  else if (rx == 1)
   {
        m_channelRx = ch;
        m_channelRx->Attach (this);
        std::cout << GetAddress () << " set to  m_channelRx "  << '\n';
   }
      
  
  //m_channel = ch;

  //m_channel->Attach (this);


  //
  // This device is up whenever it is attached to a channel.  A better plan
  // would be to have the link come up when both devices are attached, but this
  // is not done for now.
  //
  NotifyLinkUp ();
        //   std::cout << "Attach....... : \n";

  return true;
}

void
PointToPointNetDevice::SetQueue (Ptr<Queue> q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}

void
PointToPointNetDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void
PointToPointNetDevice::WaitChannel ()
{
    m_linkchannelRx++;
    m_linkchannelRx = m_linkchannelRx % 4; 
    m_WaitChannelEvent = Simulator::Schedule (ChannelWaiting_Interval, &PointToPointNetDevice::WaitChannel, this);
}


void
PointToPointNetDevice::ReceiveChannel (Ptr<Packet> packet, uint32_t linkchannel)
{
  NS_LOG_UNCOND (GetAddress () << "..received linkchannel " << linkchannel << ", m_linkchannelRx " << m_linkchannelRx);  
   if (linkchannel != m_linkchannelRx)
     {
        NS_LOG_UNCOND ("packet can not be recevied since sender and receiver are not in the same link channel");  
        return;
     }
   //mimic the real link channel     
   m_WaitChannelEvent.Cancel ();
   m_linkchannelRx = linkchannel; // in case m_linkchannelRx changes during Cancelling.

           
    PppHeader ppp;
    packet->PeekHeader (ppp);
    
    for (uint16_t i=0; i<100; i++)
    {
        if (ppp.GetType () == i)
        {
            NS_LOG_UNCOND ("ReceiveChannel , type " << i);  
            break;
        }
    }
    
    if (ppp.GetType () == CHANNELRESP  && ppp.GetDestAddre () != Mac48Address::ConvertFrom (GetAddress() ) )
      {
          NS_LOG_UNCOND ("Forward , packet " << ppp.GetDestAddre () );  
          Forward (packet, 0);
          return;
      }
    
    if (m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
      {
        NS_LOG_UNCOND (GetAddress ()  <<  "drop packet since channel is already selected " );
        //0114, ack is ignored
       if (ppp.GetType () == CHANNELREQ)
        {
           SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
        }
       else if (ppp.GetType () == CHANNELRESP)
        {
           SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ()); // external_internal_transfer
        }
       else if (ppp.GetType () == CHANNELACK)
        {
           SendChannelRequest ();
        }
        return;
      } 

   
   if (m_state == NO_CHANNEL_CONNECTED)
   {
      NS_LOG_UNCOND (GetAddress ()  <<  " state NO_CHANNEL_CONNECTED" );  
      if (ppp.GetType () == CHANNELREQ )
        {
           SetState (INTERNAL_REC_CHANNEL_REQ);
           SetState (INTERNAL_SEND_CHANNEL_RESP);
           NS_LOG_UNCOND (GetAddress ()  <<  " send SendChannelResponse NO_CHANNEL_CONNECTED " );  
           SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
        }
       return;
   }
       
       
   if (m_state == EXTERNAL_SEND_CHANNEL_REQ )
    {
       NS_LOG_UNCOND (GetAddress ()  <<  "current state EXTERNAL_SEND_CHANNEL_REQ" );  
       if (ppp.GetType () == CHANNELRESP  )
        {
          if (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress()) )
           {
               SetState (EXTERNAL_REC_CHANNEL_RESP);
               SetState (EXTERNAL_SEND_CHANNEL_ACK);
               NS_LOG_UNCOND (GetAddress ()  <<  " send SendChannelACK " );  
               SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ());
           }
        }
       else if (ppp.GetType () == CHANNELREQ )
        {
           SetState (INTERNAL_REC_CHANNEL_REQ);
           SetState (INTERNAL_SEND_CHANNEL_RESP);
           NS_LOG_UNCOND (GetAddress ()  <<  " send SendChannelResponse " );  
           SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
        } 
      return;
    }
    
    
   if (m_state == EXTERNAL_SEND_CHANNEL_ACK )
    {
       NS_LOG_UNCOND (GetAddress ()  <<  "current state EXTERNAL_SEND_CHANNEL_ACK" );  
       if (ppp.GetType () == CHANNELREQ)
        {
           SetState (EXTERNAL_REC_CHANNEL_REQ);
           SetState (EXTERNAL_SEND_CHANNEL_RESP);
           NS_LOG_UNCOND (GetAddress ()  <<  " send SendChannelResponse EXTERNAL_SEND_CHANNEL_ACK" );  
           SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
        }
       //0114, ack is ignored
       else if (ppp.GetType () == CHANNELRESP)
        {
           SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ()); // external_internal_transfer
        }
       else if (ppp.GetType () == CHANNELACK)
        {
           SendChannelRequest ();
        }
       return;          
    }
    
    
  if (m_state == EXTERNAL_SEND_CHANNEL_RESP )
    {
       NS_LOG_UNCOND (GetAddress ()  <<  "current state EXTERNAL_SEND_CHANNEL_RESP" );  
       if (ppp.GetType () == CHANNELACK)
         {
           SetState (EXTERNAL_REC_CHANNEL_ACK);
           NS_LOG_UNCOND (GetAddress ()  <<  " set EXTERNAL_REC_CHANNEL_ACK, CHANNEL SELECTED" );  

            if ( m_watingChannelConfEvent.IsRunning ())
             {
               NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelConfEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelConfEvent));
               m_watingChannelConfEvent.Cancel ();
               NS_LOG_UNCOND ("IsExpired " <<  m_watingChannelConfEvent.IsExpired ());  
             }
            if ( m_watingChannelRespEvent.IsRunning ())
             {
               NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelRespEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelRespEvent));
               m_watingChannelRespEvent.Cancel ();
               NS_LOG_UNCOND ("IsExpired " <<  m_watingChannelRespEvent.IsExpired ());  
             }
            
           if ( m_SendChannelResponsePacketEvent.IsRunning() )
             {
               m_SendChannelResponsePacketEvent.Cancel ();
             }
            
          if ( m_SendChannelRequestPacketEvent.IsRunning() )
             {
               m_SendChannelRequestPacketEvent.Cancel ();
             }
          }
       //0114, ack is ignored
       else  if (ppp.GetType () == CHANNELREQ)
        {
             SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
        }
       else  if (ppp.GetType () == CHANNELRESP)
        {
              SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ()); // external_internal_transfer
        }
      return;       
    }
    
  if (m_state == INTERNAL_SEND_CHANNEL_RESP )
    {
       NS_LOG_UNCOND (GetAddress ()  <<  "current state INTERNAL_SEND_CHANNEL_RESP" );        
       if (ppp.GetType () == CHANNELACK)
        {
           SetState (INTERNAL_REC_CHANNEL_ACK);
           SetState (INTERNAL_SEND_CHANNEL_REQ);
           NS_LOG_UNCOND (GetAddress ()  <<  " send SendChannelRequest " );  
           SendChannelRequest ();
        }
       else if (ppp.GetType () == CHANNELRESP)
        {
           //SetState (EXTERNAL_SEND_CHANNEL_ACK);
           SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ()); // external_internal_transfer
        }
      //0114, ack is ignored
      else if (ppp.GetType () == CHANNELREQ)
        {
            SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
        }
      return;      
    }
    
   if (m_state == INTERNAL_SEND_CHANNEL_REQ )
    {
       NS_LOG_UNCOND (GetAddress ()  <<  "current state  INTERNAL_SEND_CHANNEL_REQ" );        
       if (ppp.GetType () == CHANNELRESP)
        {
           SetState (INTERNAL_REC_CHANNEL_RESP);
           SetState (INTERNAL_SEND_CHANNEL_ACK);
           NS_LOG_UNCOND (GetAddress ()  <<  " set INTERNAL_SEND_CHANNEL_ACK, CHANNEL SELECTED" );  
           NS_LOG_UNCOND (GetAddress ()  <<  " send SendChannelACK " );  
            if ( m_watingChannelConfEvent.IsRunning ())
             {
               NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelConfEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelConfEvent));
               m_watingChannelConfEvent.Cancel ();
               NS_LOG_UNCOND ("IsExpired " <<  m_watingChannelConfEvent.IsExpired ());  
             }
            if ( m_watingChannelRespEvent.IsRunning ())
             {
               NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelRespEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelRespEvent));
               m_watingChannelRespEvent.Cancel ();
               NS_LOG_UNCOND ("IsExpired " <<  m_watingChannelRespEvent.IsExpired ());  
             }
            
         if ( m_SendChannelResponsePacketEvent.IsRunning() )
            {
             m_SendChannelResponsePacketEvent.Cancel ();
            }
            
         if ( m_SendChannelRequestPacketEvent.IsRunning() )
           {
             m_SendChannelRequestPacketEvent.Cancel ();
           }
           
           SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ());
        }
       //0114, ack is ignored
       else  if (ppp.GetType () == CHANNELREQ)
        {
             SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
        }
       else  if (ppp.GetType () == CHANNELACK)
        {
           SendChannelRequest ();
        }
       return;      
    }
    
    

    /*
   if (m_state == RX_Selected && ppp.GetType () == CHANNELREQ)
    {
        NS_LOG_UNCOND (GetAddress ()  <<   "drop CHANNELREQ since its already in  RX_TX_Selected state " );  
        return;
    }
       
    if (ppp.GetType () == CHANNELREQ)
      {
        //NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " receive CHANNELREQ at channel " << m_linkchannelRx  );
        if (m_state == Waiting_ChannelRequest || Waiting_ChannelResp)
          {
            NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " receive CHANNELREQ at channel " << m_linkchannelRx  );
            SetState (RX_Selected);
            SendChannelResponse ();
          }
      }
    else if  (ppp.GetType () == CHANNELCONF)
      {
        //NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " receive CHANNELCONF at channel " << m_linkchannelRx  );
        if (m_state == RX_Selected)
          {
             NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " receive CHANNELCONF at channel " << m_linkchannelRx  );
             SetState (RX_TX_Selected);
             NS_LOG_UNCOND ("m_watingChannelRespEventIsRunning  " <<  m_watingChannelRespEvent.IsRunning ());  
            if (m_watingChannelConfEvent.IsRunning ())
             {
               NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelConfEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelConfEvent));
               m_watingChannelConfEvent.Cancel ();
               NS_LOG_UNCOND ("IsExpired " <<  m_watingChannelConfEvent.IsExpired ());
             }
            if (m_watingChannelRespEvent.IsRunning ())
             {
               NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelRespEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelRespEvent));
               m_watingChannelRespEvent.Cancel ();
               NS_LOG_UNCOND ("IsExpired " <<  m_watingChannelRespEvent.IsExpired ());  
             }
             
           if ( m_SendChannelResponsePacketEvent.IsRunning() )
            {
              m_SendChannelResponsePacketEvent.Cancel ();
            }
            
          if ( m_SendChannelRequestPacketEvent.IsRunning() )
           {
             m_SendChannelRequestPacketEvent.Cancel ();
           }
             
            NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " SetState RX_TX_Selected ");
          }
      }
    else if (ppp.GetType () == CHANNELRESP)
      {
       // NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " receive CHANNELRESP at channel " << m_linkchannelRx  );
        if (m_state == Waiting_ChannelResp)
         {
            NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " receive CHANNELRESP at channel " << m_linkchannelRx  );
            SetState (RX_TX_Selected);
            NS_LOG_UNCOND ("m_watingChannelRespEventIsRunning  " <<  m_watingChannelRespEvent.IsRunning ());  

            if ( m_watingChannelConfEvent.IsRunning ())
             {
               NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelConfEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelConfEvent));
               m_watingChannelConfEvent.Cancel ();
               NS_LOG_UNCOND ("IsExpired " <<  m_watingChannelConfEvent.IsExpired ());  
             }
            if ( m_watingChannelRespEvent.IsRunning ())
             {
               NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelRespEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelRespEvent));
               m_watingChannelRespEvent.Cancel ();
               NS_LOG_UNCOND ("IsExpired " <<  m_watingChannelRespEvent.IsExpired ());  
             }
            
         if ( m_SendChannelResponsePacketEvent.IsRunning() )
           {
             m_SendChannelResponsePacketEvent.Cancel ();
           }
            
        if ( m_SendChannelRequestPacketEvent.IsRunning() )
           {
             m_SendChannelRequestPacketEvent.Cancel ();
           }
            
            SendChannelConf ();
            NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " SetState RX_TX_Selected ");
        }
      }
   else
     {
        NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " receive packets with unknown type " << m_linkchannelRx  );
        NS_LOG_UNCOND ("ReceiveChannel , type " << ppp.GetType ());  
     }
     */
  
    //Receive (packet);
    return;
}

void
PointToPointNetDevice::Receive (Ptr<Packet> packet)
{
  NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " receive ...... "   );
  NS_LOG_FUNCTION (this << packet);
  uint16_t protocol = 0;

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
          NS_LOG_UNCOND (".." << protocol << "\t" << GetRemote () << "\t" << GetAddress () );
        }

      m_macRxTrace (originalPacket);
      m_rxCallback (this, packet, protocol, GetRemote ());
      //NS_LOG_UNCOND ( GetAddress () << "\t" << m_address );
    }
}

Ptr<Queue>
PointToPointNetDevice::GetQueue (void) const
{ 
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
PointToPointNetDevice::NotifyLinkUp (void)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

void
PointToPointNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this);
  m_ifIndex = index;
}

uint32_t
PointToPointNetDevice::GetIfIndex (void) const
{
  return m_ifIndex;
}

Ptr<Channel>
PointToPointNetDevice::GetChannel (void) const
{
  return m_channel;
}

//
// This is a point-to-point device, so we really don't need any kind of address
// information.  However, the base class NetDevice wants us to define the
// methods to get and set the address.  Rather than be rude and assert, we let
// clients get and set the address, but simply ignore them.

void
PointToPointNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
  NS_LOG_UNCOND ("m_address " << m_address );
}

Address
PointToPointNetDevice::GetAddress (void) const
{
  //NS_LOG_UNCOND ("GetAddress " << m_address );
  return m_address;
}

bool
PointToPointNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}

void
PointToPointNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this);
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

//
// This is a point-to-point device, so every transmission is a broadcast to
// all of the devices on the network.
//
bool
PointToPointNetDevice::IsBroadcast (void) const
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
PointToPointNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
PointToPointNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address
PointToPointNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("01:00:5e:00:00:00");
}

Address
PointToPointNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address ("33:33:00:00:00:00");
}

bool
PointToPointNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

bool
PointToPointNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void 
PointToPointNetDevice::TryToSetLinkChannel (void)
{
    
   NS_LOG_FUNCTION (this);
   
   switch (m_state)
     {
       case NO_CHANNEL_CONNECTED:
           SetState (EXTERNAL_CHANNEL_COMMAND);
           SetState (EXTERNAL_SEND_CHANNEL_REQ);
           SendChannelRequest (); 
           break;
       case EXTERNAL_CHANNEL_COMMAND:
           break;
       case EXTERNAL_SEND_CHANNEL_REQ:
           break;  
       case EXTERNAL_REC_CHANNEL_RESP:
           break;
       case EXTERNAL_SEND_CHANNEL_ACK:
           break;
       case EXTERNAL_REC_CHANNEL_REQ:
           break;
       case EXTERNAL_SEND_CHANNEL_RESP:
           break; 
       case EXTERNAL_REC_CHANNEL_ACK:
           break; 
       case INTERNAL_REC_CHANNEL_REQ:
           break; 
       case INTERNAL_SEND_CHANNEL_RESP:
           break; 
       case INTERNAL_REC_CHANNEL_ACK:
           break; 
       case INTERNAL_SEND_CHANNEL_REQ:
           break; 
       case INTERNAL_REC_CHANNEL_RESP:
           break; 
       case INTERNAL_SEND_CHANNEL_ACK:
           break; 
     }
}

void
PointToPointNetDevice::SetState (LinkChannelState state)
{
    NS_LOG_UNCOND (GetAddress () << " SetState " << state);
    m_state = state;
}

/*
LinkChannelState
PointToPointNetDevice::GetState (void) const
{
    return m_state;
}
*/


void 
PointToPointNetDevice::ChangeTxChannel (void)
{
   m_linkchannelTx++;
   m_linkchannelTx = m_linkchannelTx % 4;
}

void 
PointToPointNetDevice::SendChannelRequestPacket (uint8_t counter)
{
   if (IsLinkUp () == false)
    {
      NS_ASSERT ("there is no channel");  
      return;
    } 
    
   Ptr<Packet> packet = Create<Packet> ();
   m_type = CHANNELREQ;
   
   if (counter == 4) // && m_SendChannelRequestPacketEvent.IsRunning() )
     {
       NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () <<  " stop sending m_SendChannelRequestPacketEvent "); 
       if ( m_SendChannelRequestPacketEvent.IsRunning() )
         {
           m_SendChannelRequestPacketEvent.Cancel ();
         }
       return;
       //limit to 4 packets
     }
   counter++;
   
   NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " send SendChannelRequestPacket at channel " << m_linkchannelTx << " counter " << counter);
   printf ("counter %d \n" , counter);
   Send (packet, Mac48Address ("ff:ff:ff:ff:ff:fe"), 0x800); // GetBroadcast (), 0x800 are not really used

   m_SendChannelRequestPacketEvent = Simulator::Schedule (Channel_delay_packet, &PointToPointNetDevice::SendChannelRequestPacket, this, counter);
}




void 
PointToPointNetDevice::SendChannelResponsePacket (uint8_t counter)
{
   if (IsLinkUp () == false)
    {
      NS_ASSERT ("there is no channel");  
      return;
    } 
   
   if (counter == 4) // && m_SendChannelResponsePacketEvent.IsRunning() )
     {
       NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () <<  " stop sending m_SendChannelResponsePacket "); 
       
       //m_SendChannelResponsePacketEvent.Cancel ();
       if ( m_SendChannelResponsePacketEvent.IsRunning() )
         {
           m_SendChannelResponsePacketEvent.Cancel ();
         }
       return;
       //limit to 16 packets
     }
   
   /*
   if (counter % 4 == 0)
     {
       ChangeTxChannel ();
     } */
   
   counter++;
   
   Ptr<Packet> packet = Create<Packet> ();

   m_type = CHANNELRESP;
   
   NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " send SendChannelResponsePacket at channel " << m_linkchannelTx << " counter " << counter);
   printf ("counter %d \n" , counter);
   Send (packet, Mac48Address ("ff:ff:ff:ff:ff:fe"), 0x800); // GetBroadcast (), 0x800 are not really used
   m_SendChannelResponsePacketEvent = Simulator::Schedule (Channel_delay_packet, &PointToPointNetDevice::SendChannelResponsePacket, this, counter);
} 

void 
PointToPointNetDevice::SendChannelRequest (void)
{
    if (m_watingChannelRespEvent.IsRunning ())
     {
        m_watingChannelRespEvent.Cancel();
     }
    
    if (m_watingChannelConfEvent.IsRunning ())
     {
        m_watingChannelConfEvent.Cancel();
     }
    
    if ( m_SendChannelRequestPacketEvent.IsRunning() )
      {
           m_SendChannelRequestPacketEvent.Cancel ();
      }
    
    if ( m_SendChannelResponsePacketEvent.IsRunning() )
      {
           m_SendChannelResponsePacketEvent.Cancel ();
      }
    
     
        
    if (m_state == EXTERNAL_SEND_CHANNEL_REQ)
      {
            ChangeTxChannel ();
      }
    
    if (m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
      {
        //SendChannelRequestPacket (0);
        return;
      }

    
    SendChannelRequestPacket (0);
    NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " start channel request " );
    uint16_t delay = m_rng->GetInteger (1, backoffcounter);
    Time ChannelResp_delay_total = Seconds(ChannelResp_delay + delay);
    //m_watingChannelRespEvent = Simulator::Schedule (ChannelResp_delay_total, &PointToPointNetDevice::SendChannelRequest, this);
    if (m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
      {
        Simulator::Schedule (ChannelResp_delay_total, &PointToPointNetDevice::SendChannelRequest, this);
      }
    else
     {   
        m_watingChannelRespEvent = Simulator::Schedule (ChannelResp_delay_total, &PointToPointNetDevice::SendChannelRequest, this);       
     }

}

void 
PointToPointNetDevice::SendChannelResponse (Mac48Address dest, uint8_t channel0, uint8_t channel1)
{
   if (m_watingChannelConfEvent.IsRunning ())
     {
        m_watingChannelConfEvent.Cancel();
     }
   
   if (m_watingChannelRespEvent.IsRunning ())
     {
        m_watingChannelRespEvent.Cancel();
     }
    
   
   
   if ( m_SendChannelRequestPacketEvent.IsRunning() )
         {
           m_SendChannelRequestPacketEvent.Cancel ();
         }
   
   if ( m_SendChannelResponsePacketEvent.IsRunning() )
         {
           m_SendChannelResponsePacketEvent.Cancel ();
         }
   
   
   
   
   
   
    //ChangeTxChannel ();
   m_destAddress = dest;
   
   
   SendChannelResponsePacket (0);
   NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " start channel resp to " << m_destAddress );
    uint16_t delay = m_rng->GetInteger (1, backoffcounter);
    Time ChannelConf_delay_total = Seconds(ChannelConf_delay + delay);
   //m_watingChannelConfEvent = Simulator::Schedule (ChannelConf_delay_total, &PointToPointNetDevice::SendChannelRequest, this);
   if (m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
     {
         Simulator::Schedule (ChannelConf_delay_total, &PointToPointNetDevice::SendChannelRequest, this);
     }
   else
    {
         m_watingChannelConfEvent = Simulator::Schedule (ChannelConf_delay_total, &PointToPointNetDevice::SendChannelRequest, this);
    }

} 


void 
PointToPointNetDevice::SendChannelConf ()
{
  if (IsLinkUp () == false)
    {
      NS_ASSERT ("there is no channel");  
      return;
    } 
    
   Ptr<Packet> packet = Create<Packet> ();

   //PppHeader ppp;
   //ppp.SetType (CHANNELCONF);
   m_type = CHANNELCONF;
   
   //packet->AddHeader (ppp);
   //m_linkchannelTx++;  use the  m_linkchannelTx of sending channelReq
   NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " send SendChannelConf at channel " << m_linkchannelTx);
   Send (packet, Mac48Address ("ff:ff:ff:ff:ff:fe"), 0x800); // GetBroadcast (), 0x800 are not really used

}

void
PointToPointNetDevice::SendChannelACK (Mac48Address dest, uint32_t packetid)
{
    if (IsLinkUp () == false)
    {
      NS_ASSERT ("there is no channel");  
      return;
    } 
    
    
    

   Ptr<Packet> packet = Create<Packet> ();

   m_destAddress = dest;
   m_ackid = packetid;
   m_type = CHANNELACK;
   
   NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " send sendChannelACK at channel " << m_linkchannelTx);
   Send (packet, Mac48Address ("ff:ff:ff:ff:ff:fe"), 0x800); // GetBroadcast (), 0x800 are not really used
}


void 
PointToPointNetDevice::Forward (Ptr<Packet> packet, uint8_t counter)
{
   if (IsLinkUp () == false)
    {
      NS_ASSERT ("there is no channel");  
      return;
    } 

   
    if (counter == 4) // && m_SendChannelResponsePacketEvent.IsRunning() )
     {
       NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () <<  " stop Forward packet "); 
       if ( m_watingChannelForwardEvent.IsRunning() )
         {
           m_watingChannelForwardEvent.Cancel ();
         }
       return;
     }

   counter++;
   
  if (m_txMachineState == READY)
     {
       TransmitStart (packet);
     }
   
  m_watingChannelForwardEvent = Simulator::Schedule (Channel_delay_packet, &PointToPointNetDevice::Forward, this, packet, counter);
}



bool
PointToPointNetDevice::Send (
  Ptr<Packet> packet, 
  const Address &dest, 
  uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
  NS_LOG_LOGIC ("UID is " << packet->GetUid ());
  
  //NS_LOG_UNCOND (Simulator::Now() << " send packet to " << dest << "\t" << m_state);

  /*
  if (dest == GetBroadcast () && m_state != RX_TX_Selected)
   {
      NS_LOG_UNCOND (Simulator::Now() <<" channel is not selected, " << GetAddress () << ", size " << packet->GetSize ());
      return false; // temperorily, broadcast is only used for channel selection 
   }*/
  
  NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", send to " << dest);

  if (packet->GetSize () > 1000)
   {
      NS_LOG_UNCOND (Simulator::Now() <<" drop since too big size, " << GetAddress () << ", size " << packet->GetSize ());
      return false; // temperorily, broadcast is only used for channel selection 
   }

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
  AddHeader (packet, protocolNumber);

  m_macTxTrace (packet);

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
          packet = m_queue->Dequeue ();
          m_snifferTrace (packet);
          m_promiscSnifferTrace (packet);
          return TransmitStart (packet);
        }
      return true;
    }

  // Enqueue may fail (overflow)
  m_macTxDropTrace (packet);
  return false;
}

bool
PointToPointNetDevice::SendFrom (Ptr<Packet> packet, 
                                 const Address &source, 
                                 const Address &dest, 
                                 uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);
  return false;
}

Ptr<Node>
PointToPointNetDevice::GetNode (void) const
{
  return m_node;
}

void
PointToPointNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
  m_node = node;
}

bool
PointToPointNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
PointToPointNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  m_rxCallback = cb;
}

void
PointToPointNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  m_promiscCallback = cb;
}

bool
PointToPointNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
PointToPointNetDevice::DoMpiReceive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  Receive (p);
}

Address 
PointToPointNetDevice::GetRemote (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_channel->GetNDevices () == 2);
  for (uint32_t i = 0; i < m_channel->GetNDevices (); ++i)
    {
      Ptr<NetDevice> tmp = m_channel->GetDevice (i);
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
PointToPointNetDevice::SetMtu (uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}

uint16_t
PointToPointNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}

uint16_t
PointToPointNetDevice::PppToEther (uint16_t proto)
{
  NS_LOG_FUNCTION_NOARGS();
  switch(proto)
    {
    case 0x0021: return 0x0800;   //IPv4
    case 0x0057: return 0x86DD;   //IPv6
    default: NS_ASSERT_MSG (false, "PppToEther PPP Protocol number not defined!");
    }
  return 0;
}

uint16_t
PointToPointNetDevice::EtherToPpp (uint16_t proto)
{
  NS_LOG_FUNCTION_NOARGS();
  switch(proto)
    {
    case 0x0800: return 0x0021;   //IPv4
    case 0x86DD: return 0x0057;   //IPv6
    default: NS_ASSERT_MSG (false, "EtherToPpp PPP Protocol number not defined!");
    }
  return 0;
}


} // namespace ns3
