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
#include "point-to-point-net-device-ges.h"
#include "point-to-point-channel.h"
#include "ppp-header.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/spf.h"



#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <fstream>

using namespace std;
namespace ns3 {
    
struct WifiRemoteStationState;
struct WifiRemoteStation;


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
    .AddAttribute ("PACKETREPEATNUMBER", "Number of request, response, and forward packets for channel selection",
                   UintegerValue (4),
                   MakeUintegerAccessor (&PointToPointNetDevice::PACKETREPEATNUMBER),
                   MakeUintegerChecker<uint16_t> ()) 
    .AddAttribute ("Outputpath", "output path to store info of sensors and offload stations",
                   StringValue ("stationfile"),
                   MakeStringAccessor (&PointToPointNetDevice::m_outputpath),
                   MakeStringChecker ()) 
    .AddAttribute ("ARQAckTimeout", 
                   "The time interval for sending packets during channel selection",
                   TimeValue (Seconds (10.0)),
                   MakeTimeAccessor (&PointToPointNetDevice::ARQAckTimer),
                   MakeTimeChecker ()) 
    .AddAttribute ("ARQBufferSize", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (100),
                   MakeUintegerAccessor (&PointToPointNetDevice::BUFFERSIZE),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("m_InterfaceNum", "Total interface number, used for predefined topology",
                   UintegerValue (22),
                   MakeUintegerAccessor (&PointToPointNetDevice::m_InterfaceNum),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LoadBalance", "channel selection is initiated by external command (true) or internal random backoff (false)",
                   BooleanValue (false),
                   MakeBooleanAccessor (&PointToPointNetDevice::m_loadBalance),
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
    .AddAttribute ("TxQueueQos3", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDevice::m_queueQos3),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("TxQueueQos2", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDevice::m_queueQos2),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("TxQueueQos1", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDevice::m_queueQos1),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("TxQueueQos0", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDevice::m_queueQos0),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("ForwardQueue4", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDevice::m_queueForwardQos4),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("ForwardQueue3", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDevice::m_queueForwardQos3),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("ForwardQueue2", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDevice::m_queueForwardQos2),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("ForwardQueue1", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDevice::m_queueForwardQos1),
                   MakePointerChecker<Queue> ())
   .AddAttribute ("ForwardQueue0", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&PointToPointNetDevice::m_queueForwardQos0),
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
    .AddTraceSource ("RecPacketTrace",
                    "Channel has been successfully Selected"
                    "The header of successfully transmitted packet",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_recPacketTrace),
                    "ns3::PointToPointNetDevice::RecPacketCallback")
    .AddTraceSource ("ARQRecPacketTrace",
                    "Channel has been successfully Selected"
                    "The header of successfully transmitted packet",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_ARQrecPacketTrace),
                    "ns3::PointToPointNetDevice::ARQRecPacketCallback")
    .AddTraceSource ("ARQTxPacketTrace",
                    "Channel has been successfully Selected"
                    "The header of successfully transmitted packet",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_ARQTxPacketTrace),
                    "ns3::PointToPointNetDevice::ARQTxPacketCallback")
   .AddTraceSource ("ForwardQueueTrace",
                    "Channel has been successfully Selected"
                    "The header of successfully transmitted packet",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_ForwardQueueTrace),
                    "ns3::PointToPointNetDevice::m_ForwardQueueCallback")
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
    CCmax (100), //ms
    //m_linkchannelRx (0),
    m_state (NO_CHANNEL_CONNECTED), //Initiation
    m_type (DATATYPE),    
    m_channel0_usedInside (CHANNELNOTDEFINED),
    m_channel1_usedInside (CHANNELNOTDEFINED),
    m_channel0_usedOutside (CHANNELNOTDEFINED),
    m_channel1_usedOutside (CHANNELNOTDEFINED),
    PACKETREPEATNUMBER (4),
    selectedTraced (false),
    m_GESSameNodeFlag (false),
    m_packetId (0),
    m_reqid (0),
    m_nextHop0( Mac48Address::ConvertFrom (GetBroadcast() )),
    m_nextHop1(Mac48Address::ConvertFrom (GetBroadcast() )),
    m_topologyCompleted (false),
    m_TopoInitialized (false),
    ARQAckTimer (Seconds(10)),
    ARQ_Packetid (0),
    m_currentPkt (0)
{
  NS_LOG_FUNCTION (this);
  WaitChannel ();
  m_rng = CreateObject<UniformRandomVariable> ();

  uint64_t delay = m_rng->GetInteger (1, CCmax);
  Simulator::Schedule (MilliSeconds (delay), &PointToPointNetDevice::TryToSetLinkChannelExternal, this);
  
  /*
  Ptr<Queue> queueCritical;
  Ptr<Queue> queuehighPri;
  Ptr<Queue> queueBestEff;
  Ptr<Queue> queueBackGround; */
   
  
  

  /*
  m_queueMap.insert (std::make_pair (3, queueCritical));
  m_queueMap.insert (std::make_pair (2, queuehighPri));
  m_queueMap.insert (std::make_pair (1, queueBestEff));
  m_queueMap.insert (std::make_pair (0, queueBackGround));  
   * */
}

PointToPointNetDevice::~PointToPointNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
PointToPointNetDevice::Reset ()
{
    /*
    if (m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK || EXTERNAL_SEND_CHANNEL_ACK)
        {
            if (m_ResetTimeOutEvent.IsRunning())
            {
              m_ResetTimeOutEvent.Cancel();
            }
        }
    else
    {
           SetState (NO_CHANNEL_CONNECTED);
           m_linkchannelTx = 0;
           m_channel0_usedInside = CHANNELNOTDEFINED;
           m_channel1_usedInside = CHANNELNOTDEFINED;
           m_channel0_usedOutside = CHANNELNOTDEFINED;
           m_channel1_usedOutside = CHANNELNOTDEFINED;
           TryToSetLinkChannel (); 
    } */

}
 
void
PointToPointNetDevice::TryToSetLinkChannelExternal ()
{
  NS_LOG_FUNCTION (this);
  if (m_externalChSel)
    {
      std::ofstream myfile;
      std::string dropfile=m_outputpath;
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
  NS_LOG_DEBUG ("m_type " << m_type);
  ppp.SetType (m_type);
  ppp.SetSourceAddre (Mac48Address::ConvertFrom (GetAddress ()));
  ppp.SetDestAddre (m_destAddress);
  ppp.SetQos (m_Qos);
  ppp.SetTTL (0);
  NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () << " add header to " << m_destAddress);

  
  if (m_type == CHANNELACK || m_type == DATAACK || m_type == N_ACK) 
   {
        ppp.SetID (m_ackid);
   }
  else if (m_type == CHANNELRESP) 
   {
        ppp.SetID (m_respid);
   }
  else if (m_type == CHANNELREQ) 
   {
        ppp.SetID (m_reqid);
        m_packetId++;
   }
  else 
   {
      ppp.SetID (m_packetId);
      m_packetId++;
   }

  
  if (m_type == CHANNELREQ)
   {
      if (!m_channel->IsChannelUni())
      {
        ppp.SetUsedChannel0 (m_channel0_usedInside);
        ppp.SetUsedChannel1 (m_channel0_usedInside);
      }
      else
      {
        ppp.SetUsedChannel0 (m_linkchannelRx);
        ppp.SetUsedChannel1 (m_linkchannelTx);
      }
   }
  
  p->AddHeader (ppp);
}


void
PointToPointNetDevice::AddHeaderChannel (Ptr<Packet> p, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << p << protocolNumber);
  PppHeader ppp;
  ppp.SetProtocol (EtherToPpp (protocolNumber));
  NS_LOG_DEBUG ("m_type " << m_type);
  ppp.SetType (m_type);
  ppp.SetSourceAddre (Mac48Address::ConvertFrom (GetAddress ()));
  ppp.SetQos (3);
  //ppp.SetDestAddre (m_destAddress);
  if (m_type == CHANNELACK) 
   {
        ppp.SetDestAddre (m_destAddressAck);
        NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () << " add header to " << m_destAddressAck);
   }
  else if (m_type == CHANNELRESP) 
   {
        ppp.SetDestAddre (m_destAddressResp);
        NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () << " add header to " << m_destAddressResp);
   }
 else if (m_type == LINKSTATE) 
   {
        ppp.SetDestAddre (Mac48Address::ConvertFrom (GetBroadcast() ) );
        NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () << " add header to " << m_destAddressResp);
   }
  else 
   {
      ppp.SetDestAddre (m_destAddress);
      NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () << " add header to " << m_destAddress);
   }

  
  if (m_type == CHANNELACK) 
   {
        ppp.SetID (m_ackid);
   }
  else if (m_type == CHANNELRESP) 
   {
        ppp.SetID (m_respid);
   }
   else if (m_type == CHANNELREQ) 
   {
        ppp.SetID (m_reqid);
        m_packetId++;
   }
  else 
   {
      ppp.SetID (m_packetId);
      m_packetId++;
   }

  
   if (m_type == LINKSTATE) 
   {
        ppp.SetNextHop0 (m_nextHop0 );
        ppp.SetNextHop1 (m_nextHop1 );
   }
  
  if (m_type == CHANNELREQ)
   {
      if (!m_channel->IsChannelUni())
      {
        ppp.SetUsedChannel0 (m_channel0_usedInside);
        ppp.SetUsedChannel1 (m_channel0_usedInside);
      }
      else
      {
        ppp.SetUsedChannel0 (m_linkchannelRx);
        ppp.SetUsedChannel1 (m_linkchannelTx);
      }
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
  m_random = CreateObject<UniformRandomVariable> ();

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
  NS_LOG_DEBUG (GetAddress () << " transmit to  ");

  //forwarding table 
  //enqueue
  
  PppHeader ppp;  
  p->PeekHeader (ppp);
  if (ppp.GetType ()==DATATYPE || ppp.GetType() == DATAACK )
  {
  NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () << " transmit to " << ppp.GetDestAddre () << " transmit id " << ppp.GetID() );
  }



   
  //
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
  //NS_LOG_DEBUG ("SIZE " << p->GetSize () << ", txTime " << txTime);
  Time txCompleteTime = txTime + m_tInterframeGap;
  //
  if (ppp.GetType ()==DATATYPE || ppp.GetType() == DATAACK )
  {
    if (ppp.GetSourceAddre() == Mac48Address::ConvertFrom (GetAddress() ) )//  || ppp.GetSourceAddre () == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()) )
     {
        m_src = true;
     }
    else
     {
         m_src = false;
     }
  
    //m_loadBalance = true;
    if (m_loadBalance && m_src && !m_channel->IsChannelUni ())
     {
        if (m_interface)
         {
              //EnqueueForward (PacketForwardtest);   
                m_interface = false;
                Simulator::Schedule (txCompleteTime, &PointToPointNetDevice::TransmitComplete, this);
                bool result = m_channel->TransmitStart (p, this, txTime, m_linkchannelTx);
                
                NS_LOG_UNCOND("TX \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre()  << "\t" << ppp.GetDestAddre() << "\t"  << ppp.GetID () << "\t" << p->GetSize()  << "\t"  <<  Simulator::Now().GetMilliSeconds()  );
                //NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () << " forward to my queue " << ppp.GetID());
         }
        else
        {
              //EnqueueForward (PacketForwardtest);  
              // NS_LOG_UNCOND (Simulator::Now () << "\t" << GetAddress () << " forward to inside device " << ppp.GetID());
              m_DevSameNode->Forward (p, PACKETREPEATNUMBER-1); 
              m_interface = true;   
              Simulator::Schedule (Seconds(0), &PointToPointNetDevice::TransmitComplete, this);
        }
        return true;
     }
  }
  if (!m_channel->IsChannelUni ())
    {
        if (m_src || ppp.GetSourceAddre () == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()) )
            NS_LOG_UNCOND("TX \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre()  << "\t" << ppp.GetDestAddre() << "\t"  << ppp.GetID () << "\t" << p->GetSize()  << "\t"  <<  Simulator::Now().GetMilliSeconds()  );
  
        else
            NS_LOG_UNCOND("FW \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre()  << "\t" << ppp.GetDestAddre() << "\t"  << ppp.GetID () << "\t" << p->GetSize()  << "\t"  <<  Simulator::Now().GetMilliSeconds()  );
    }
  //

  NS_LOG_LOGIC ("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds () << "sec");
  Simulator::Schedule (txCompleteTime, &PointToPointNetDevice::TransmitComplete, this);
  bool result = m_channel->TransmitStart (p, this, txTime, m_linkchannelTx);
  //NS_LOG_DEBUG (Simulator::Now() <<  "\t" << GetAddress () << " TransmitStart at channel " << m_linkchannelTx);


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

  //m_phyTxEndTrace (m_currentPkt);
  m_currentPkt = 0;

  //Ptr<Packet> p = m_queue->Dequeue (); 
  if (Simulator::Now ().GetSeconds () > 300)
  {
   // NS_LOG_DEBUG (GetAddress () <<" TransmitComplete " << Simulator::Now ());
  }

  //Ptr<Packet> p = PreparePacketToSend ();
  
  Ptr<Packet>  p = DequeueForward ();
  
  if (p == 0)
    {
      //
      // No packet was on the queue, so we just exit.
      //
     //NS_LOG_DEBUG (GetAddress () <<" return TransmitComplete" );
      return;
    }

  //
  // Got another packet off of the queue, so start the transmit process agin.
  //
  m_snifferTrace (p);
  m_promiscSnifferTrace (p);
  NS_LOG_DEBUG ("TransmitComplete");
   //NS_LOG_DEBUG (GetAddress () << " transmit finish, start on " );
  //TransmitStart (p);
  PppHeader ppp;  
  p->PeekHeader (ppp);

  if (ppp.GetType () == DATATYPE || ppp.GetType() == DATAACK )
    {
      //TransmitStartTwoInterface (p);
      if (m_loadBalance)
         TransmitStart (p);
      else
          TransmitStartTwoInterface (p);
          
    }
  else
   {
      TransmitStart (p);
   }
}

bool
PointToPointNetDevice::ForwardDown (void)
{
   PppHeader ppp;
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
          
          //NS_LOG_DEBUG(Simulator::Now() << ", dst: " << dest << " my address:" << GetAddress()  <<  " send--route to " << nextHop << ", packet " << packet->GetSize() << ", protocolNumber " << protocolNumber);
           
          if (nextHop == aid - 1 )
            {
              Ptr<Packet> packetsend = DequeueForward ();
              //NS_LOG_DEBUG(dest << "\t" << GetAddress()  <<  " route to  neighbour " );
             m_DevSameNode->Forward (packetsend, PACKETREPEATNUMBER-1); 
             //goto LinkedDevice;
              return true;
           //return true;
           //return m_DevSameNode->Froward (packet, dest, protocolNumber, true);
           //return m_DevSameNode->SendFromInside (packet, dest, protocolNumber, true);
            }
          else
            {
              goto LinkedDevice;
            }
        }
  }
      
 
     LinkedDevice:
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
          
          return TransmitStart (packetsend);
        }
     return false; 
    
}

void
PointToPointNetDevice::TransmitStartTwoInterface (Ptr<Packet> packet)
{
   if (!m_channel->IsChannelUni ())
    {
          PppHeader ppp;
          packet->PeekHeader (ppp);
          uint16_t nextHop = LookupRoutingTable (ppp.GetDestAddre());
          Mac48Address addr = Mac48Address::ConvertFrom(m_DevSameNode->GetAddress ());
          uint8_t mac[6];
          addr.CopyTo (mac);
          uint8_t aid_l = mac[5];
          uint8_t aid_h = mac[4] & 0x1f;
          uint16_t aid = (aid_h << 8) | (aid_l << 0); 
          
          if (nextHop == aid - 1 )
            {
              NS_LOG_DEBUG (GetAddress () <<"forward to end");
              m_DevSameNode->Forward (packet, PACKETREPEATNUMBER-1); 
              m_txMachineState = BUSY;
              m_currentPkt = packet;
              TransmitComplete ();
            }
          else
            {
              //goto LinkedDevice;
              TransmitStart (packet);
            }
          return;
        
    }
  
LinkedDevice:
          TransmitStart (packet);
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
        NS_LOG_DEBUG(GetAddress () << " set to  m_channel ");
   }
  else if (rx == 1)
   {
        m_channelRx = ch;
        m_channelRx->Attach (this);
        NS_LOG_DEBUG(GetAddress () << " set to  m_channelRx " );
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
PointToPointNetDevice::SetQueue (Ptr<Queue> q, Ptr<Queue> queueCritical, Ptr<Queue> queuehighPri, Ptr<Queue> queueBestEff, Ptr<Queue> queueBackGround, 
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
   NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress ()  <<  " WaitChannel ");   
  uint16_t repetition = 0;  
  while (1)
   {
    if (repetition == CHANNELNUMBER)
     {
        NS_LOG_DEBUG (GetAddress ()  <<  " no available rx channel...... "); 
     }
    m_linkchannelRx++;
    m_linkchannelRx = m_linkchannelRx % CHANNELNUMBER; 
    
    uint16_t channel= m_linkchannelRx;
    if (channel != m_channel0_usedInside  ) // && channel != m_linkchannelTx 
      {
        //NS_LOG_DEBUG (GetAddress ()  <<  " WaitChannel " << m_linkchannelRx); 
        break;
      }
    repetition++;
   }
   m_WaitChannelEvent = Simulator::Schedule (ChannelWaiting_Interval, &PointToPointNetDevice::WaitChannel, this);
}


void
PointToPointNetDevice::setNextHop1 (Mac48Address addr)
{
    m_nextHop1 = addr; // nexthop1 is internal device for bi-
    
    NS_LOG_DEBUG (GetAddress ()  <<  " m_nextHop1 " << m_nextHop1 << ", from samenode " << m_DevSameNode->GetAddress() ); 
}

void
PointToPointNetDevice::ReceiveChannel (Ptr<Packet> packet, uint32_t linkchannel)
{
  //NS_LOG_DEBUG ("add m_phyRxEndTrace " );  

  if (m_channel->IsChannelUni())
    {
      NS_ASSERT (m_channel0_usedInside == CHANNELNOTDEFINED);
      NS_ASSERT (m_channel1_usedInside == CHANNELNOTDEFINED);
      NS_ASSERT (m_channel0_usedOutside == CHANNELNOTDEFINED);
      NS_ASSERT (m_channel1_usedOutside == CHANNELNOTDEFINED);
    }
  //NS_LOG_DEBUG (m_channel0_usedInside << "\t " << m_channel1_usedInside << "\t" << m_channel0_usedOutside << "\t" << m_channel1_usedOutside) ;
  
  
    PppHeader ppp;
    packet->PeekHeader (ppp);
    
     if (ppp.GetSourceAddre () == Mac48Address::ConvertFrom (GetAddress() ))
      {
          NS_LOG_DEBUG ("drop packet due to loop" );
          return;
      }
    
    
      NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () << "...received linkchannel " << linkchannel << ", m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx " << m_linkchannelTx <<
          ", src  " << ppp.GetSourceAddre ());  
      NS_LOG_DEBUG (", nexthop0 " << ppp.GetNextHop0 () << ", nexthop1 " << ppp.GetNextHop1 ());

    
    for (uint16_t i=0; i<100; i++)
    {
        if (ppp.GetType () == i)
        {
            NS_LOG_DEBUG ("ReceiveChannel , type " << i);  
            break;
        }
    }
  
   if (linkchannel != m_linkchannelRx)
     {
        NS_LOG_DEBUG ("packet can not be recevied since sender and receiver are not in the same link channel");  
        return;
     }
   //mimic the real link channel   
   if (m_WaitChannelEvent.IsRunning())
    {
       NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () << "..received linkchannel " << linkchannel << ", m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx " << m_linkchannelTx);  
       m_WaitChannelEvent.Cancel ();
       NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () << "..received linkchannel " << linkchannel << ", m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx " << m_linkchannelTx);  
        
        if (m_linkchannelRx == m_linkchannelTx || m_linkchannelRx == m_channel0_usedInside)
            {
                NS_ASSERT (m_state == NO_CHANNEL_CONNECTED || m_state == EXTERNAL_SEND_CHANNEL_REQ);
                NS_LOG_DEBUG ("restart channel request since tx channel " << m_linkchannelTx << " is used by rx " << m_linkchannelRx << ", or m_channel0_usedInside " << m_channel0_usedInside );  
                //SetState (EXTERNAL_SEND_CHANNEL_REQ);
                //ChangeTxChannel ();
                
           /*SetState (EXTERNAL_SEND_CHANNEL_REQ);
           if (m_WaitChannelEvent.IsRunning())
               m_WaitChannelEvent.Cancel();
            WaitChannel ();
            ChangeTxChannel ();*/
            
            SendChannelRequest ();
            return;
            }
    }
   m_linkchannelRx = linkchannel; // in case m_linkchannelRx changes during Cancelling.

    
    if (ppp.GetType () == CHANNELRESP  && ppp.GetDestAddre () != Mac48Address::ConvertFrom (GetAddress() ) )
      {
          NS_LOG_DEBUG ("Forward , packet " << ppp.GetDestAddre () );  
          Forward (packet, 0);
          return;
      }
   
  
     if (ppp.GetType () == LINKSTATE  && ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetBroadcast() )  )
      {
          if (m_channel->IsChannelUni())
            {
                NS_LOG_DEBUG (GetAddress () << " Forward LINKSTATE packet " << ppp.GetDestAddre () << " from " << ppp.GetSourceAddre ()  );  
                Forward (packet, PACKETREPEATNUMBER-1);
            }
          else
            {
              if (ppp.GetSourceAddre () == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress() ))
              {
                   NS_LOG_DEBUG ("drop packet due to loop bi-" );
                   return;
              }
             
             NS_LOG_DEBUG (Simulator::Now() << "\t" << m_DevSameNode->GetAddress () << " internal update link state of " << ppp.GetSourceAddre () << ", nexthop0 " << ppp.GetNextHop0 () << ", nexthop1 " << ppp.GetNextHop1 ());
             m_DevSameNode->UpdateTopology (ppp.GetSourceAddre (), ppp.GetNextHop0 (), ppp.GetNextHop1 ());
             m_DevSameNode->Forward (packet, PACKETREPEATNUMBER-1);  
            }
      }
   
   if ( ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress() ) && (m_nextHop0 == Mac48Address::ConvertFrom (GetBroadcast() ) || m_nextHop1 == Mac48Address::ConvertFrom (GetBroadcast() ) ) )
    {
       if (m_channel->IsChannelUni())
        {
           if (ppp.GetType () == CHANNELRESP &&  m_nextHop0 == Mac48Address::ConvertFrom (GetBroadcast() ) )
            {
                m_nextHop0 = ppp.GetSourceAddre ();
                NS_LOG_DEBUG ("CHANNELRESP " << GetAddress ()  <<  " m_nextHop0 " << m_nextHop0 << ", m_nextHop1 " << m_nextHop1 ); 

            }
           else if (ppp.GetType () == CHANNELREQ &&  m_nextHop1 == Mac48Address::ConvertFrom (GetBroadcast() ) )
            {
                m_nextHop1 = ppp.GetSourceAddre ();
                NS_LOG_DEBUG ("CHANNELREQ " << GetAddress ()  <<  " m_nextHop0 " << m_nextHop0 << ", m_nextHop1 " << m_nextHop1 ); 
            }
        }
       else
        {
           if (m_nextHop0 == Mac48Address::ConvertFrom (GetBroadcast() ) )
            {
                m_nextHop0 = ppp.GetSourceAddre ();
                m_DevSameNode->setNextHop1( Mac48Address::ConvertFrom(GetAddress ()) );
                //NS_LOG_DEBUG (GetAddress ()  <<  " m_nextHop0 " << m_nextHop0 << ", m_nextHop1 " << m_nextHop1 ); 
            }
        }
    }

    
    if (m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
      {
        NS_LOG_DEBUG (GetAddress ()  <<  "channel is already selected " );
        
       if (ppp.GetType () == CHANNELREQ)
        {
           if ( !m_channel->IsChannelUni())
             {
                SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
             }
           else
             {
                //SetUsedChannelOutside (); //uni
             }
           SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), false, ppp.GetID ());
        }
       else if (ppp.GetType () == CHANNELRESP)
        {
           SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ()); // external_internal_transfer
        }
       else if (ppp.GetType () == CHANNELACK)
        {
            //SendChannelRequest ();
            NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " start channel request " );
            SendChannelRequestPacket (0);
        }
       else if (ppp.GetType () == LINKSTATE)
        {
            //SendChannelRequest ();
            NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " update link state of " << ppp.GetSourceAddre () << ", nexthop0 " << ppp.GetNextHop0 () << ", nexthop1 " << ppp.GetNextHop1 ());
            UpdateTopology (ppp.GetSourceAddre (), ppp.GetNextHop0 (), ppp.GetNextHop1 ());
        }
       else if (ppp.GetType () == DATATYPE || ppp.GetType () == DATAACK || ppp.GetType () == N_ACK)
        {
           NS_LOG_DEBUG ("receive data packet");
           Receive (packet);
        }
        return;
      } 

   
   if (m_state == NO_CHANNEL_CONNECTED)
   {
      NS_LOG_DEBUG (GetAddress ()  <<  " state NO_CHANNEL_CONNECTED" );  
      if (ppp.GetType () == CHANNELREQ )
        {
           SetState (INTERNAL_REC_CHANNEL_REQ);
           SetState (INTERNAL_SEND_CHANNEL_RESP);
           SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
           NS_LOG_DEBUG (GetAddress ()  <<  " send SendChannelResponse, INTERNAL_SEND_CHANNEL_RESP " ); 
          SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), true, ppp.GetID ());
        }
      else
       {
          NS_ASSERT ("no correct packets or state");
       }
            
       return;
   }
       
       
   if (m_state == EXTERNAL_SEND_CHANNEL_REQ )
    {
       NS_LOG_DEBUG (GetAddress ()  <<  "current state EXTERNAL_SEND_CHANNEL_REQ" );  
       if (ppp.GetType () == CHANNELRESP && m_reqid == ppp.GetID() )
        {

          if (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress()) )
           {
               SetState (EXTERNAL_REC_CHANNEL_RESP);
               SetState (EXTERNAL_SEND_CHANNEL_ACK);
               NS_LOG_DEBUG (GetAddress ()  <<  " EXTERNAL_SEND_CHANNEL_ACK,  channel nearly selected, m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx " << m_linkchannelTx );  
               NS_LOG_DEBUG (GetAddress ()  <<  " send SendChannelACK " );  
               
               SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ());
           }
        }
       else if (ppp.GetType () == CHANNELREQ )
        {
           if ( m_watingChannelRespEvent.IsRunning() )
            {
                NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelRespEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelRespEvent));
                m_watingChannelRespEvent.Cancel ();
            }
           
           if ( m_SendChannelRequestPacketEvent.IsRunning() )
            {
                m_SendChannelRequestPacketEvent.Cancel ();
            }
           SetState (INTERNAL_REC_CHANNEL_REQ);
           SetState (INTERNAL_SEND_CHANNEL_RESP);
           SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
           NS_LOG_DEBUG (GetAddress ()  <<  " send SendChannelResponse " );  
           SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), true, ppp.GetID ());
        } 
      return;
    }
    
    
   if (m_state == EXTERNAL_SEND_CHANNEL_ACK )
    {
       NS_LOG_DEBUG (GetAddress ()  <<  "current state EXTERNAL_SEND_CHANNEL_ACK" );  
       if (ppp.GetType () == CHANNELREQ)
        {
           SetState (EXTERNAL_REC_CHANNEL_REQ);
           SetState (EXTERNAL_SEND_CHANNEL_RESP);
           SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
           NS_LOG_DEBUG (GetAddress ()  <<  " send SendChannelResponse EXTERNAL_SEND_CHANNEL_ACK" ); 
           SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), false, ppp.GetID ());
        }
       else if (ppp.GetType () == CHANNELRESP && m_reqid == ppp.GetID())
        {
           SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ()); // external_internal_transfer
        }
       else if (ppp.GetType () == CHANNELACK && m_respid == ppp.GetID())
        {
           //SendChannelRequest ();
           NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " start channel request " );
           SendChannelRequestPacket (0);
        }
      else if (ppp.GetType () == LINKSTATE)
        {
            //SendChannelRequest ();
            NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " update link state of " << ppp.GetSourceAddre () << ", nexthop0 " << ppp.GetNextHop0 ());
            UpdateTopology (ppp.GetSourceAddre (), ppp.GetNextHop0 (), ppp.GetNextHop1 ());
        }
      else if (ppp.GetType () == DATATYPE || ppp.GetType () == DATAACK || ppp.GetType () == N_ACK)
        {
           NS_LOG_DEBUG ("receive data packet");
           Receive (packet);
        }
       return;          
    }
    
    
  if (m_state == EXTERNAL_SEND_CHANNEL_RESP )
    {
       NS_LOG_DEBUG (GetAddress ()  <<  "current state EXTERNAL_SEND_CHANNEL_RESP" );  
       if (ppp.GetType () == CHANNELACK && m_respid == ppp.GetID())
         {
           SetState (EXTERNAL_REC_CHANNEL_ACK);
           NS_LOG_DEBUG (GetAddress ()  <<  " set EXTERNAL_REC_CHANNEL_ACK, CHANNEL SELECTED, m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx " << m_linkchannelTx );  
           if (!selectedTraced)
            {
                m_channelSelected (Mac48Address::ConvertFrom(GetAddress ()), Simulator::Now (), m_linkchannelRx, m_linkchannelTx);
                selectedTraced = true;
                CreateRoutingTable ();
                NS_LOG_DEBUG (GetAddress ()  <<  " m_nextHop0 " << m_nextHop0 << ", m_nextHop1 " << m_nextHop1 ); 
            }
           Cancel4Events ();
           if (! m_channel->IsChannelUni())
           {
             m_DevSameNode->TryToSetLinkChannelFromInside (m_linkchannelTx, m_linkchannelRx);
           }
          }
       else  if (ppp.GetType () == CHANNELREQ)
        {
             SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
             SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), false, ppp.GetID ());
        }
       else  if (ppp.GetType () == CHANNELRESP && m_reqid == ppp.GetID())
        {
              SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ()); // external_internal_transfer
        }
      return;       
    }
    
  if (m_state == INTERNAL_SEND_CHANNEL_RESP )
    {
       NS_LOG_DEBUG (GetAddress ()  <<  "current state INTERNAL_SEND_CHANNEL_RESP" );        
       if (ppp.GetType () == CHANNELACK && m_respid == ppp.GetID())
        {
           SetState (INTERNAL_REC_CHANNEL_ACK);
           SetState (INTERNAL_SEND_CHANNEL_REQ);
           NS_LOG_DEBUG (GetAddress ()  <<  " INTERNAL_SEND_CHANNEL_REQ,  channel nearly selected, m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx " << m_linkchannelTx );  

           //SendChannelRequest ();
           NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " start channel request " );
           SendChannelRequestPacket (0);
        }
       else if (ppp.GetType () == CHANNELRESP && m_reqid == ppp.GetID())
        {
           //SetState (EXTERNAL_SEND_CHANNEL_ACK);
           SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ()); // external_internal_transfer
        }
      else if (ppp.GetType () == CHANNELREQ)
        {
            SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
            SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), false, ppp.GetID ());
        }
      return;      
    }
    
   if (m_state == INTERNAL_SEND_CHANNEL_REQ )
    {
       NS_LOG_DEBUG (GetAddress ()  <<  "current state  INTERNAL_SEND_CHANNEL_REQ" );        
       if (ppp.GetType () == CHANNELRESP && m_reqid == ppp.GetID())
        {
           SetState (INTERNAL_REC_CHANNEL_RESP);
           SetState (INTERNAL_SEND_CHANNEL_ACK);
           NS_LOG_DEBUG (GetAddress ()  <<  " set INTERNAL_SEND_CHANNEL_ACK, CHANNEL SELECTED, m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx " << m_linkchannelTx ); 
           if (!selectedTraced)
            {
                m_channelSelected (Mac48Address::ConvertFrom(GetAddress ()), Simulator::Now (), m_linkchannelRx, m_linkchannelTx);
                selectedTraced = true;
                CreateRoutingTable ();
                NS_LOG_DEBUG (GetAddress ()  <<  " m_nextHop0 " << m_nextHop0 << ", m_nextHop1 " << m_nextHop1 ); 
            }
           Cancel4Events ();
           if (! m_channel->IsChannelUni())
           {
             m_DevSameNode->TryToSetLinkChannelFromInside (m_linkchannelTx, m_linkchannelRx);
           }
           NS_LOG_DEBUG (GetAddress ()  <<  " send SendChannelACK " );  
           SendChannelACK (ppp.GetSourceAddre (), ppp.GetID ());
        }
       else  if (ppp.GetType () == CHANNELREQ)
        {
             SetUsedChannelOutside (ppp.GetUsedChannel0 (), ppp.GetUsedChannel1());
             SendChannelResponse (ppp.GetSourceAddre (), ppp.GetUsedChannel0 (), ppp.GetUsedChannel1(), false, ppp.GetID ());
        }
       else  if (ppp.GetType () == CHANNELACK && m_respid == ppp.GetID())
        {
           //SendChannelRequest ();
           NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " start channel request " );
           SendChannelRequestPacket (0);
        }
       return;      
    }
   

    return;
}

void 
PointToPointNetDevice::Cancel4Events ()
{
    
      NS_LOG_DEBUG (GetAddress () << " Cancel4Events, m_watingChannelRespTime " << m_watingChannelRespTime << ", m_watingChannelConfTime " << m_watingChannelConfTime);

            
     if ( m_watingChannelConfEvent.IsRunning ())
             {
               NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelConfEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelConfEvent));
               m_watingChannelConfEvent.Cancel ();
               NS_LOG_DEBUG ("IsExpired " <<  m_watingChannelConfEvent.IsExpired ());  
             }
       NS_LOG_DEBUG ("IsExpired .." );  
     if ( m_watingChannelRespEvent.IsRunning ())
             {
               NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " m_watingChannelRespEvent cancel, need  " << Simulator::GetDelayLeft (m_watingChannelRespEvent));
               m_watingChannelRespEvent.Cancel ();
               NS_LOG_DEBUG ("IsExpired " <<  m_watingChannelRespEvent.IsExpired ());  
             }
       NS_LOG_DEBUG ("IsExpired .." );  
     if ( m_SendChannelResponsePacketEvent.IsRunning() )
            {
             m_SendChannelResponsePacketEvent.Cancel ();
            }
            
     if ( m_SendChannelRequestPacketEvent.IsRunning() )
           {
             m_SendChannelRequestPacketEvent.Cancel ();
           }
              NS_LOG_DEBUG ("IsExpired .." );  

}

/*
void
PointToPointNetDevice::Receive (Ptr<Packet> packet)
{
  NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " receive ......size "  <<  packet->GetSize() );
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
      PppHeader ppp;
         packet->PeekHeader (ppp);
             
       
  
      ProcessHeader (packet, protocol);
      
      NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " receive ......size "  <<  packet->GetSize() << ",  protocol " << protocol );
         
       NS_LOG_DEBUG(Simulator::Now() << "\t" << ", myaddress" << GetAddress () << ", packet src "  <<ppp.GetSourceAddre() << "\t" << ppp.GetDestAddre ()  << " receive ......size "  <<  packet->GetSize() << ",  protocol " << protocol );

     if (ppp.GetSourceAddre () == Mac48Address::ConvertFrom (GetAddress() ))
      {
          NS_LOG_DEBUG ("drop packet due to loop" );
          return;
      }
       
      if (!m_promiscCallback.IsNull ())
        {
          m_macPromiscRxTrace (originalPacket);
          m_promiscCallback (this, packet, protocol, GetRemote (), GetAddress (), NetDevice::PACKET_HOST);
          NS_LOG_DEBUG (".." << protocol << "\t" << GetRemote () << "\t" << GetAddress () );
        }

      m_macRxTrace (originalPacket);
      if (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress() ) || ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff"))
      {
      m_rxCallback (this, packet, protocol, GetRemote ());
      } // Should the two condition be separated? if the packet is for me, there should be no forward
      
      if (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress() ))
      {
      m_Qos = ppp.GetQos ();
      NS_LOG_DEBUG ("receive data m_Qos " << uint16_t (m_Qos));
      }
      //NS_LOG_DEBUG ( GetAddress () << "\t" << m_address );

            NS_LOG_DEBUG(Simulator::Now() << "\t" << ", 2 myaddress" << GetAddress () << ", packet src "  <<ppp.GetSourceAddre() << "\t" << ppp.GetDestAddre ()  << " receive ......size "  <<  packet->GetSize() << ",  protocol " << protocol );
 
      if (m_channel->IsChannelUni () )
      {
        if (ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff") && ppp.GetDestAddre () != Mac48Address::ConvertFrom (m_GESSameNode->GetAddress() ) )
        {
            m_GESSameNode->m_rxCallback (this, packet, protocol, GetRemote ());
        }    
      
           
        if ( ppp.GetDestAddre () != Mac48Address::ConvertFrom (GetAddress() ))
        {
                            NS_LOG_DEBUG(", myaddress ...." << GetAddress () << ", packet src "  <<ppp.GetSourceAddre() << "\t" << ppp.GetDestAddre ()  << " receive ......size "  <<  packet->GetSize() << ",  protocol " << protocol );

         uint16_t nextHop = LookupRoutingTable (ppp.GetDestAddre ());
                NS_LOG_DEBUG(", myaddress" << GetAddress () << ", packet src "  <<ppp.GetSourceAddre() << "\t" << ppp.GetDestAddre ()  << " receive ......size "  <<  packet->GetSize() << ",  protocol " << protocol );

         //no need to use nexthop
         //NS_LOG_DEBUG(ppp.GetDestAddre () << "\t" << GetAddress()  <<  " route to " << nextHop  );
         m_destAddress = ppp.GetDestAddre ();
         //Send (packet, m_destAddress, protocol); // GetBroadcast (), 0x800 are not really used
         
         //Forward (originalPacket, PACKETREPEATNUMBER-1);
         
         //
         
            uint8_t mac[6];
            m_nextHop0.CopyTo (mac);
            uint8_t aid_l = mac[5];
            uint8_t aid_h = mac[4] & 0x1f;
            uint16_t aid = (aid_h << 8) | (aid_l << 0); 
    
         if (aid !=  nextHop)
            {
               // (originalPacket, PACKETREPEATNUMBER-1); // allow ges device to send
               m_GESSameNode->Forward (originalPacket);
            }
         else
            {
                Forward (originalPacket, PACKETREPEATNUMBER-1);
            }
       }
      }
      else
      {
          if ( ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetBroadcast() ))
            {
              NS_LOG_DEBUG("broadcast from outside, transfer to inside");
              m_destAddress = ppp.GetDestAddre ();

              if ( ppp.GetSourceAddre () == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()) )
                {
                  return;
                }
               if (m_destAddress == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress())  || m_destAddress == Mac48Address::ConvertFrom (GetBroadcast ()))
                {
                       NS_LOG_DEBUG ("sent to upper layer" );
                       m_DevSameNode->m_rxCallback (m_DevSameNode, packet, protocol, GetRemote ());
                }
                if ( m_destAddress == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()) )
                {
                  return;
                }
              //m_DevSameNode->SendFromInside (packet, m_destAddress, protocol, false); 
              m_DevSameNode->Forward (originalPacket, PACKETREPEATNUMBER-1);

            }
          else if ( ppp.GetDestAddre () != Mac48Address::ConvertFrom (GetAddress() ))
           {
               NS_LOG_DEBUG("no broadcast from outside, transfer to inside");
               uint16_t nextHop = LookupRoutingTable (ppp.GetDestAddre ());
               NS_LOG_DEBUG(ppp.GetDestAddre () << "\t" << GetAddress()  <<  " route to " << nextHop  );
               m_destAddress = ppp.GetDestAddre ();
               if ( ppp.GetSourceAddre () == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()) )
                {
                  return;
                }
               if (m_destAddress == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress())  || m_destAddress == Mac48Address::ConvertFrom (GetBroadcast ()))
                {
                       NS_LOG_DEBUG ("sent to upper layer" );
                       m_DevSameNode->m_rxCallback (m_DevSameNode, packet, protocol, GetRemote ());
                }
                if ( m_destAddress == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()) )
                {
                  return;
                }
               //m_DevSameNode->SendFromInside (packet, m_destAddress, protocol, false);
               m_DevSameNode->Forward (originalPacket, PACKETREPEATNUMBER-1);
               //allow the other device to transmit, no need to  route
           }
      }
    }
}
*/


void
PointToPointNetDevice::Receive (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  uint16_t protocol = 0;
  
  //double correctrate = m_random->GetValue (0,1);
  double correctrate = (float) random() / (RAND_MAX);

  if (correctrate < 1e-6 )
  {
      NS_LOG_DEBUG (GetAddress ()  << " drop packet " << correctrate);
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
        if (m_GESSameNodeFlag  )
            {
                //NS_LOG_DEBUG (GetAddress ( ) << " has GES " << m_GESSameNode->GetAddress());

            }

      //
      // Strip off the point-to-point protocol header and forward this packet
      // up the protocol stack.  Since this is a simple point-to-point link,
      // there is no difference in what the promisc callback sees and what the
      // normal receive callback sees.
      //
      PppHeader ppp;
      packet->PeekHeader (ppp);
      uint8_t ttl = ppp.GetTTL ();
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
      
      Ptr<Packet> originalPacket = packet->Copy ();
      Ptr<Packet> originalPacket2 = packet->Copy ();
      Ptr<Packet> originalPacket3 = packet->Copy ();
      Ptr<Packet> LEOPacket = packet->Copy ();
             
       
  
      ProcessHeader (packet, protocol);
      Ptr<Packet> packet2 = packet->Copy ();

               
      //NS_LOG_UNCOND(Simulator::Now() << "\t" << ", address" << GetAddress () << ", packet src "  <<ppp.GetSourceAddre() << "\t" << ppp.GetDestAddre ()  << " receive ......size "  <<  packet->GetSize() << ",  protocol " << protocol << ",  id " << ppp.GetID() );
     if (ppp.GetSourceAddre () == Mac48Address::ConvertFrom (GetAddress() ))
      {
          NS_LOG_DEBUG ("drop packet due to loop" );
          return;
      }
       
      if (!m_promiscCallback.IsNull ())
        {
          m_macPromiscRxTrace (originalPacket);
          m_promiscCallback (this, packet, protocol, GetRemote (), GetAddress (), NetDevice::PACKET_HOST);
          NS_LOG_DEBUG (".." << protocol << "\t" << GetRemote () << "\t" << GetAddress () );
        }

      m_macRxTrace (originalPacket);
      
      if (ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff") )
        {
          Ptr<Packet> ARQRxPacket = packet->Copy ();
          m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , ppp.GetQos (), ppp.GetID ());
          m_rxCallback (this, packet, protocol, GetRemote ());
        }
      
     if  (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress() ) )
      {
        if (ppp.GetType () == DATATYPE )
          {
            Ptr<Packet> ARQRxPacket = packet->Copy ();
            //NS_LOG_UNCOND(Simulator::Now() << "\t"  << GetAddress () << " receives packet from mine "  <<ppp.GetSourceAddre() << " to " << ppp.GetDestAddre ()  << ", id "  <<  ppp.GetID () << " mine");
            //NS_LOG_UNCOND("RX \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre() << "\t"  << ppp.GetID () << "\t" << packet->GetSize()  << "\t"  <<  Simulator::Now().GetMicroSeconds()  );
            NS_LOG_UNCOND("RX \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre()  << "\t" << ppp.GetDestAddre() << "\t"  << ppp.GetID () << "\t" << packet->GetSize()  << "\t"  <<  Simulator::Now().GetMilliSeconds()  );
            m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , ppp.GetQos (), ppp.GetID ());
            ARQReceive (ppp.GetSourceAddre(), ppp.GetDestAddre(), Simulator::Now (), packet->GetSize() ,  ppp.GetQos (), ppp.GetID (), ARQRxPacket);

            m_rxCallback (this, packet, protocol, GetRemote ());
            Ptr<Packet> SpiderPacket = packet->Copy ();
          //m_spiderrxCallback (this, SpiderPacket, protocol, GetRemote ());
          }
        else if (ppp.GetType () == DATAACK  )
          {
            ARQACKRecevie (ppp.GetSourceAddre(), ppp.GetID ());  
          }
       else if ( ppp.GetType () == N_ACK)
         {
           ARQN_ACKRecevie (ppp.GetSourceAddre(), ppp.GetID () );   
         }
        return;
      }
      
     if (!m_channel->IsChannelUni () )
     {
      if (ppp.GetDestAddre () == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()))
      {
          if (ppp.GetType () == DATATYPE )
          {
                Ptr<Packet> ARQRxPacket = packet->Copy ();
                //NS_LOG_UNCOND(Simulator::Now() << "\t"  << GetAddress () << " receives packet from neig "  <<ppp.GetSourceAddre() << " to " << ppp.GetDestAddre ()  << ", id "  <<  ppp.GetID () << " neigh"  );
                //NS_LOG_UNCOND("RX \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre() << "\t"  << ppp.GetID () << "\t" << packet->GetSize()  << "\t"  <<  Simulator::Now().GetMicroSeconds()  );
                NS_LOG_UNCOND("RX \t" << Mac48Address::ConvertFrom(GetAddress ()) << "\t"  << ppp.GetSourceAddre()  << "\t" << ppp.GetDestAddre() << "\t"  << ppp.GetID () << "\t" << packet->GetSize()  << "\t"  <<  Simulator::Now().GetMilliSeconds()  );
                //m_DevSameNode->m_recPacketTrace (Mac48Address::ConvertFrom(GetAddress ()), ppp.GetSourceAddre(), ppp.GetDestAddre (), Simulator::Now (), packet->GetSize() , ppp.GetQos (), ppp.GetID ());
                m_DevSameNode->ARQReceive (ppp.GetSourceAddre(), ppp.GetDestAddre(), Simulator::Now (), packet->GetSize() ,  ppp.GetQos (), ppp.GetID (), ARQRxPacket);
          }
         else if (ppp.GetType () == DATAACK  )
         {
                m_DevSameNode->ARQACKRecevie (ppp.GetSourceAddre(), ppp.GetID ());  
         }
        else if ( ppp.GetType () == N_ACK && ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress() ))
         {
               m_DevSameNode->ARQN_ACKRecevie (ppp.GetSourceAddre(), ppp.GetID () );   
         }
         return;
      }
     }
      
      
      if (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress() ))
      {
      m_Qos = ppp.GetQos ();
      }

       //NS_LOG_DEBUG(Simulator::Now() << "\t" << ", 2 myaddress..." << GetAddress () << ", packet src "  <<ppp.GetSourceAddre() << "\t" << ppp.GetDestAddre ()  << " receive ......size "  <<  packet->GetSize() << ",  protocol " << protocol );

      if (m_channel->IsChannelUni () )
      {
        if (ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff")  )  
            {
           //     m_GESSameNode->Forward (originalPacket);
                Forward (originalPacket2, PACKETREPEATNUMBER-1);
            }
        else if ( ppp.GetDestAddre () != Mac48Address::ConvertFrom (GetAddress() ))
            {
               Forward (originalPacket2, PACKETREPEATNUMBER-1);
            }
      }
     else
      {
          if ( ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetBroadcast() ))
            {
              NS_LOG_DEBUG("broadcast from outside, transfer to inside");
              m_destAddress = ppp.GetDestAddre ();

              if ( ppp.GetSourceAddre () == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()) )
                {
                  return;
                }
               if (m_destAddress == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress())  || m_destAddress == Mac48Address::ConvertFrom (GetBroadcast ()))
                {
                       NS_LOG_DEBUG ("sent to upper layer" );
                       m_DevSameNode->m_rxCallback (m_DevSameNode, packet, protocol, GetRemote ());
                       m_DevSameNode->Forward (originalPacket, PACKETREPEATNUMBER-1);
                }
              
              if (m_GESSameNodeFlag &&  (m_destAddress == Mac48Address::ConvertFrom (m_GESSameNode->GetAddress())  || m_destAddress == Mac48Address::ConvertFrom (GetBroadcast ()) ) )
               {
                   //m_GESSameNode->m_rxCallback (m_DevSameNode, packet, protocol, GetRemote ());
                   //m_GESSameNode->Forward (originalPacket);

               }
              return;
            }
          else if ( ppp.GetDestAddre () != Mac48Address::ConvertFrom (GetAddress() ))
           {
               NS_LOG_DEBUG("no broadcast from outside, transfer to inside");
               m_destAddress = ppp.GetDestAddre ();
               
               if ( ppp.GetSourceAddre () == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()) )
                {
                  return;
                }
               
               if (m_destAddress == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress())  )
                {
                       NS_LOG_DEBUG ("sent to upper layer" );
                       m_DevSameNode->m_rxCallback (m_DevSameNode, packet, protocol, GetRemote ());
                       return;
                }
               else if (m_GESSameNodeFlag && m_destAddress == Mac48Address::ConvertFrom (m_GESSameNode->GetAddress()) )
                {
                   //m_GESSameNode->m_rxCallback (m_DevSameNode, packet, protocol, GetRemote ());
                   return;
                }
               else
                {
                    uint16_t nextHop = m_DevSameNode->LookupRoutingTable (ppp.GetDestAddre ());
                    NS_LOG_DEBUG("dst " << ppp.GetDestAddre () << ", my address " << GetAddress()  <<  " aaroute to " << nextHop  );
                    
                    uint8_t mac[6];
                    m_DevSameNode->m_nextHop0.CopyTo (mac);
                    uint8_t aid_l = mac[5];
                    uint8_t aid_h = mac[4] & 0x1f;
                    uint16_t aid = (aid_h << 8) | (aid_l << 0); 
                    
                    
                     NS_LOG_DEBUG("dst " << ppp.GetDestAddre () << ", my address " << GetAddress()  <<  " aaroute to " << nextHop << ", my next hop " << aid - 1 );
    
                    if ((aid - 1) !=  nextHop)
                       {
                          //m_GESSameNode->Forward (originalPacket);
                        //NS_FATAL_ERROR ("SHOULD NOT HAPPEN");
                       }
                    else
                       {
                          NS_LOG_DEBUG("my neighbor " << m_DevSameNode->GetAddress()  <<  " forward packets ");
                          //m_DevSameNode->Forward (originalPacket, PACKETREPEATNUMBER-1);
                       } 
                     //NS_LOG_UNCOND("my neighbor " << m_DevSameNode->GetAddress()  <<  " forward packets ");
                     m_DevSameNode->Forward (originalPacket, PACKETREPEATNUMBER-1);

                }

           }
      }
    }
}
 

void
PointToPointNetDevice::ReceiveFromGES (Ptr<Packet> packet)
{
    
 if ( m_state != EXTERNAL_REC_CHANNEL_ACK && m_state != INTERNAL_SEND_CHANNEL_ACK && m_state != EXTERNAL_SEND_CHANNEL_ACK )
  {
      NS_LOG_DEBUG (Simulator::Now() <<" drop data packet, " << GetAddress () << ", since channel is not selected "  );
      return;  
  }
      
  NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " receive from GES......size "  <<  packet->GetSize() );
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

      //
      // Strip off the point-to-point protocol header and forward this packet
      // up the protocol stack.  Since this is a simple point-to-point link,
      // there is no difference in what the promisc callback sees and what the
      // normal receive callback sees.
      //
      PppHeader ppp;
      packet->PeekHeader (ppp);
      
      uint8_t ttl = ppp.GetTTL ();
      if (ttl > TTLmax)
        {
          NS_LOG_DEBUG ("PACKET DROPPED DUE TO TT--GESL ");
          return;
        }
      else
        {
            PppHeader newppp;
            packet->RemoveHeader (newppp);
            newppp.SetTTL (ttl + 1);
            packet->AddHeader (newppp);
        }
             
      Ptr<Packet> originalPacket = packet->Copy ();

  
      ProcessHeader (packet, protocol);
      
         
       NS_LOG_DEBUG(Simulator::Now() << "\t" << ", ReceiveFromGES myaddress" << GetAddress () << ", packet src "  <<ppp.GetSourceAddre() << "\t" << ppp.GetDestAddre ()  << " receive ......size GES "  <<  packet->GetSize() << ",  protocol " << protocol << ", id " << ppp.GetID () );

     if (ppp.GetSourceAddre () == Mac48Address::ConvertFrom (GetAddress() ))
      {
          NS_ASSERT ("drop packet due to loop" );
          return;
      }
       
      if (!m_promiscCallback.IsNull ())
        {
          m_macPromiscRxTrace (originalPacket);
          m_promiscCallback (this, packet, protocol, GetRemote (), GetAddress (), NetDevice::PACKET_HOST);
          NS_LOG_DEBUG (".." << protocol << "\t" << GetRemote () << "\t" << GetAddress () );
        }

      m_macRxTrace (originalPacket);
      if (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress() ) || ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff"))
      {
        m_rxCallback (this, packet, protocol, GetRemote ());
        Ptr<Packet> SpiderPacket = packet->Copy ();
        //m_spiderrxCallback (this, SpiderPacket, protocol, GetRemote ());
      } // Should the two condition be separated? if the packet is for me, there should be no forward
      
      if (ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetAddress() ))
      {
        m_Qos = ppp.GetQos ();
        NS_LOG_DEBUG ("receive data m_Qos " << uint16_t (m_Qos));
      }
      //NS_LOG_DEBUG ( GetAddress () << "\t" << m_address );

      NS_LOG_DEBUG(Simulator::Now() << "\t" << ", 2 myaddress" << GetAddress () << ", packet src "  <<ppp.GetSourceAddre() << "\t" << ppp.GetDestAddre ()  << " receive ......size  GES"  <<  packet->GetSize() << ",  protocol " << protocol );
 
      if (m_channel->IsChannelUni () )
      {          
        if ( ppp.GetDestAddre () != Mac48Address::ConvertFrom (GetAddress() ))
        {
                Forward (originalPacket, PACKETREPEATNUMBER-1);
        }
      }
      else
      {
          if ( ppp.GetDestAddre () == Mac48Address::ConvertFrom (GetBroadcast() ))
            {
              NS_LOG_DEBUG("broadcast from outside, transfer to inside");
              m_destAddress = ppp.GetDestAddre ();

              if ( ppp.GetSourceAddre () == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()) )
                {
                  return;
                }
      
                NS_LOG_DEBUG ("sent to upper layer" );
                m_DevSameNode->m_rxCallback (m_DevSameNode, packet, protocol, GetRemote ());
                
                if ( m_destAddress == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()) )
                {
                  return;
                }
              //m_DevSameNode->SendFromInside (packet, m_destAddress, protocol, false); 
              m_DevSameNode->Forward (originalPacket, PACKETREPEATNUMBER-1);
            }
          else if ( ppp.GetDestAddre () != Mac48Address::ConvertFrom (GetAddress() ))
           {
               NS_LOG_DEBUG("no broadcast from outside, transfer to inside");
             
               m_destAddress = ppp.GetDestAddre ();
               if ( ppp.GetSourceAddre () == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()) )
                {
                  return;
                }
               if (m_destAddress == Mac48Address::ConvertFrom (m_DevSameNode->GetAddress()) )
                {
                       NS_LOG_DEBUG ("sent to upper layer" );
                       m_DevSameNode->m_rxCallback (m_DevSameNode, packet, protocol, GetRemote ());
                       return;
                }
               else
               {     
                    uint16_t nextHop = LookupRoutingTable (ppp.GetDestAddre ());
                    NS_LOG_DEBUG(ppp.GetDestAddre () << "\t" << GetAddress()  <<  " route to " << nextHop  );
                   
                    uint8_t mac[6];
                    m_nextHop0.CopyTo (mac);
                    uint8_t aid_l = mac[5];
                    uint8_t aid_h = mac[4] & 0x1f;
                    uint16_t aid = (aid_h << 8) | (aid_l << 0); 
    
                    if (aid !=  nextHop)
                       {
                          // (originalPacket, PACKETREPEATNUMBER-1); // allow ges device to send
                                       NS_LOG_DEBUG(GetAddress () <<" no broadcast from outside, transfer to inside");
                          m_DevSameNode->Forward (originalPacket, PACKETREPEATNUMBER-1);
                       }
                    else
                       {
                          Forward (originalPacket, PACKETREPEATNUMBER-1);
                       }             
               }
           }
      }
    }
  
  
}


bool
PointToPointNetDevice::SendFromInside (
  Ptr<Packet> packet, 
  const Address &dest, 
  uint16_t protocolNumber, bool fromUpperLayer)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
  NS_LOG_LOGIC ("UID is " << packet->GetUid ());
  
  if (!fromUpperLayer )
      {
         // m_rxCallback (this, packet, protocolNumber, GetRemote ());
      }
  
   if (dest == GetAddress()  || dest == GetBroadcast ())
   {
        NS_LOG_DEBUG ("sent to upper layer" );
        m_rxCallback (this, packet, protocolNumber, GetRemote ());
        Ptr<Packet> SpiderPacket = packet->Copy ();
        //m_spiderrxCallback (this, SpiderPacket, protocolNumber, GetRemote ());
   }
  
  
    if ( dest == GetAddress() )
    {
        return true;
    }



  
  
  
  NS_LOG_DEBUG (Simulator::Now() <<" \t " << GetAddress () << ", SendFromInside data packet to " << dest << ", size " << packet->GetSize() << ", protocolNumber " << protocolNumber << "\t" << GetBroadcast ());
  m_destAddress = Mac48Address::ConvertFrom (dest);
  LlcSnapHeader llc_test;
  if (packet->PeekHeader (llc_test))
  {
                  NS_LOG_DEBUG( "llc_test.GetType " << llc_test.GetType () );
            NS_LOG_DEBUG("Found a LLC looking header of " );
            //llc_test.Print (std::cout);
                             NS_LOG_DEBUG(  ", llc_test.GetType " << llc_test.GetType () );

        if (llc_test.GetType () == 2054)
        {

          NS_LOG_DEBUG(GetAddress ()  << "  Found an ARP packet!,  size " << packet->GetSize());
        }
  }
 
  if  (dest == GetBroadcast () )
     {
        m_destAddress =  Mac48Address::ConvertFrom (GetBroadcast() );
        NS_LOG_DEBUG (Simulator::Now() <<" \t " << GetAddress () << ", SendFromInside data packet to " << m_destAddress << ", size " << packet->GetSize() << ", protocolNumber " << protocolNumber);
    }
  
  m_type =  DATATYPE;
  
  if ( m_state != EXTERNAL_REC_CHANNEL_ACK && m_state != INTERNAL_SEND_CHANNEL_ACK && m_state != EXTERNAL_SEND_CHANNEL_ACK )
  {
      NS_LOG_DEBUG (Simulator::Now() <<" drop data packet, " << GetAddress () << ", since channel is not selected "  );
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


void 
PointToPointNetDevice::SendLinkStateUpdate ( )
{
   if (IsLinkUp () == false)
    {
      NS_ASSERT ("there is no channel");  
      return;
    } 
   
   Ptr<Packet> packet = Create<Packet> ();
   m_type = LINKSTATE;
   //NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " send SendLinkStateUpdate  m_nextHop0 " << m_nextHop0 );
   //NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " send SendLinkStateUpdate  m_nextHop0 " << m_nextHop0 << ", m_nextHop1 " << m_nextHop1 );
         
   AddHeaderChannel (packet, 0x800);
   SendChannelSelection (packet, Mac48Address ("ff:ff:ff:ff:ff:ff"), 0x800); // GetBroadcast (), 0x800 are not really used
   
   //Simulator::Schedule (Seconds(100), &PointToPointNetDevice::SendLinkStateUpdate, this);
}

uint16_t
PointToPointNetDevice::LookupRoutingTable (Mac48Address addr)
{
    uint8_t mac[6];
    addr.CopyTo (mac);
    uint8_t aid_l = mac[5];
    uint8_t aid_h = mac[4] & 0x1f;
    uint16_t aid = (aid_h << 8) | (aid_l << 0); 
    
    uint16_t nextHop;
    
    if ( addr != Mac48Address::ConvertFrom (GetAddress() ))
    {
        //ConvertMacaddr (ppp.GetDestAddre ())
        nextHop = *(tablePoint + aid -1);
    }
    return nextHop;
}

void
PointToPointNetDevice::CreateRoutingTable ()
{
      CalPath = new ShortPath;

    NS_LOG_DEBUG( GetAddress () << " CreateRoutingTable " );
    //SendLinkStateUpdate ();
    if (!m_TopoInitialized)
       {  InitializeTopology ();}
    SendLinkStateUpdate ();
    //InitializeTopology ();
    Mac48Address addr = Mac48Address::ConvertFrom (GetAddress() );
    uint32_t table[V];
    //uint32_t * tablePoint;
    
    uint8_t mac[6];
    addr.CopyTo (mac);
    uint8_t aid_l = mac[5];
    uint8_t aid_h = mac[4] & 0x1f;
    uint16_t aid = (aid_h << 8) | (aid_l << 0); 
  
    NS_LOG_DEBUG( "node " << aid );
    
    uint32_t numhop = 0;
   for (uint16_t kk=0; kk < V; kk++) //only for bi
  {
      for (uint16_t ii=0; ii < V; ii++)
      {
        if (Topology[kk][ii] == 1)
        {
            numhop++;
        }
              //      NS_LOG_DEBUG (  kk << "\t" << ii << "\t" << Topology[kk][ii] );
      }  
  }
   
    
     NS_LOG_DEBUG( GetAddress () << " CreateRoutingTable with hop number " <<   numhop);

    if (aid-1 < V)
    {
        tablePoint  = CalPath->dijkstra(Topology, aid - 1);
        
    for (uint32_t kk=0; kk< V; kk++ )
      {
        //if (aid-1 != kk)
          //NS_LOG_DEBUG( "src " << aid-1 <<  ", dest " << kk << ", next hop " <<  *(tablePoint+kk) );
      } 
    }
    
    Simulator::Schedule (Seconds(2000), &PointToPointNetDevice::CreateRoutingTable, this);
}

void 
PointToPointNetDevice::InitializeTopology ()
{
    //SendLinkStateUpdate ();
  NS_LOG_DEBUG ( " InitializeTopology Topology of  " << GetAddress () );
 m_TopoInitialized = true;
  for (uint16_t kk=0; kk < V; kk++) //only for bi
  {
      for (uint16_t ii=0; ii < V; ii++)
      {
            Topology[kk][ii]=9999;
      }   
  }
  
  uint32_t N = 11;  //temp, only for test
  if (m_channel->IsChannelUni ())
  {
    //uint32_t Nsatellite = 11;  
    uint32_t Nsatellite = 11; // two GES satellite
    for (uint16_t kk=0; kk < V; kk++) //only for uni
     {
      for (uint16_t ii=0; ii < V; ii++)
      {
          if (kk == ii)
          {
            Topology[kk][ii] = 0;
          }
          /*
          else if (ii == kk + 1)
          {
            Topology[kk][ii] = 1;  
          }
         else if (kk==N-1 && ii==0)
         {
            Topology[kk][ii] = 1;  
         } 
           */
      }   
     } 
  }
  else
  {
    for (uint16_t kk=0; kk < m_InterfaceNum; kk++) //only for bi
    {
      for (uint16_t ii=0; ii < m_InterfaceNum; ii++)
      {
          if (kk == ii)
          {
            Topology[kk][ii] = 0;
          }
          
          else if (ii == kk + 1 && kk % 2 != 0)
          {
            Topology[kk][ii] = 0;  
            Topology[ii][kk] = 0; 
          }
          
         else if (ii == kk + 1 && kk % 2 == 0)
          {
            Topology[kk][ii] = 1; 
            Topology[ii][kk] = 1; 
          }
         else if (kk == m_InterfaceNum-1 && ii==0)
         {
            Topology[kk][ii] = 0;  
            Topology[ii][kk] = 0;  
         } 
      }   
    }
 }
  
  for (uint16_t kk=0; kk < V; kk++) //only for bi
  {
      for (uint16_t ii=0; ii < V; ii++)
      {
        //std::cout << Topology[kk][ii] << '\t' ;
      }  
        //std::cout  << '\n' ;
  }
  
    for (uint16_t kk=0; kk < V; kk++) //only for bi
  {
      for (uint16_t ii=0; ii < V; ii++)
      {
        if (Topology[kk][ii] == 1)
        {
            //numhop++;
        }
       //NS_LOG_DEBUG (  kk << "\t" << ii << "\t" << Topology[kk][ii] );
      }  
  }
}

void 
PointToPointNetDevice::UpdateTopologyGES ()
{
   /*   
   //for ges
    
  static uint16_t   count = 1;
  uint16_t distanceToGES0 = 0;
  uint16_t distanceToGES1 = 0;

  
  
    Mac48Address addr = Mac48Address::ConvertFrom (GetAddress() );
    
    // for test
    uint8_t mac[6];
    addr.CopyTo (mac);
    uint8_t aid_l = mac[5];
    uint8_t aid_h = mac[4] & 0x1f;
    uint16_t aid = (aid_h << 8) | (aid_l << 0); 
  

    if (m_channel->IsChannelUni ())
        {
         if (count == aid % 11)
          {
               distanceToGES0 = 1;
           }
        else
          {
              distanceToGES0 = 9999;
          }
       }
    else
       {
        //to do, for bidirectional link
       }
    
     if (m_channel->IsChannelUni ())
        {
         if (count == aid % 11 + 5 )
          {
               distanceToGES1 = 1;
           }
        else
          {
              distanceToGES1 = 9999;
          }
       }
    else
       {
        //to do, for bidirectional link
       }
    
    count++;
   
  
  if (m_channel->IsChannelUni ())
    {
      uint32_t Nsatellite = 11;
      for (uint16_t ii=0; ii < Nsatellite; ii++)
      {
        Topology[ii][11] = distanceToGES0;  
        Topology[ii][12] = distanceToGES1; 
      }
    }
  else
    {
      for (uint16_t ii=0; ii < 22; ii++)
      {
        Topology[ii][22] = distanceToGES0;   
        Topology[ii][23] = distanceToGES1; 
      }
    }
    
   // CreateRoutingTable ();
   Simulator::Schedule (Seconds(100), &PointToPointNetDevice::UpdateTopologyGES, this);
  */  
}



//attached to LEO satellite
void 
PointToPointNetDevice::LEOInitLinkDst (uint32_t position, uint32_t InitPosGES [], std::map<uint32_t, Mac48Address> DeviceMapPosition, uint32_t NumGES, uint32_t NumLEO, Time interval)
{
    
       //m_indicator = position;
       m_initPostion = position; // temp, we assume GES and LEO device of LEO satellite have the same position.
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
                //NS_LOG_DEBUG ("position " << position << ", NumGES " << NumGES);
                //NS_LOG_DEBUG ("kk " << kk << ", m_initPosGES " << m_initPosGES[kk]);
                sum = sum + m_initPosGES[kk];
            }
        
        while (sum)
          {
             count += sum & 1;
             sum = sum >> 1;
          }
        NS_ASSERT (count == m_NumGES);
         
        uint32_t pos = 1;
        for (uint32_t kk = 0; kk < NumLEO; kk++)
           {
              m_indicator[kk] = pos;
              pos <<= 1;
           }
        
     LEOupdateLinkDst ();  
      
}
//position of mine
void 
PointToPointNetDevice::LEOupdateLinkDst ()
{ 
    
       //fixed below    
     for (uint32_t staId = 0; staId < m_NumLEO; staId++)
     {
        uint32_t sum = 0;
        uint32_t ges = 0; //the ges can be linked to
        bool connected = false;
        for (uint32_t  kk = 0; kk < m_NumGES; kk++)
            {
                //NS_LOG_DEBUG (GetAddress () << "\t" << m_indicator  << " m_initPosGES[kk] "  << m_initPosGES[kk] << ", m_NumLEO " << m_NumLEO);
                m_linkDst[kk] = m_initPosGES[kk] & m_indicator[staId];
                sum = sum + m_linkDst[kk] ;
            }
          
           for (uint32_t  kk = 0; kk < m_NumGES; kk++) // initialize, all GES is not connected
            {
                Mac48Address  Addr = m_deviceMapPosition.find (kk)->second;
                uint8_t mac[6];
                Addr.CopyTo (mac);
                uint8_t aid_l = mac[5];
                uint8_t aid_h = mac[4] & 0x1f;
                uint16_t aidNext1 = (aid_h << 8) | (aid_l << 0); 
                Topology[staId][aidNext1 - 1] = 9999;   
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
          NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () << ", device id " <<  staId << " LEO-LEOdevice links to GES "  << m_dstGESAddr);
                uint8_t mac[6];
                m_dstGESAddr.CopyTo (mac);
                uint8_t aid_l = mac[5];
                uint8_t aid_h = mac[4] & 0x1f;
                uint16_t aidNext1 = (aid_h << 8) | (aid_l << 0); 
                Topology[staId][aidNext1 - 1] = 1;
        }
        else
        {
          m_dstGESAddr = Mac48Address ("00:00:00:00:00:00"); //unreachable dst
          //NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () << ", device id " <<  staId << " is not linked to any GES ");
        }
        
                
        // Circular
        // example, Assuming that CHAR_BITS == 8, x = ( x << 1U ) | ( x >> 7U ) ; would do a circular bit shift left by one.
        //m_indicator << 1;
        uint32_t indicator_a = m_indicator[staId];
        uint32_t indicator_b = m_indicator[staId];
        m_indicator[staId] = (indicator_a >> 1) | (indicator_b <<  (m_NumLEO - 1) );
     }
        
    LEOupdateLinkDstEvent =  Simulator::Schedule (m_dstGESinterval, &PointToPointNetDevice::LEOupdateLinkDst, this);
     
}


void 
PointToPointNetDevice::UpdateTopology (Mac48Address src, Mac48Address nexthop0, Mac48Address nexthop1)
{
   /*
  uint8_t mac[6];
  src.CopyTo (mac);
  uint8_t aid_l = mac[5];
  uint8_t aid_h = mac[4] & 0x1f;
  uint16_t aidSrc = (aid_h << 8) | (aid_l << 0); 
  
  nexthop0.CopyTo (mac);
  aid_l = mac[5];
  aid_h = mac[4] & 0x1f;
  uint16_t aidNext = (aid_h << 8) | (aid_l << 0); 
  
  nexthop1.CopyTo (mac);
  aid_l = mac[5];
  aid_h = mac[4] & 0x1f;
  uint16_t aidNext1 = (aid_h << 8) | (aid_l << 0); 
  
  if (m_channel->IsChannelUni ())
  {
    Topology[aidSrc-1][aidNext-1] = 1;  
  }
  else
  {
    //Topology[aidSrc-1][aidNext-1] = 1;   
    //Topology[aidSrc-1][aidNext1-1] = 0; 
  }
  
  //update the toplogy of my self, the code below can be moved to other funcuton, donot need to alwasy update
  (Mac48Address::ConvertFrom (GetAddress ()) ).CopyTo (mac );
   aid_l = mac[5];
   aid_h = mac[4] & 0x1f;
  uint16_t aidself = (aid_h << 8) | (aid_l << 0);
  
  
  m_nextHop0.CopyTo (mac );
   aid_l = mac[5];
   aid_h = mac[4] & 0x1f;
  uint16_t aidself_nexthop = (aid_h << 8) | (aid_l << 0);
   NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " aidself " << aidself -1 << ", nexthop0 " << m_nextHop0  );

  //Topology[aidself-1][aidself_nexthop - 1] = 1;
  
  
 NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " update link state of " << aidSrc-1 << ", nexthop0 " << aidNext-1 << ", nexthop1 " << aidNext1-1 );
 
  NS_LOG_DEBUG ( " new Topology of  " << GetAddress () );
  uint16_t numhop = 0;
  for (uint16_t kk=0; kk < V; kk++) //only for bi
  {
      for (uint16_t ii=0; ii < V; ii++)
      {
        if (Topology[kk][ii] == 1)
        {
            numhop++;
        }
       //NS_LOG_DEBUG (  kk << "\t" << ii << "\t" << Topology[kk][ii] );
      }  
  }
  
   NS_LOG_DEBUG (  GetAddress () << ", m_topologyCompleted " << m_topologyCompleted  << ", numhop " << numhop) ;

  if (m_topologyCompleted && m_channel->IsChannelUni () && numhop != 9)
  {
      NS_LOG_DEBUG ("topology unexpectedly changed");
      //NS_ASSERT_MSG (numhop==9,"topology unexpectedly changed");
  }
  if (m_channel->IsChannelUni () && numhop == 9)
  {
              m_topologyCompleted = true;
              NS_LOG_DEBUG ( " complete Topology of  " << GetAddress () << ", m_topologyCompleted " << m_topologyCompleted) ;
  }
  else if (!m_channel->IsChannelUni () && numhop == 21)
  {
        NS_LOG_DEBUG ( GetAddress () << " !complete Topology of  " << numhop );
  }
  */
  //CreateRoutingTable ();
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
  NS_LOG_DEBUG ("m_address " << m_address );
}

Address
PointToPointNetDevice::GetAddress (void) const
{
  //NS_LOG_DEBUG ("GetAddress " << m_address );
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
    NS_LOG_DEBUG (GetAddress () << " receive channel command from inside, it uses channel " << tx << " to send, and " << rx << " to receive.");
    SetUsedChannelInside (tx, rx);
    TryToSetLinkChannel ();
}

  
void 
PointToPointNetDevice::TryToSetLinkChannel (void)
{
   NS_LOG_FUNCTION (this);
   NS_LOG_DEBUG (GetAddress () << " TryToSetLinkChannel " );
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
    //NS_LOG_DEBUG (GetAddress () << " SetState " << state);
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
            NS_LOG_DEBUG (GetAddress ()  <<  " no available tx channel ...."); 
          }
   
        uint16_t channel= m_linkchannelTx;
        if (channel != m_channel0_usedOutside &&  channel != m_channel1_usedInside && channel != m_linkchannelRx  ) // tx (outside), rx(inside), )
        // channel selection may not be finished due to "channel != m_linkchannelRx"
           {
              m_linkchannelTx = channel;
              NS_LOG_DEBUG (GetAddress ()  <<  " ChangeTxChannel " << m_linkchannelTx); 
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
 /*
   if (!Constrainthold ())
    {   
       NS_ASSERT ("Constrainthold not hold inside");
        if (m_watingChannelRespEvent.IsRunning()) // why it is still running?
            {
                m_watingChannelRespEvent.Cancel ();
            }
        if (m_watingChannelConfEvent.IsRunning()) // why it is still running?
            {
                m_watingChannelConfEvent.Cancel ();
            }
        SetState (NO_CHANNEL_CONNECTED);
        TryToSetLinkChannel ();
    } */
   
       if ( m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
        {  
         if (!Constrainthold () )
          {
            NS_LOG_DEBUG ("m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx "  << m_linkchannelTx );
            NS_LOG_DEBUG ("m_channel0_usedInside " << m_channel0_usedInside << ", m_channel1_usedInside "  << m_channel1_usedInside);
            NS_LOG_DEBUG ("m_channel0_usedOutside " << m_channel0_usedOutside << ", m_channel1_usedOutside "  << m_channel1_usedOutside);
            NS_LOG_DEBUG ("Constrainthold not hold inside ??????????????????");
            if (m_watingChannelRespEvent.IsRunning()) // why it is still running?
                {
                  m_watingChannelRespEvent.Cancel ();
                }
            if (m_watingChannelConfEvent.IsRunning()) // why it is still running?
                {
                  m_watingChannelConfEvent.Cancel ();
                }
            SetState (NO_CHANNEL_CONNECTED);
            TryToSetLinkChannel ();
          }
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

    if ( m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
        {  
         if (!Constrainthold () )
          {
            NS_LOG_DEBUG ("m_linkchannelRx " << m_linkchannelRx << ", m_linkchannelTx "  << m_linkchannelTx );
            NS_LOG_DEBUG ("m_channel0_usedInside " << m_channel0_usedInside << ", m_channel1_usedInside "  << m_channel1_usedInside);
            NS_LOG_DEBUG ("m_channel0_usedOutside " << m_channel0_usedOutside << ", m_channel1_usedOutside "  << m_channel1_usedOutside);
            NS_LOG_DEBUG ("Constrainthold not hold inside ??????????????????");
            if (m_watingChannelRespEvent.IsRunning()) // why it is still running?
                {
                  m_watingChannelRespEvent.Cancel ();
                }
            if (m_watingChannelConfEvent.IsRunning()) // why it is still running?
                {
                  m_watingChannelConfEvent.Cancel ();
                }
            SetState (NO_CHANNEL_CONNECTED);
            TryToSetLinkChannel ();
          }
        }
 }

bool
PointToPointNetDevice::Constrainthold (void)
{
   if (m_linkchannelRx == m_linkchannelTx)
   {
       return false;
   }
   
   if (m_channel0_usedInside == m_channel1_usedInside)
   {
       return true; // incorrect inside channel
   }
   
  if (m_channel0_usedOutside == m_channel1_usedOutside)
   {
       return true; // incorrect inside channel
   }
   
  if (m_linkchannelRx == m_channel0_usedInside)
      {
       return false;
      }
   
   if (m_linkchannelRx == m_channel1_usedOutside)
      {
       return false;
      }
   
   if (m_linkchannelTx == m_channel1_usedInside)
      {
       return false;
      }
   
    if (m_linkchannelTx == m_channel0_usedOutside)
      {
       return false;
      }
    return true;
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
        NS_LOG_DEBUG (GetAddress () << " restart SendChannelRequest even selected..., m_watingChannelRespTime " << m_watingChannelRespTime << ", m_watingChannelConfTime " << m_watingChannelConfTime);
        return;
    }
   
    if ( m_SendChannelRequestPacketTime +  Channel_delay_packet * PACKETREPEATNUMBER > Simulator::Now()  )
      {
          //NS_LOG_DEBUG (GetAddress ()  <<  " can not start channel req since last one did not finish"); 
          //return;
      }
    
   Ptr<Packet> packet = Create<Packet> ();
   m_type = CHANNELREQ;

           
   if (counter == PACKETREPEATNUMBER) // && m_SendChannelRequestPacketEvent.IsRunning() )
     {
       NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () <<  " stop sending m_SendChannelRequestPacketEvent "); 
       if ( m_SendChannelRequestPacketEvent.IsRunning() )
         {
           m_SendChannelRequestPacketEvent.Cancel ();
         }
       return;
     }
   counter++;
   
   NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " send SendChannelRequestPacket at channel " << m_linkchannelTx << " counter " << counter
           << ", id " << m_reqid << ", PACKETREPEATNUMBER " << PACKETREPEATNUMBER);
      
   AddHeaderChannel (packet, 0x800);
   NS_LOG_DEBUG ( " here " );
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
   
   if (counter == PACKETREPEATNUMBER) // && m_SendChannelResponsePacketEvent.IsRunning() )
     {
       NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () <<  " stop sending m_SendChannelResponsePacket "); 
       if ( m_SendChannelResponsePacketEvent.IsRunning() )
         {
           m_SendChannelResponsePacketEvent.Cancel ();
         }
       return;
     }
   
   counter++;
   
   Ptr<Packet> packet = Create<Packet> ();

   m_type = CHANNELRESP;
   
   
   NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " send SendChannelResponsePacket at channel " << m_linkchannelTx << " counter " << counter);
   AddHeaderChannel (packet, 0x800);
   SendChannelSelection (packet, Mac48Address ("ff:ff:ff:ff:ff:fe"), 0x800); // GetBroadcast (), 0x800 are not really used
   m_SendChannelResponsePacketEvent = Simulator::Schedule (Channel_delay_packet, &PointToPointNetDevice::SendChannelResponsePacket, this, counter);
   m_SendChannelResponsePacketTime =  Simulator::Now ();

   
} 

void 
PointToPointNetDevice::SendChannelRequest (void)
{       
   NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << "  SendChannelRequest "  << m_linkchannelRx << m_linkchannelTx);
   
     if (m_watingChannelRespEvent.IsRunning()) // why it is still running?
        {
            m_watingChannelRespEvent.Cancel ();
        }
   
    if (m_watingChannelConfEvent.IsRunning()) // why it is still running?
        {
            m_watingChannelConfEvent.Cancel ();
        }
   
   

   if ( m_state == EXTERNAL_SEND_CHANNEL_ACK)
    {  
       if (!selectedTraced)
       {
        m_channelSelected (Mac48Address::ConvertFrom(GetAddress ()), Simulator::Now (), m_linkchannelRx, m_linkchannelTx);
        selectedTraced = true;
        CreateRoutingTable ();
       }
        NS_LOG_DEBUG (GetAddress () << " channel selected at state EXTERNAL_SEND_CHANNEL_ACK  " << m_watingChannelRespTime << ", m_watingChannelConfTime " << m_watingChannelConfTime);
        NS_LOG_DEBUG (GetAddress ()  <<  " m_nextHop0 " << m_nextHop0 << ", m_nextHop1 " << m_nextHop1 ); 
        if (m_watingChannelRespEvent.IsRunning()) // why it is still running?
        {
            m_watingChannelRespEvent.Cancel ();
        }
        return;
    }

    if (m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
    {
        NS_LOG_DEBUG (GetAddress () << " restart SendChannelRequest even selected..., m_watingChannelRespTime " << m_watingChannelRespTime << ", m_watingChannelConfTime " << m_watingChannelConfTime);
        if (m_watingChannelRespEvent.IsRunning()) // why it is still running?
        {
            m_watingChannelRespEvent.Cancel ();
        }
        return;
    }
    if (m_state == EXTERNAL_SEND_CHANNEL_REQ)
    {
           NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << "...  SendChannelRequest "  << m_linkchannelRx << m_linkchannelTx);
           SetState (EXTERNAL_SEND_CHANNEL_REQ);
           if (m_WaitChannelEvent.IsRunning())
               m_WaitChannelEvent.Cancel();
            m_linkchannelRx = m_rng->GetInteger (1, CHANNELNUMBER);
            WaitChannel ();
            ChangeTxChannel ();
            
            
    }
    else
    {
           NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << "  SendChannelRequest "  << m_linkchannelRx << m_linkchannelTx);

           SetState (EXTERNAL_SEND_CHANNEL_REQ);
           if (m_WaitChannelEvent.IsRunning())
               m_WaitChannelEvent.Cancel();
           m_linkchannelRx = m_rng->GetInteger (1, CHANNELNUMBER);
           WaitChannel ();
           ChangeTxChannel ();
    }
    //SetState (EXTERNAL_SEND_CHANNEL_REQ);
    //ChangeTxChannel ();
     
   
    NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " start SendChannelRequestEvent " );
    m_reqid = m_packetId;
    NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << "  SendChannelRequest id "  << m_reqid);
    SendChannelRequestPacket (0);
    uint16_t delay = m_rng->GetInteger (1, backoffcounter);
    Time ChannelResp_delay_total = MilliSeconds(delay) + ChannelResp_delay;
    m_watingChannelRespEvent = Simulator::Schedule (ChannelResp_delay_total, &PointToPointNetDevice::SendChannelRequest, this);   
    m_watingChannelRespTime =  Simulator::Now ();  
    //m_ResetTimeOutEvent = Simulator::Schedule (Seconds(100), &PointToPointNetDevice::Reset, this);   
}

void 
PointToPointNetDevice::SendChannelResponse (Mac48Address dest, uint16_t channel0, uint16_t channel1, bool timeout, uint32_t packetid)
{
    
   if (m_state == EXTERNAL_REC_CHANNEL_ACK || m_state == INTERNAL_SEND_CHANNEL_ACK)
    {
       if (timeout)
       {
        NS_LOG_DEBUG (GetAddress () << " restart SendChannelResponse even selected...");
       }
    }
        
   if ( m_SendChannelResponsePacketTime +  Channel_delay_packet * PACKETREPEATNUMBER > Simulator::Now())
      {
          //NS_LOG_DEBUG (GetAddress ()  <<  " can not start channel resp since last one did not finish"); 
          //return;
      }
  
   m_destAddressResp = dest;
   uint16_t repetition = 0;
   
   
 if ( m_linkchannelTx == CHANNELNOTDEFINED) 
  {
   while (1)
     { 
        if (repetition == CHANNELNUMBER)
          {
            NS_LOG_DEBUG (GetAddress ()  <<  " no available tx resp channel ...."); 
          }
                
        uint32_t channel= m_rng->GetInteger (1, CHANNELNUMBER);
        if (channel != m_channel0_usedOutside &&  channel != m_channel1_usedInside && channel != m_linkchannelRx  ) // tx (outside), rx(inside), )
        // channel selection may not be finished due to "channel != m_linkchannelRx"
           {
              m_linkchannelTx = channel;
              NS_LOG_DEBUG (GetAddress ()  <<  " choose SendChannelResponse channel " << m_linkchannelTx); 
              break;
           }
        repetition++;
     } 
 }
           
      m_respid = packetid;
    NS_LOG_DEBUG(Simulator::Now() << "\t" << GetAddress () << " start channel resp to " << m_destAddressResp );
    SendChannelResponsePacket (0);
    uint16_t delay = m_rng->GetInteger (1, backoffcounter);
    Time ChannelConf_delay_total = ChannelConf_delay + MilliSeconds(delay);
   if (timeout)
     {
          m_watingChannelConfEvent = Simulator::Schedule (ChannelConf_delay_total, &PointToPointNetDevice::SendChannelRequest, this);
          //m_ResetTimeOutEvent = Simulator::Schedule (Seconds(100), &PointToPointNetDevice::Reset, this);   
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

   m_destAddressAck = dest;
   m_ackid = packetid;
   m_type = CHANNELACK;
   
   NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () << " send sendChannelACK at channel " << m_linkchannelTx);
   AddHeaderChannel (packet, 0x800);
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
            NS_LOG_DEBUG (GetAddress ()  <<  " no available tx resp channel ...."); 
          }
                
        uint32_t channel= m_rng->GetInteger (1, CHANNELNUMBER);
        if (channel != m_channel0_usedOutside &&  channel != m_channel1_usedInside && channel != m_linkchannelRx  ) 
           {
              m_linkchannelTx = channel;
              NS_LOG_DEBUG (GetAddress ()  <<  " choose SendChannelResponse channel " << m_linkchannelTx); 
              break;
           }
        repetition++;
     }  
   }
   
    if (counter == PACKETREPEATNUMBER) // && m_SendChannelResponsePacketEvent.IsRunning() )
     {
       NS_LOG_DEBUG (Simulator::Now() << "\t" << GetAddress () <<  " stop Forward packet "); 
       if ( m_watingChannelForwardEvent.IsRunning() )
         {
           m_watingChannelForwardEvent.Cancel ();
         }
       return;
     }

   counter++;
   /*
   m_queue->Enqueue (packet);
   
   Ptr<Packet> packetforward;
   packetforward = m_queue->Dequeue ();
  if (packetforward != 0 )
  {  
      if (m_txMachineState == READY)
        {
        TransmitStart (packetforward);
        }
      m_watingChannelForwardEvent = Simulator::Schedule (Channel_delay_packet, &PointToPointNetDevice::Forward, this, packet, counter);
      return;
  }
    */
   
   
    // if (m_queueMap.find (m_Qos)->second->Enqueue (packet)) 
   

    Ptr<Packet> PacketForwardtest = packet->Copy ();

    EnqueueForward (PacketForwardtest);
    
    PppHeader ppp;
    packet->PeekHeader (ppp);
    
   if (ppp.GetType() == DATATYPE || ppp.GetType() == DATAACK)
     {
       //NS_LOG_DEBUG(Simulator::Now() << "\t" << GetAddress () <<  "  Forward packet "); 
       ForwardDown ();
       return;
     }
        
      if (m_txMachineState == READY)
       {
         Ptr<Packet> packetforward = DequeueForward ();
         if (packetforward !=0)
         {
          TransmitStart (packetforward);
         }
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
  
  NS_LOG_DEBUG (Simulator::Now() <<" \t " << GetAddress () << ", send to " << dest);
  
  if (m_type ==  DATATYPE || m_type == DATAACK || m_type == N_ACK)
  {
      NS_LOG_DEBUG (Simulator::Now() <<" drop data packet, " << GetAddress () << ", incorrect data type " );
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
  //AddHeader (packet, protocolNumber);

  m_macTxTrace (packet);
  
    PppHeader ppp;
    packet->PeekHeader (ppp);
     // NS_LOG_DEBUG (Simulator::Now() <<" \t " << ppp.GetSourceAddre () << ", send to " << ppp.GetDestAddre () << ", hop0 " << ppp.GetNextHop0 () << ", hop1 " << ppp.GetNextHop1 ());


  //
  // We should enqueue and dequeue the packet to hit the tracing hooks.
  //

  m_queue->Enqueue (packet);
  //NS_LOG_DEBUG (Simulator::Now() <<" \t " << GetAddress () << ", enqueue, qos " << uint16_t (ppp.GetQos ()) );

  Ptr<Packet>  packetkupper = m_queue->Dequeue ();
  packetkupper->PeekHeader (ppp);
     // NS_LOG_DEBUG (Simulator::Now() <<" \t " << GetAddress () << ", dequeue, qos " << uint16_t (ppp.GetQos ()) );

      Ptr<Packet> PacketForwardtest = packetkupper->Copy ();

  EnqueueForward(PacketForwardtest);

  

  //m_queueForward[4]->Enqueue(packetkupper);
   //Ptr<Packet>  packetsend = m_queueForward[4]->Dequeue ();
  
       if (Simulator::Now ().GetSeconds () > 300)
       {
           //NS_LOG_DEBUG (Simulator::Now() <<" \t " << GetAddress () << ", dequeue, qos " << uint16_t (ppp.GetQos ()) );
       }
      //
      // If the channel is ready for transition we send the packet right now
      // 
      if (m_txMachineState == READY)
        {
          Ptr<Packet>  packetsend = DequeueForward ();
          if (packetsend!=0)
          {
            m_snifferTrace (packetsend);
            m_promiscSnifferTrace (packetsend);
            return TransmitStart (packetsend);
          }
        }
    

  // Enqueue may fail (overflow)
  //m_macTxDropTrace (packet);
  return false;
}

bool
PointToPointNetDevice::Send (
  Ptr<Packet> packet, 
  const Address &dest, 
  uint16_t protocolNumberARQtemp)
{
  //  for ARQ, temp
  uint8_t QosTempARQ = protocolNumberARQtemp -2048;
 uint16_t protocolNumber = 2048;
  //

  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
  NS_LOG_LOGIC ("UID is " << packet->GetUid ());
  
  
  if ( m_state != EXTERNAL_REC_CHANNEL_ACK && m_state != INTERNAL_SEND_CHANNEL_ACK && m_state != EXTERNAL_SEND_CHANNEL_ACK )
  {
      NS_LOG_DEBUG (Simulator::Now() <<" drop data packet, " << GetAddress () << ", since channel is not selected "  );
      return false;  
  }
  m_destAddress = Mac48Address::ConvertFrom (dest);

    NS_LOG_DEBUG (Simulator::Now() <<" \t " << GetAddress () << ", send data packet to " << dest << ", size " << packet->GetSize() << ", protocolNumber " << protocolNumber );

 
  if  (dest == GetBroadcast () )
     {
        m_destAddress =  Mac48Address::ConvertFrom (GetBroadcast() );
    }
  
unicast:
  m_type =  DATATYPE;
  
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
  //m_Qos = 0;
  m_Qos = QosTempARQ;
 // NS_LOG_DEBUG ("set data m_Qos " << uint16_t(m_Qos));
  AddHeader (packet, protocolNumber);

  m_macTxTrace (packet);
  
   PppHeader ppp;
   packet->PeekHeader (ppp);
    
  m_Qos = ppp.GetQos ();
 // NS_LOG_DEBUG ("send data m_Qos " << uint16_t(m_Qos));
  m_queueMap.find (m_Qos)->second->Enqueue (packet); //uppper layer queue
  
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
  NS_LOG_DEBUG (GetAddress () <<" enqueue ARQ to forward, m_Qos " << uint16_t(ppp.GetQos ()) << ", id " << ppp.GetID ());

//
  


  
  
//  
  /*
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
          
          NS_LOG_DEBUG(Simulator::Now() << ", dst: " << dest << " my address:" << GetAddress()  <<  " send--route to " << nextHop << ", packet " << packet->GetSize() << ", protocolNumber " << protocolNumber);
           
          if (nextHop == aid - 1 )
            {
              //Ptr<Packet> packetsend = DequeueForward ();
              NS_LOG_DEBUG(dest << "\t" << GetAddress()  <<  " route to  neighbour " );
              //m_DevSameNode->Forward (packetsend, PACKETREPEATNUMBER-1); 
              goto LinkedDevice;
           //return true;
           //return m_DevSameNode->Froward (packet, dest, protocolNumber, true);
           //return m_DevSameNode->SendFromInside (packet, dest, protocolNumber, true);
            }
          else
            {
              goto LinkedDevice;
            }
          return false;
        }
  }
      
 
     LinkedDevice:
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
          
          return TransmitStart (packetsend);
        }
   */ 
  return ForwardDown ();
}

void
PointToPointNetDevice::EnqueueForward (Ptr<Packet> packet)
{
   PppHeader ppp;
   packet->PeekHeader (ppp);  //enqueue to forward queue
   bool IsQueued = false;
   if ( ppp.GetType () == DATATYPE || ppp.GetType () == DATAACK || ppp.GetType () == N_ACK )
    {
       IsQueued = m_queueForward.find (ppp.GetQos ())->second->Enqueue (packet);
       uint32_t queueLen = m_queueForward.find (ppp.GetQos ())->second->GetNPackets ();
       uint32_t qos = ppp.GetQos ();
       uint32_t queueTx = Simulator::Now().GetMicroSeconds ();
       m_ForwardQueueTrace ( queueTx, Mac48Address::ConvertFrom(GetAddress ()), qos, queueLen );
       //NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () <<" forwardqueue enqueue m_Qos " << uint16_t(ppp.GetQos ()) << ", original id " << ppp.GetID () << ", size " << packet->GetSize () << " IsQueued "<<  IsQueued);
    }
   else
    {
       IsQueued = m_queueForward[4]->Enqueue(packet);
    }
   
}

Ptr<Packet>  
PointToPointNetDevice::DequeueForward (void)
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
              //NS_LOG_DEBUG ( "size " << m_queueForward.find (qos-1)->second->GetNPackets () );
              NS_LOG_DEBUG (Simulator::Now () << "\t" << GetAddress () <<" forwardqueue dequeue m_Qos " << uint16_t(ppp.GetQos ()) << ", id " << ppp.GetID () << ", size " << packet->GetSize () << " src " << ppp.GetSourceAddre ());  
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

Ptr<const Packet>  
PointToPointNetDevice::PeekqueueForward (void)
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
PointToPointNetDevice::PreparePacketToSend ()
{
    //NS_LOG_DEBUG (GetAddress () << " PreparePacketToSend"); 

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
            //return m_queueMap.find (qos-1)->second->Dequeue ();

            queueEmpyt = false;
            PppHeader ppp;

            packetkPeek->PeekHeader (ppp);

            if (ppp.GetDestAddre () == Mac48Address ("ff:ff:ff:ff:ff:ff") || ppp.GetType () != DATATYPE)
            {
                return m_queueMap.find (qos-1)->second->Dequeue ();
            }

            RemoteSatelliteTx * Satellite = ARQLookupTx ( ppp.GetDestAddre (), ppp.GetID () );
            RemoteSatelliteARQBufferTx * buffer  = Satellite->m_buffer;
            
             //NS_LOG_DEBUG (GetAddress () << " buffer->m_bufferSize " << buffer->m_bufferSize <<  ", buffer->m_listPacket->size " << buffer->m_listPacket->size ());

            if (buffer->m_bufferSize == buffer->m_listPacket->size ()  )
             {
               NS_LOG_DEBUG (GetAddress () <<  " ARQ is full, packets are not dequeued" << " buffer->m_bufferSize " << buffer->m_bufferSize << ", buffer->m_listPacket->size ()  " << buffer->m_listPacket->size () ); 
               packet = ARQSend (buffer);
             }
            else 
             {
               NS_ASSERT(buffer->m_bufferSize > buffer->m_listPacket->size ());
               packetSend = m_queueMap.find (qos-1)->second->Dequeue (); 
               //NS_LOG_DEBUG (GetAddress () << " ARQ is not full, packets are not dequeued"); 
              packet = ARQSend (buffer, packetSend);               
             }
            //NS_ASSERT (packet != 0); this could happen if all packet are sent but not receive ack
            if (packet !=0)
            {
            packet->PeekHeader (ppp);
            NS_LOG_DEBUG (GetAddress () <<  " get ARQ packet id " <<  ppp.GetID () << ", size  " << packet->GetSize () << ", qos  " << ppp.GetQos () ); 
            }
            else
            {
               // NS_LOG_DEBUG (GetAddress () <<  " get ARQ packet 0 "  ); 

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
    
    
  
  


   /*
  for (uint8_t qos = 4; qos > 0; qos--)
   {
       NS_LOG_DEBUG ("PointToPointNetDevice m_Qos " << uint16_t (qos-1) << ", size " << m_queueMap.find (qos-1)->second->GetNPackets ()); 
       packet =  m_queueMap.find (qos-1)->second->Dequeue ();
       if (packet != 0)
         {
            return packet;
            //break;
         }
   } 
  
   */
 
  
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
  //return false;
  return true;
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

void
PointToPointNetDevice::SetSpiderReceiveCallback (NetDevice::ReceiveCallback cb)
{
    NS_LOG_FUNCTION (this);
    m_spiderrxCallback = cb;
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
    case 0x0806: return 0x0806;   //Arp
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
    case 0x0806: return 0x0806;   //Arp
    default: NS_ASSERT_MSG (false, "EtherToPpp PPP Protocol number not defined!");
    }
  return 0;
}

void
PointToPointNetDevice::AddDevice (Ptr<PointToPointNetDevice> dev)
{
  m_DevSameNode = dev;
}


void
PointToPointNetDevice::AddGESDevice (Ptr<PointToPointNetDeviceGES> deva)
{
  m_GESSameNodeFlag = true;
  m_GESSameNode = deva;
  NS_LOG_DEBUG (GetAddress () << " has GESDEVICE " << m_GESSameNode->GetAddress() );
}



void
PointToPointNetDevice::CheckAvailablePacket (RemoteSatelliteARQBuffer * buffer, Mac48Address src)
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


        Simulator::ScheduleNow (&PointToPointNetDevice::CheckAvailablePacket, this, buffer, src);
    }
    else 
    {
        NS_ASSERT ( buffer->m_bufferStart < (*i) );
    }
    
}


void
PointToPointNetDevice::ARQReceive (Mac48Address src, Mac48Address dst, Time rxtime, uint32_t size , uint32_t protocol, uint32_t packetid, Ptr<Packet> packet)
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
PointToPointNetDevice::ARQLookup (Mac48Address src, Time rece, uint32_t size , uint32_t protocol, uint32_t packetid)  const
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
  
  const_cast<PointToPointNetDevice *> (this)->m_statDevice.push_back (Satellite);
  
  ARQRxBufferCheck (buffer);
  
  return Satellite;
}

            
            



bool
PointToPointNetDevice::ARQRxBufferCheck (RemoteSatelliteARQBuffer * buffer) const
{
    uint32_t size = buffer->m_listPacket->size ();
    NS_ASSERT (buffer->m_listRecTime->size () == size);
    NS_ASSERT (buffer->m_listSize->size () == size);
    NS_ASSERT (buffer->m_listProtocol->size () == size);
    NS_ASSERT (buffer->m_listPacketId->size () == size);
    return true;
}


bool
PointToPointNetDevice::ARQTxBufferCheck (RemoteSatelliteARQBufferTx * buffer) const
{
    uint32_t size = buffer->m_listPacket->size ();
    NS_ASSERT (buffer->m_listTxTime->size () == size);
    NS_ASSERT (buffer->m_listPacketId->size () == size);
    NS_ASSERT (buffer->m_listPacketStatus->size () == size);
    NS_ASSERT (buffer->m_listPacketRetryNum->size () == size);
    return true;
}


void
PointToPointNetDevice::ARQAckTimeout (RemoteSatelliteARQBufferTx * buffer)
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
    ARQAckTimeoutEvent = Simulator::Schedule(ARQAckTimer, &PointToPointNetDevice::ARQAckTimeout, this, buffer );
    //NS_ASSERT (packetInbuffer);
} 

void
PointToPointNetDevice::ARQN_ACKRecevie (Mac48Address dst, uint32_t packetid) 
{
    RemoteSatelliteTx * Satellite = ARQLookupTx (dst, packetid); 
    RemoteSatelliteARQBufferTx * buffer = Satellite->m_buffer;
    
    ARQTxBufferCheck (buffer);

    //ARQAckTimeout (buffer, packetid);
}
       
  
  
void
PointToPointNetDevice::ARQACKRecevie (Mac48Address dst, uint32_t packetid)  
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
PointToPointNetDevice::ARQSend (RemoteSatelliteARQBufferTx * buffer)  
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
                  ARQAckTimeoutEvent = Simulator::Schedule(ARQAckTimer, &PointToPointNetDevice::ARQAckTimeout, this, buffer);
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
           ARQAckTimeoutEvent = Simulator::Schedule(ARQAckTimer, &PointToPointNetDevice::ARQAckTimeout, this, buffer);
       } 

    return 0;
}

Ptr<Packet> 
PointToPointNetDevice::ARQSend (RemoteSatelliteARQBufferTx * buffer, Ptr<Packet> packet)
{
   
     NS_LOG_DEBUG  (GetAddress () <<"buffer->m_listPacket->size () " <<  buffer->m_listPacket->size ()  << ", buffer->m_bufferSize " << buffer->m_bufferSize); // || buffer->m_listPacket->size () < buffer->m_bufferSize);

    NS_ASSERT (buffer->m_listPacket->size () < buffer->m_bufferSize || buffer->m_listPacket->size () == buffer->m_bufferSize );
    ARQTxBufferCheck (buffer);
    
    PppHeader ppp;
    packet->PeekHeader (ppp);
    Time txTime = Seconds (0);
    
    NS_LOG_UNCOND(GetAddress () << " original id  " << ppp.GetID()  << " new id " << ARQ_Packetid);
        
    uint16_t protocol = 0;
     ProcessHeader (packet, protocol);//t0 do
     ppp.SetID (ARQ_Packetid);
     packet->AddHeader (ppp);
     
     PppHeader pppTest;
     packet->PeekHeader (pppTest);
     


    
    buffer->m_listPacket->push_back (packet);
    buffer->m_listTxTime->push_back (txTime);
    //buffer->m_listPacketId->push_back (ppp.GetID ());
    buffer->m_listPacketId->push_back (ARQ_Packetid);
    buffer->m_listPacketStatus->push_back (NO_TRANSMITTED);
    buffer->m_listPacketRetryNum->push_back (0);
    
    ARQ_Packetid++;
    //NS_LOG_DEBUG (GetAddress () << "  ARQSend am_listPacketId size " << buffer->m_listPacketId->size () << ", id " << pppTest.GetID () << ", " << pppTest.GetDestAddre ());

    
    return ARQSend (buffer); 
}


RemoteSatelliteTx * 
PointToPointNetDevice::ARQLookupTx (Mac48Address dst, uint32_t packetid)  const
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
  
  const_cast<PointToPointNetDevice *> (this)->m_statDeviceTx.push_back (Satellite);

  
  NS_LOG_DEBUG (GetAddress () << " create buffer for Tx " << dst );

    ARQTxBufferCheck (buffer);
  
  return Satellite;
}



bool
PointToPointNetDevice::SendAck (const Address &dest, 
  uint16_t protocolNumber, uint32_t packetid, uint32_t acktype, uint32_t QosARQ)
{
  NS_ASSERT (acktype == DATAACK || acktype == N_ACK);
          //NS_LOG_UNCOND (GetAddress () << " send ack to " << packetid );

  
  Ptr<Packet> packet = Create<Packet> ();;
  if ( m_state != EXTERNAL_REC_CHANNEL_ACK && m_state != INTERNAL_SEND_CHANNEL_ACK && m_state != EXTERNAL_SEND_CHANNEL_ACK )
  {
      return false;  
  }
  m_destAddress = Mac48Address::ConvertFrom (dest);

 
  if  (dest == GetBroadcast () )
     {
        m_destAddress =  Mac48Address::ConvertFrom (GetBroadcast() );
    }
  

unicast:
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
  //NS_LOG_DEBUG ("set ack m_Qos " << uint16_t(m_Qos));
  
  m_ackid = packetid;
  AddHeader (packet, protocolNumber);

  m_macTxTrace (packet);
  
   PppHeader ppp;
   packet->PeekHeader (ppp);
    
  m_Qos = ppp.GetQos ();
  //NS_LOG_DEBUG ("send ack m_Qos " << uint16_t(m_Qos));
  
  Ptr<Packet> PacketForwardtest = packet->Copy ();

  EnqueueForward (PacketForwardtest);
  //m_queueForward.find (m_Qos)->second->Enqueue (packet);
  /*
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
          
          NS_LOG_DEBUG(Simulator::Now() << ", dst: " << dest << " my address:" << GetAddress()  <<  " ack--route to " << nextHop << ", packet " << packet->GetSize() << ", protocolNumber " << protocolNumber);
           
          if (nextHop == aid - 1 )
            {
              //Ptr<Packet> packetforward = DequeueForward ();
              //NS_LOG_DEBUG(dest << "\t" << GetAddress()  <<  " route ack to  neighbour " );
              //m_DevSameNode->Forward (packetforward, PACKETREPEATNUMBER-1); 
              goto LinkedDevice;
           //return true;
           //return m_DevSameNode->Froward (packet, dest, protocolNumber, true);
           //return m_DevSameNode->SendFromInside (packet, dest, protocolNumber, true);
            }
          else
            {
              goto LinkedDevice;
            }
          return false;
        }
  }
  
LinkedDevice:
      NS_LOG_DEBUG( GetAddress()  <<  " send ack to neighbour " ); 
      if (m_txMachineState == READY)
        {
          Ptr<Packet> packetforward = DequeueForward ();
          if (packetforward == 0)
           {
              return false;
           }
          return TransmitStart (packetforward);
        }
  */

  return ForwardDown ();
}



RemoteSatellite::~RemoteSatellite ()
{
  NS_LOG_FUNCTION (this);
}

RemoteSatelliteTx::~RemoteSatelliteTx ()
{
  NS_LOG_FUNCTION (this);
}




 
} // namespace ns3
