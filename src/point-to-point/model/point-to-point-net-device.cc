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
#include "ns3/boolean.h"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <fstream>

using namespace std;
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
    .AddAttribute ("ChannelResp_delay", 
                   "Time out for SendChannelRequest",
                   TimeValue (Seconds(21.0)),
                   MakeTimeAccessor (&PointToPointNetDevice::ChannelResp_delay),
                   MakeTimeChecker ())
    .AddAttribute ("ChannelConf_delay", 
                   "Time out for SendChannelResponse",
                   TimeValue (Seconds(17.0)),
                   MakeTimeAccessor (&PointToPointNetDevice::ChannelConf_delay),  
                   MakeTimeChecker ())
    .AddAttribute ("Channel_delay_packet", 
                   "The time interval for sending packets during channel selection",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&PointToPointNetDevice::Channel_delay_packet),
                   MakeTimeChecker ())     
    .AddAttribute ("ChannelWaiting_Interval", 
                   "Rx channel switch interval",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&PointToPointNetDevice::ChannelWaiting_Interval),
                   MakeTimeChecker ())      
    .AddAttribute ("backoffcounter", "backoff counter for restarting channel request",
                   UintegerValue (20000),
                   MakeUintegerAccessor (&PointToPointNetDevice::backoffcounter),  
                   MakeUintegerChecker<uint16_t> ())      
    .AddAttribute ("StartChannelSelection", "channel selection is initiated by external command (true) or internal random backoff (false)",
                   BooleanValue (false),
                   MakeBooleanAccessor (&PointToPointNetDevice::m_externalChSel),
                   MakeBooleanChecker ())
          
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
    .AddTraceSource ("ChannelSelected",
                    "Channel has been successfully Selected"
                    "The header of successfully transmitted packet",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_channelSelected),
                    "ns3::PointToPointNetDevice::ChannelSelectedCallback")
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
    CCmax (100),
    //m_linkchannelRx (0),
    m_state (NO_CHANNEL_CONNECTED), //Initiation
    m_type (DATATYPE),    
    m_channel0_usedInside (CHANNELNOTDEFINED),
    m_channel1_usedInside (CHANNELNOTDEFINED),
    m_channel0_usedOutside (CHANNELNOTDEFINED),
    m_channel1_usedOutside (CHANNELNOTDEFINED),
    m_packetId (0),
    m_currentPkt (0)
{
  NS_LOG_FUNCTION (this);
  WaitChannel ();
  m_rng = CreateObject<UniformRandomVariable> ();

  uint64_t delay = m_rng->GetInteger (1, CCmax);
  Simulator::Schedule (Seconds (delay), &PointToPointNetDevice::TryToSetLinkChannelExternal, this);
}

PointToPointNetDevice::~PointToPointNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
PointToPointNetDevice::TryToSetLinkChannelExternal ()
{
  NS_LOG_FUNCTION (this);
  if (m_externalChSel)
    {
      std::ofstream myfile;
      std::string dropfile="./OptimalRawGroup/channelSelected.txt";
      myfile.open (dropfile, ios::out | ios::app);
      myfile << "Device "  << GetAddress () << " receive external command at " << Simulator::Now ()<< "\n";
      myfile.close();
      TryToSetLinkChannel ();
    }
}

void
PointToPointNetDevice::AddHeader (Ptr<Packet> p, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << p << protocolNumber);
  PppHeader ppp;
  ppp.SetProtocol (EtherToPpp (protocolNumber));
  NS_LOG_UNCOND ("m_type " << m_type);
  ppp.SetType (m_type);
  ppp.SetSourceAddre (Mac48Address::ConvertFrom (GetAddress ()));
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
      ppp.SetUsedChannel0 (m_channel0_usedInside);
      ppp.SetUsedChannel1 (m_channel0_usedInside);
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
  NS_LOG_UNCOND ("SIZE " << p->GetSize () << ", txTime " << txTime);
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
  NS_ASSERT_MSG (!ch->IsChannelUni (), "incorrect channel type ");
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
PointToPointNetDevice::Attach (Ptr<PointToPointChannel> ch, uint16_t rx)
{

  NS_LOG_FUNCTION (this << &ch);
  NS_ASSERT_MSG (ch->IsChannelUni (), "incorrect channel type ");
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
  uint16_t repetition = 0;  
  while (1)
   {
    if (repetition == CHANNELNUMBER)
     {
        NS_LOG_UNCOND (GetAddress ()  <<  " no available rx channel...... "); 
     }
    m_linkchannelRx++;
    m_linkchannelRx = m_linkchannelRx % CHANNELNUMBER; 
    
    uint16_t channel= m_linkchannelRx;
    if (channel != m_channel0_usedInside  ) // && channel != m_linkchannelTx 
      {
        //NS_LOG_UNCOND (GetAddress ()  <<  " WaitChannel " << m_linkchannelRx); 
        break;
      }
    repetition++;
   }
   m_WaitChannelEvent = Simulator::Schedule (ChannelWaiting_Interval, &PointToPointNetDevice::WaitChannel, this);
}


void
PointToPointNetDevice::ReceiveChannel (Ptr<Packet> packet, uint32_t linkchannel)
{
  //NS_LOG_UNCOND ("add m_phyRxEndTrace " );  
  m_phyRxEndTrace (packet);
  NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () << "..received linkchannel " << linkchannel << ", m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx " << m_linkchannelTx);  
  if (m_channel->IsChannelUni())
    {
      NS_ASSERT (m_channel0_usedInside == CHANNELNOTDEFINED);
      NS_ASSERT (m_channel1_usedInside == CHANNELNOTDEFINED);
      NS_ASSERT (m_channel0_usedOutside == CHANNELNOTDEFINED);
      NS_ASSERT (m_channel1_usedOutside == CHANNELNOTDEFINED);
    }
  //NS_LOG_UNCOND (m_channel0_usedInside << "\t " << m_channel1_usedInside << "\t" << m_channel0_usedOutside << "\t" << m_channel1_usedOutside) ;
  
  
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
  
   if (linkchannel != m_linkchannelRx)
     {
        NS_LOG_UNCOND ("packet can not be recevied since sender and receiver are not in the same link channel");  
        return;
     }
   //mimic the real link channel   
   if (m_WaitChannelEvent.IsRunning())
    {
        m_WaitChannelEvent.Cancel ();
        if (m_linkchannelRx == m_linkchannelTx || m_linkchannelRx == m_channel0_usedInside)
            {
                NS_ASSERT (m_state == NO_CHANNEL_CONNECTED || m_state == EXTERNAL_SEND_CHANNEL_REQ);
                NS_LOG_UNCOND ("restart channel request since tx channel " << m_linkchannelTx << " is used by rx " << m_linkchannelRx << ", or m_channel0_usedInside " << m_channel0_usedInside );  
                ChangeTxChannel ();
                //SetState (NO_CHANNEL_CONNECTED);
                //TryToSetLinkChannel ();
            }
    }
   m_linkchannelRx = linkchannel; // in case m_linkchannelRx changes during Cancelling.

    
    if (ppp.GetType () == CHANNELRESP  && ppp.GetDestAddre () != Mac48Address::ConvertFrom (GetAddress() ) )
      {
          NS_LOG_UNCOND ("Forward , packet " << ppp.GetDestAddre () );  
          Forward (packet, 0);
          return;
      }
    
    if (m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
      {
        NS_LOG_UNCOND (GetAddress ()  <<  "channel is already selected " );
        
       if (ppp.GetType () == CHANNELREQ)
        {
           SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
           SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), false);
        }
       else if (ppp.GetType () == CHANNELRESP)
        {
           SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ()); // external_internal_transfer
        }
       else if (ppp.GetType () == CHANNELACK)
        {
            //SendChannelRequest ();
            NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " start channel request " );
            SendChannelRequestPacket (0);
        }
       else if (ppp.GetType () == DATATYPE)
        {
           NS_LOG_UNCOND ("receive data packet");
           Receive (packet);
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
           SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
           NS_LOG_UNCOND (GetAddress ()  <<  " send SendChannelResponse, INTERNAL_SEND_CHANNEL_RESP " ); 
          SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), true);
        }
      else
       {
          NS_ASSERT ("no correct packets or state");
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
               NS_LOG_UNCOND (GetAddress ()  <<  " EXTERNAL_SEND_CHANNEL_ACK,  channel nearly selected, m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx " << m_linkchannelTx );  
               NS_LOG_UNCOND (GetAddress ()  <<  " send SendChannelACK " );  
               SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ());
           }
        }
       else if (ppp.GetType () == CHANNELREQ )
        {
           if ( m_watingChannelRespEvent.IsRunning() )
            {
                NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelRespEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelRespEvent));
                m_watingChannelRespEvent.Cancel ();
            }
           
           if ( m_SendChannelRequestPacketEvent.IsRunning() )
            {
                m_SendChannelRequestPacketEvent.Cancel ();
            }
           SetState (INTERNAL_REC_CHANNEL_REQ);
           SetState (INTERNAL_SEND_CHANNEL_RESP);
           SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
           NS_LOG_UNCOND (GetAddress ()  <<  " send SendChannelResponse " );  
           SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), true);
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
           SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
           NS_LOG_UNCOND (GetAddress ()  <<  " send SendChannelResponse EXTERNAL_SEND_CHANNEL_ACK" ); 
           SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), false);
        }
       else if (ppp.GetType () == CHANNELRESP)
        {
           SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ()); // external_internal_transfer
        }
       else if (ppp.GetType () == CHANNELACK)
        {
           //SendChannelRequest ();
           NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " start channel request " );
           SendChannelRequestPacket (0);
        }
       return;          
    }
    
    
  if (m_state == EXTERNAL_SEND_CHANNEL_RESP )
    {
       NS_LOG_UNCOND (GetAddress ()  <<  "current state EXTERNAL_SEND_CHANNEL_RESP" );  
       if (ppp.GetType () == CHANNELACK)
         {
           SetState (EXTERNAL_REC_CHANNEL_ACK);
           NS_LOG_UNCOND (GetAddress ()  <<  " set EXTERNAL_REC_CHANNEL_ACK, CHANNEL SELECTED, m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx " << m_linkchannelTx );  
           m_channelSelected (Mac48Address::ConvertFrom(GetAddress ()), Simulator::Now (), m_linkchannelRx, m_linkchannelTx);
           m_phyTxBeginTrace (packet);
           Cancel4Events ();
           if (! m_channel->IsChannelUni())
           {
             m_DevSameNode->TryToSetLinkChannelFromInside (m_linkchannelTx, m_linkchannelRx);
           }
          }
       else  if (ppp.GetType () == CHANNELREQ)
        {
             SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
             SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), false);
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
           NS_LOG_UNCOND (GetAddress ()  <<  " INTERNAL_SEND_CHANNEL_REQ,  channel nearly selected, m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx " << m_linkchannelTx );  

           //SendChannelRequest ();
           NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " start channel request " );
           SendChannelRequestPacket (0);
        }
       else if (ppp.GetType () == CHANNELRESP)
        {
           //SetState (EXTERNAL_SEND_CHANNEL_ACK);
           SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ()); // external_internal_transfer
        }
      else if (ppp.GetType () == CHANNELREQ)
        {
            SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
            SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), false);
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
           NS_LOG_UNCOND (GetAddress ()  <<  " set INTERNAL_SEND_CHANNEL_ACK, CHANNEL SELECTED, m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx " << m_linkchannelTx );       
           m_channelSelected (Mac48Address::ConvertFrom(GetAddress ()), Simulator::Now (), m_linkchannelRx, m_linkchannelTx);
           m_phyTxBeginTrace (packet);
           Cancel4Events ();
           if (! m_channel->IsChannelUni())
           {
             m_DevSameNode->TryToSetLinkChannelFromInside (m_linkchannelTx, m_linkchannelRx);
           }
           NS_LOG_UNCOND (GetAddress ()  <<  " send SendChannelACK " );  
           SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ());
        }
       else  if (ppp.GetType () == CHANNELREQ)
        {
             SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
             SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), false);
        }
       else  if (ppp.GetType () == CHANNELACK)
        {
           //SendChannelRequest ();
           NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " start channel request " );
           SendChannelRequestPacket (0);
        }
       return;      
    }
   

    return;
}

void 
PointToPointNetDevice::Cancel4Events ()
{
    
      NS_LOG_UNCOND (GetAddress () << " Cancel4Events, m_watingChannelRespTime " << m_watingChannelRespTime << ", m_watingChannelConfTime " << m_watingChannelConfTime);

            
     if ( m_watingChannelConfEvent.IsRunning ())
             {
               NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelConfEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelConfEvent));
               m_watingChannelConfEvent.Cancel ();
               NS_LOG_UNCOND ("IsExpired " <<  m_watingChannelConfEvent.IsExpired ());  
             }
       NS_LOG_UNCOND ("IsExpired .." );  
     if ( m_watingChannelRespEvent.IsRunning ())
             {
               NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelRespEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelRespEvent));
               m_watingChannelRespEvent.Cancel ();
               NS_LOG_UNCOND ("IsExpired " <<  m_watingChannelRespEvent.IsExpired ());  
             }
       NS_LOG_UNCOND ("IsExpired .." );  
     if ( m_SendChannelResponsePacketEvent.IsRunning() )
            {
             m_SendChannelResponsePacketEvent.Cancel ();
            }
            
     if ( m_SendChannelRequestPacketEvent.IsRunning() )
           {
             m_SendChannelRequestPacketEvent.Cancel ();
           }
              NS_LOG_UNCOND ("IsExpired .." );  

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
PointToPointNetDevice::TryToSetLinkChannelFromInside (uint32_t tx, uint32_t rx)
{
    NS_LOG_UNCOND (GetAddress () << " receive channel command from inside, it uses channel " << tx << " to send, and " << rx << " to receive.");
    SetUsedChannelInside (tx, rx);
    TryToSetLinkChannel ();
}

  
void 
PointToPointNetDevice::TryToSetLinkChannel (void)
{
   NS_LOG_FUNCTION (this);
   NS_LOG_UNCOND (GetAddress () << " TryToSetLinkChannel " );
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
    //NS_LOG_UNCOND (GetAddress () << " SetState " << state);
    m_state = state;
}


void 
PointToPointNetDevice::ChangeTxChannel (void)
{
   uint16_t repetition = 0; 

   while (1)
     {
         m_linkchannelTx++;
         m_linkchannelTx = m_linkchannelTx % CHANNELNUMBER;
         
         if (repetition == CHANNELNUMBER)
          {
            NS_LOG_UNCOND (GetAddress ()  <<  " no available tx channel ...."); 
          }
   
        uint16_t channel= m_linkchannelTx;
        if (channel != m_channel0_usedOutside &&  channel != m_channel1_usedInside && channel != m_linkchannelRx  ) // tx (outside), rx(inside), )
        // channel selection may not be finished due to "channel != m_linkchannelRx"
           {
              m_linkchannelTx = channel;
              NS_LOG_UNCOND (GetAddress ()  <<  " ChangeTxChannel " << m_linkchannelTx); 
              break;
           }
        repetition++;
     } 
}

void 
PointToPointNetDevice::SetUsedChannelInside (uint32_t tx, uint32_t rx)
{
   if (!m_channel->IsChannelUni ())
    {
     m_channel0_usedInside = tx;
     m_channel1_usedInside = rx;
    }
   else
   {
    // to do, if constraint not hold
   }
}

void 
PointToPointNetDevice::SetUsedChannelOutside (uint32_t tx, uint32_t rx)
{
   if (!m_channel->IsChannelUni ())
    {
     m_channel0_usedOutside = tx;
     m_channel1_usedOutside = rx;
    }
   else
   {
       //to do, if constraint not hold
   }
}


void 
PointToPointNetDevice::SendChannelRequestPacket (uint16_t counter)
{
   if (IsLinkUp () == false)
    {
      NS_ASSERT ("there is no channel");  
      return;
    } 
   
   if (m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
    {
        NS_LOG_UNCOND (GetAddress () << " restart SendChannelRequest even selected..., m_watingChannelRespTime " << m_watingChannelRespTime << ", m_watingChannelConfTime " << m_watingChannelConfTime);
        return;
    }
   
    if ( m_SendChannelRequestPacketTime +  Channel_delay_packet * 4 > Simulator::Now()  )
      {
          //NS_LOG_UNCOND (GetAddress ()  <<  " can not start channel req since last one did not finish"); 
          //return;
      }
    
   Ptr<Packet> packet = Create<Packet> ();
   m_type = CHANNELREQ;

           
   if (counter == CHANNELNUMBER) // && m_SendChannelRequestPacketEvent.IsRunning() )
     {
       NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () <<  " stop sending m_SendChannelRequestPacketEvent "); 
       if ( m_SendChannelRequestPacketEvent.IsRunning() )
         {
           m_SendChannelRequestPacketEvent.Cancel ();
         }
       return;
     }
   counter++;
   
   NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " send SendChannelRequestPacket at channel " << m_linkchannelTx << " counter " << counter);
   SendChannelSelection (packet, Mac48Address ("ff:ff:ff:ff:ff:fe"), 0x800); // GetBroadcast (), 0x800 are not really used

   m_SendChannelRequestPacketEvent = Simulator::Schedule (Channel_delay_packet, &PointToPointNetDevice::SendChannelRequestPacket, this, counter);
   m_SendChannelRequestPacketTime =  Simulator::Now ();
}




void 
PointToPointNetDevice::SendChannelResponsePacket (uint16_t counter)
{
   if (IsLinkUp () == false)
    {
      NS_ASSERT ("there is no channel");  
      return;
    } 
   
   if (counter == CHANNELNUMBER) // && m_SendChannelResponsePacketEvent.IsRunning() )
     {
       NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () <<  " stop sending m_SendChannelResponsePacket "); 
       if ( m_SendChannelResponsePacketEvent.IsRunning() )
         {
           m_SendChannelResponsePacketEvent.Cancel ();
         }
       return;
     }
   
   counter++;
   
   Ptr<Packet> packet = Create<Packet> ();

   m_type = CHANNELRESP;
   
   NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " send SendChannelResponsePacket at channel " << m_linkchannelTx << " counter " << counter);
   SendChannelSelection (packet, Mac48Address ("ff:ff:ff:ff:ff:fe"), 0x800); // GetBroadcast (), 0x800 are not really used
   m_SendChannelResponsePacketEvent = Simulator::Schedule (Channel_delay_packet, &PointToPointNetDevice::SendChannelResponsePacket, this, counter);
   m_SendChannelResponsePacketTime =  Simulator::Now ();

   
} 

void 
PointToPointNetDevice::SendChannelRequest (void)
{
    if (m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
    {
        NS_LOG_UNCOND (GetAddress () << " restart SendChannelRequest even selected..., m_watingChannelRespTime " << m_watingChannelRespTime << ", m_watingChannelConfTime " << m_watingChannelConfTime);
        if (m_watingChannelRespEvent.IsRunning()) // why it is still running?
        {
            m_watingChannelRespEvent.Cancel ();
        }
        return;
    }
    if (m_state == EXTERNAL_SEND_CHANNEL_REQ)
    {
            ChangeTxChannel ();
    }
    else
    {
           SetState (EXTERNAL_SEND_CHANNEL_REQ);
           WaitChannel ();
           ChangeTxChannel ();
    }
    //SetState (EXTERNAL_SEND_CHANNEL_REQ);
    //ChangeTxChannel ();
     
    NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " start SendChannelRequestEvent " );
    SendChannelRequestPacket (0);
    uint16_t delay = m_rng->GetInteger (1, backoffcounter);
    Time ChannelResp_delay_total = MilliSeconds(delay) + ChannelResp_delay;
    m_watingChannelRespEvent = Simulator::Schedule (ChannelResp_delay_total, &PointToPointNetDevice::SendChannelRequest, this);   
    m_watingChannelRespTime =  Simulator::Now ();  
}

void 
PointToPointNetDevice::SendChannelResponse (Mac48Address dest, uint16_t channel0, uint16_t channel1, bool timeout)
{
    
   if (m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
    {
       if (timeout)
       {
        NS_LOG_UNCOND (GetAddress () << " restart SendChannelResponse even selected...");
       }
    }
        
   if ( m_SendChannelResponsePacketTime +  Channel_delay_packet * 4 > Simulator::Now())
      {
          //NS_LOG_UNCOND (GetAddress ()  <<  " can not start channel resp since last one did not finish"); 
          //return;
      }
  
   m_destAddress = dest;
   uint16_t repetition = 0;
   
 if ( m_linkchannelTx == CHANNELNOTDEFINED) 
  {
   while (1)
     { 
        if (repetition == CHANNELNUMBER)
          {
            NS_LOG_UNCOND (GetAddress ()  <<  " no available tx resp channel ...."); 
          }
                
        uint32_t channel= m_rng->GetInteger (1, CHANNELNUMBER);
        if (channel != m_channel0_usedOutside &&  channel != m_channel1_usedInside && channel != m_linkchannelRx  ) // tx (outside), rx(inside), )
        // channel selection may not be finished due to "channel != m_linkchannelRx"
           {
              m_linkchannelTx = channel;
              NS_LOG_UNCOND (GetAddress ()  <<  " choose SendChannelResponse channel " << m_linkchannelTx); 
              break;
           }
        repetition++;
     } 
 }
           
    NS_LOG_UNCOND (Simulator::Now() << "\t" << GetAddress () << " start channel resp to " << m_destAddress );
    SendChannelResponsePacket (0);
    uint16_t delay = m_rng->GetInteger (1, backoffcounter);
    Time ChannelConf_delay_total = ChannelConf_delay + MilliSeconds(delay);
   if (timeout)
     {
          m_watingChannelConfEvent = Simulator::Schedule (ChannelConf_delay_total, &PointToPointNetDevice::SendChannelRequest, this);
          m_watingChannelConfTime = Simulator::Now (); 
      }
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
   SendChannelSelection (packet, Mac48Address ("ff:ff:ff:ff:ff:fe"), 0x800); // GetBroadcast (), 0x800 are not really used
}


void 
PointToPointNetDevice::Forward (Ptr<Packet> packet, uint16_t counter)
{
   if (IsLinkUp () == false)
    {
      NS_ASSERT ("there is no channel");  
      return;
    } 
   
  if ( m_linkchannelTx == CHANNELNOTDEFINED) 
   {
    uint16_t repetition = 0;
    while (1)
     { 
        if (repetition == CHANNELNUMBER)
          {
            NS_LOG_UNCOND (GetAddress ()  <<  " no available tx resp channel ...."); 
          }
                
        uint32_t channel= m_rng->GetInteger (1, CHANNELNUMBER);
        if (channel != m_channel0_usedOutside &&  channel != m_channel1_usedInside && channel != m_linkchannelRx  ) 
           {
              m_linkchannelTx = channel;
              NS_LOG_UNCOND (GetAddress ()  <<  " choose SendChannelResponse channel " << m_linkchannelTx); 
              break;
           }
        repetition++;
     }  
   }
   
    if (counter == CHANNELNUMBER) // && m_SendChannelResponsePacketEvent.IsRunning() )
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
PointToPointNetDevice::SendChannelSelection (
  Ptr<Packet> packet, 
  const Address &dest, 
  uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
  NS_LOG_LOGIC ("UID is " << packet->GetUid ());
  
  NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", send to " << dest);
  
  if (m_type ==  DATATYPE )
  {
      NS_LOG_UNCOND (Simulator::Now() <<" drop data packet, " << GetAddress () << ", incorrect data type " );
      return false;  
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
PointToPointNetDevice::Send (
  Ptr<Packet> packet, 
  const Address &dest, 
  uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
  NS_LOG_LOGIC ("UID is " << packet->GetUid ());
  
  NS_LOG_UNCOND (Simulator::Now() <<" \t " << GetAddress () << ", send to " << dest);
  m_type =  DATATYPE;
  
  if ( m_state != EXTERNAL_REC_CHANNEL_ACK && m_state != INTERNAL_SEND_CHANNEL_ACK  )
  {
      NS_LOG_UNCOND (Simulator::Now() <<" drop data packet, " << GetAddress () << ", since channel is not selected "  );
      return false;  
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

void
PointToPointNetDevice::AddDevice (Ptr<PointToPointNetDevice> dev)
{
  m_DevSameNode = dev;
}


} // namespace ns3
