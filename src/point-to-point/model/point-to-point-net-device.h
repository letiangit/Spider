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

#ifndef POINT_TO_POINT_NET_DEVICE_H
#define POINT_TO_POINT_NET_DEVICE_H

#include <cstring>
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/traced-callback.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/ptr.h"
#include "ns3/mac48-address.h"
#include "ns3/event-id.h"
#include "ns3/string.h"
#include "point-to-point-net-device-ges.h"

#include "ns3/spf.h"
#include <map>
#include <list>
#include <utility>

#include "ns3/random-variable-stream.h"




namespace ns3 {


    
//const uint32_t ChannelResp_delay = 21; // second
//const uint32_t ChannelConf_delay = 17; //
//const Time Channel_delay_packet = Seconds (1); // one packet
//const Time ChannelWaiting_Interval = Seconds (1); // 4  channel
//const uint32_t CCmin = 1 ;
//const uint32_t CCmax = 100;
const uint32_t backoffcounter = 20;

class Queue;
class PointToPointChannel;
class PointToPointNetDeviceGES;

class ErrorModel;
class UniformRandomVariable;

 struct RemoteSatelliteARQBuffer;
 struct RemoteSatellite;

 struct RemoteSatelliteARQBufferTx;
 struct RemoteSatelliteTx;




/**
 * \defgroup point-to-point Point-To-Point Network Device
 * This section documents the API of the ns-3 point-to-point module. For a
 * functional description, please refer to the ns-3 manual here:
 * http://www.nsnam.org/docs/models/html/point-to-point.html
 *
 * Be sure to read the manual BEFORE going down to the API.
 */

/**
 * \ingroup point-to-point
 * \class PointToPointNetDevice
 * \brief A Device for a Point to Point Network Link.
 *
 * This PointToPointNetDevice class specializes the NetDevice abstract
 * base class.  Together with a PointToPointChannel (and a peer 
 * PointToPointNetDevice), the class models, with some level of 
 * abstraction, a generic point-to-point or serial link.
 * Key parameters or objects that can be specified for this device 
 * include a queue, data rate, and interframe transmission gap (the 
 * propagation delay is set in the PointToPointChannel).
 */
class PointToPointNetDevice : public NetDevice
{
public:
    #define CHANNELREQ 17
    #define CHANNELRESP 18
    #define CHANNELCONF 19
    #define CHANNELACK 33
    #define DATATYPE 49
    #define LINKSTATE 36 


    #define CHANNELNOTDEFINED 255

    #define CHANNELNUMBER 4
    //#define PACKETREPEATNUMBER 4
    
    //#define V 22
    #define V 24 //two GES station
    #define TTLmax 24 //two GES station

    //#define BUFFERSIZE 100 //should be changed to be configurable
    #define ARQCOUNT 3
    #define DATAACK 43
    #define N_ACK 34

    
    #define ACK_REC 1
    #define TRANSMITTED 2
    #define NO_TRANSMITTED 3
   
    #define NACKNUMSTART 3




  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /**
   * Construct a PointToPointNetDevice
   *
   * This is the constructor for the PointToPointNetDevice.  It takes as a
   * parameter a pointer to the Node to which this device is connected, 
   * as well as an optional DataRate object.
   */
  PointToPointNetDevice ();

  /**
   * Destroy a PointToPointNetDevice
   *
   * This is the destructor for the PointToPointNetDevice.
   */
  virtual ~PointToPointNetDevice ();

  /**
   * Set the Data Rate used for transmission of packets.  The data rate is
   * set in the Attach () method from the corresponding field in the channel
   * to which the device is attached.  It can be overridden using this method.
   *
   * \param bps the data rate at which this object operates
   */
  void SetDataRate (DataRate bps);

  /**
   * Set the interframe gap used to separate packets.  The interframe gap
   * defines the minimum space required between packets sent by this device.
   *
   * \param t the interframe gap time
   */
  void SetInterframeGap (Time t);

  /**
   * Attach the device to a channel.
   *
   * \param ch Ptr to the channel to which this object is being attached.
   * \return true if the operation was successfull (always true actually)
   */
  //bool Attach (Ptr<PointToPointChannel> ch);
  bool Attach (Ptr<PointToPointChannel> ch);
  bool Attach (Ptr<PointToPointChannel> ch, uint16_t rx);


  /**
   * Attach a queue to the PointToPointNetDevice.
   *
   * The PointToPointNetDevice "owns" a queue that implements a queueing 
   * method such as DropTailQueue or RedQueue
   *
   * \param queue Ptr to the new queue.
   */
  void SetQueue (Ptr<Queue> q, Ptr<Queue> queueCritical, Ptr<Queue> queuehighPri, Ptr<Queue> queueBestEff, Ptr<Queue> queueBackGround, 
          Ptr<Queue> queueForward, Ptr<Queue> queueCriticalForward, Ptr<Queue> queuehighPriForward, Ptr<Queue> queueBestEffForward, Ptr<Queue> queueBackGroundForward);


  //void SetQueue (Ptr<Queue> queue, Ptr<Queue> queueCritical, Ptr<Queue> queuehighPri, Ptr<Queue> queueBestEff, Ptr<Queue> queueBackGround); 
  void SetQueue (Ptr<Queue> queue); 

  /**
   * Get a copy of the attached Queue.
   *
   * \returns Ptr to the queue.
   */
  Ptr<Queue> GetQueue (void) const;

  /**
   * Attach a receive ErrorModel to the PointToPointNetDevice.
   *
   * The PointToPointNetDevice may optionally include an ErrorModel in
   * the packet receive chain.
   *
   * \param em Ptr to the ErrorModel.
   */
  void SetReceiveErrorModel (Ptr<ErrorModel> em);

  /**
   * Receive a packet from a connected PointToPointChannel.
   *
   * The PointToPointNetDevice receives packets from its connected channel
   * and forwards them up the protocol stack.  This is the public method
   * used by the channel to indicate that the last bit of a packet has 
   * arrived at the device.
   *
   * \param p Ptr to the received packet.
   */
  void Receive (Ptr<Packet> p);
  void ReceiveChannel (Ptr<Packet> packet, uint32_t linkchannel);
  void WaitChannel ();
  void ChangeTxChannel ();
  void Reset ();


  // The remaining methods are documented in ns3::NetDevice*

  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;

  virtual Ptr<Channel> GetChannel (void) const;

  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;

  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;

  virtual bool IsLinkUp (void) const;

  /**
   * TracedCallback signature for link changed event.
   */
  typedef void (* LinkChangeTracedCallback) (void);
  
  virtual void AddLinkChangeCallback (Callback<void> callback);
  virtual void TryToSetLinkChannel (void);
  virtual void TryToSetLinkChannelFromInside (uint32_t tx, uint32_t rx);
  virtual void TryToSetLinkChannelExternal (void);
  

  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;

  virtual bool IsMulticast (void) const;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;

  virtual bool IsPointToPoint (void) const;
  virtual bool IsBridge (void) const;

  virtual bool Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  virtual bool SendFromInside (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber, bool fromUppper);
  virtual bool SendChannelSelection (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
  virtual void SendChannelRequest (void);
  //virtual void SendChannelResponse ();
  virtual void SendChannelResponse (Mac48Address dest, uint16_t channel0, uint16_t channel1, bool timeout, uint32_t id);
  virtual void SendChannelACK (Mac48Address dest, uint32_t packetid);
  virtual void SendLinkStateUpdate ();
  virtual void Forward (Ptr<Packet> packet, uint16_t count);
  virtual void SetUsedChannelInside (uint32_t tx, uint32_t rx);
  virtual void SetUsedChannelOutside (uint32_t tx, uint32_t rx);
  virtual bool Constrainthold (void);
  virtual uint16_t LookupRoutingTable (Mac48Address addr);
  virtual void CreateRoutingTable (void);
  virtual void InitializeTopology (void);
  virtual void UpdateTopology (Mac48Address src, Mac48Address nexthop0, Mac48Address nexthop1);
  //virtual void SendDataPacket (void);
  virtual Ptr<Packet> PreparePacketToSend (void);
  virtual void setNextHop1 (Mac48Address addr);
  
  void UpdateTopologyGES ();
  
  void LEOInitLinkDst (uint32_t position, uint32_t InitPosGES [], std::map<uint32_t, Mac48Address> DeviceMapPosition, uint32_t NumGES, uint32_t NumLEO, Time interval);
  void LEOupdateLinkDst ();
  


void ARQReceive (Mac48Address src, Mac48Address dst, Time rece, uint32_t size , uint32_t protocol,  uint32_t packetid, Ptr<Packet> packet);
RemoteSatellite* ARQLookup (Mac48Address src, Time rece, uint32_t size , uint32_t protocol, uint32_t packetid)  const;
void CheckAvailablePacket (RemoteSatelliteARQBuffer * buffer, Mac48Address);

RemoteSatelliteTx* ARQLookupTx (Mac48Address dst, uint32_t packetid)  const;
Ptr<Packet> ARQSend (RemoteSatelliteARQBufferTx * buffer, Ptr<Packet> packet);
Ptr<Packet> ARQSend (RemoteSatelliteARQBufferTx * buffer);

//void ARQAckTimeout (RemoteSatelliteARQBufferTx * buffer, uint32_t packetid);
void ARQAckTimeout (RemoteSatelliteARQBufferTx * buffer);



void ARQACKRecevie (Mac48Address dst, uint32_t packetid);
void ARQN_ACKRecevie (Mac48Address dst, uint32_t packetid); 

bool ARQTxBufferCheck (RemoteSatelliteARQBufferTx * buffer) const;
bool ARQRxBufferCheck (RemoteSatelliteARQBuffer * buffer) const;





bool SendAck (const Address &dest, uint16_t protocolNumber, uint32_t packetid, uint32_t ackType, uint32_t qos);




  virtual void SendChannelRequestPacket (uint16_t counter);
  virtual void SendChannelResponsePacket (uint16_t counter);
  virtual void AddDevice (Ptr<PointToPointNetDevice> dev);
  void AddGESDevice (Ptr<PointToPointNetDeviceGES> dev);
  
  void ReceiveFromGES (Ptr<Packet> packet);
  
  void  EnqueueForward (Ptr<Packet> packet);
  Ptr<Packet>  DequeueForward (void);


    







  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);

  virtual bool NeedsArp (void) const;

  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);
  void SetSpiderReceiveCallback (NetDevice::ReceiveCallback cb);


  virtual Address GetMulticast (Ipv6Address addr) const;

  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);

  virtual bool SupportsSendFrom (void) const;
  virtual void  Cancel4Events ();

protected:
  /**
   * \brief Handler for MPI receive event
   *
   * \param p Packet received
   */
  void DoMpiReceive (Ptr<Packet> p);

private:
    
   enum LinkChannelState
    {          
       NO_CHANNEL_CONNECTED,
       EXTERNAL_CHANNEL_COMMAND,
       EXTERNAL_SEND_CHANNEL_REQ,
       EXTERNAL_REC_CHANNEL_RESP,
       EXTERNAL_SEND_CHANNEL_ACK, // nearly
       EXTERNAL_REC_CHANNEL_REQ,
       EXTERNAL_SEND_CHANNEL_RESP,
       EXTERNAL_REC_CHANNEL_ACK,
       INTERNAL_REC_CHANNEL_REQ,
       INTERNAL_SEND_CHANNEL_RESP,
       INTERNAL_REC_CHANNEL_ACK,
       INTERNAL_SEND_CHANNEL_REQ,   // nearly
       INTERNAL_REC_CHANNEL_RESP,
       INTERNAL_SEND_CHANNEL_ACK
    };

    
    LinkChannelState m_state;
    
    virtual void SetState (LinkChannelState state);
    //virtual LinkChannelState GetState (void) const;


  /**
   * \brief Assign operator
   *
   * The method is private, so it is DISABLED.
   *
   * \param o Other NetDevice
   * \return New instance of the NetDevice
   */
  PointToPointNetDeviceGES& operator = (const PointToPointNetDeviceGES &o);
  PointToPointNetDevice& operator = (const PointToPointNetDevice &o);

  /**
   * \brief Copy constructor
   *
   * The method is private, so it is DISABLED.

   * \param o Other NetDevice
   */
  PointToPointNetDevice (const PointToPointNetDevice &o);

  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);
  

private:

  /**
   * \returns the address of the remote device connected to this device
   * through the point to point channel.
   */
  Address GetRemote (void) const;

  /**
   * Adds the necessary headers and trailers to a packet of data in order to
   * respect the protocol implemented by the agent.
   * \param p packet
   * \param protocolNumber protocol number
   */
  void AddHeader (Ptr<Packet> p, uint16_t protocolNumber);
  void AddHeaderChannel (Ptr<Packet> p, uint16_t protocolNumber);

  /**
   * Removes, from a packet of data, all headers and trailers that
   * relate to the protocol implemented by the agent
   * \param p Packet whose headers need to be processed
   * \param param An integer parameter that can be set by the function
   * \return Returns true if the packet should be forwarded up the
   * protocol stack.
   */
  bool ProcessHeader (Ptr<Packet> p, uint16_t& param);

  /**
   * Start Sending a Packet Down the Wire.
   *
   * The TransmitStart method is the method that is used internally in the
   * PointToPointNetDevice to begin the process of sending a packet out on
   * the channel.  The corresponding method is called on the channel to let
   * it know that the physical device this class represents has virtually
   * started sending signals.  An event is scheduled for the time at which
   * the bits have been completely transmitted.
   *
   * \see PointToPointChannel::TransmitStart ()
   * \see TransmitComplete()
   * \param p a reference to the packet to send
   * \returns true if success, false on failure
   */
  bool TransmitStart (Ptr<Packet> p);

  /**
   * Stop Sending a Packet Down the Wire and Begin the Interframe Gap.
   *
   * The TransmitComplete method is used internally to finish the process
   * of sending a packet out on the channel.
   */
  void TransmitComplete (void);

  /**
   * \brief Make the link up and running
   *
   * It calls also the linkChange callback.
   */
  void NotifyLinkUp (void);

  /**
   * Enumeration of the states of the transmit machine of the net device.
   */
  enum TxMachineState
  {
    READY,   /**< The transmitter is ready to begin transmission of a packet */
    BUSY     /**< The transmitter is busy transmitting a packet */
  };
  /**
   * The state of the Net Device transmit state machine.
   */
  TxMachineState m_txMachineState;

  /**
   * The data rate that the Net Device uses to simulate packet transmission
   * timing.
   */
  DataRate       m_bps;

  /**
   * The interframe gap that the Net Device uses to throttle packet
   * transmission
   */
  Time           m_tInterframeGap;

  /**
   * The PointToPointChannel to which this PointToPointNetDevice has been
   * attached.
   */
  //Ptr<PointToPointChannel> m_channel;
  Ptr<PointToPointChannel> m_channel;
  Ptr<PointToPointChannel> m_channelRx;



  /**
   * The Queue which this PointToPointNetDevice uses as a packet source.
   * Management of this Queue has been delegated to the PointToPointNetDevice
   * and it has the responsibility for deletion.
   * \see class DropTailQueue
   */
  Ptr<Queue> m_queue;

  /**
   * Error model for receive packet events
   */
  Ptr<ErrorModel> m_receiveErrorModel;
  uint32_t m_linkchannelRx;
  uint32_t m_linkchannelTx;

  /**
   * The trace source fired when packets come into the "top" of the device
   * at the L3/L2 transition, before being queued for transmission.
   */
  TracedCallback<Ptr<const Packet> > m_macTxTrace;

  /**
   * The trace source fired when packets coming into the "top" of the device
   * at the L3/L2 transition are dropped before being queued for transmission.
   */
  TracedCallback<Ptr<const Packet> > m_macTxDropTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3 
   * transition).  This is a promiscuous trace (which doesn't mean a lot here
   * in the point-to-point device).
   */
  TracedCallback<Ptr<const Packet> > m_macPromiscRxTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3 
   * transition).  This is a non-promiscuous trace (which doesn't mean a lot 
   * here in the point-to-point device).
   */
  TracedCallback<Ptr<const Packet> > m_macRxTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * but are dropped before being forwarded up to higher layers (at the L2/L3 
   * transition).
   */
  TracedCallback<Ptr<const Packet> > m_macRxDropTrace;

  /**
   * The trace source fired when a packet begins the transmission process on
   * the medium.
   */
  TracedCallback<Ptr<const Packet> > m_phyTxBeginTrace;

  /**
   * The trace source fired when a packet ends the transmission process on
   * the medium.
   */
  TracedCallback<Ptr<const Packet> > m_phyTxEndTrace;

  /**
   * The trace source fired when the phy layer drops a packet before it tries
   * to transmit it.
   */
  TracedCallback<Ptr<const Packet> > m_phyTxDropTrace;

  /**
   * The trace source fired when a packet begins the reception process from
   * the medium -- when the simulated first bit(s) arrive.
   */
  TracedCallback<Ptr<const Packet> > m_phyRxBeginTrace;

  /**
   * The trace source fired when a packet ends the reception process from
   * the medium.
   */
  TracedCallback<Ptr<const Packet> > m_phyRxEndTrace;

  /**
   * The trace source fired when the phy layer drops a packet it has received.
   * This happens if the receiver is not enabled or the error model is active
   * and indicates that the packet is corrupt.
   */
  TracedCallback<Ptr<const Packet> > m_phyRxDropTrace;

  /**
   * A trace source that emulates a non-promiscuous protocol sniffer connected 
   * to the device.  Unlike your average everyday sniffer, this trace source 
   * will not fire on PACKET_OTHERHOST events.
   *
   * On the transmit size, this trace hook will fire after a packet is dequeued
   * from the device queue for transmission.  In Linux, for example, this would
   * correspond to the point just before a device \c hard_start_xmit where 
   * \c dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET 
   * ETH_P_ALL handlers.
   *
   * On the receive side, this trace hook will fire when a packet is received,
   * just before the receive callback is executed.  In Linux, for example, 
   * this would correspond to the point at which the packet is dispatched to 
   * packet sniffers in \c netif_receive_skb.
   */
  TracedCallback<Ptr<const Packet> > m_snifferTrace;

  /**
   * A trace source that emulates a promiscuous mode protocol sniffer connected
   * to the device.  This trace source fire on packets destined for any host
   * just like your average everyday packet sniffer.
   *
   * On the transmit size, this trace hook will fire after a packet is dequeued
   * from the device queue for transmission.  In Linux, for example, this would
   * correspond to the point just before a device \c hard_start_xmit where 
   * \c dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET 
   * ETH_P_ALL handlers.
   *
   * On the receive side, this trace hook will fire when a packet is received,
   * just before the receive callback is executed.  In Linux, for example, 
   * this would correspond to the point at which the packet is dispatched to 
   * packet sniffers in \c netif_receive_skb.
   */
  TracedCallback<Ptr<const Packet> > m_promiscSnifferTrace;

  Ptr<Node> m_node;         //!< Node owning this NetDevice
  Mac48Address m_address;   //!< Mac48Address of this NetDevice
  NetDevice::ReceiveCallback m_rxCallback;   //!< Receive callback
  NetDevice::PromiscReceiveCallback m_promiscCallback;  //!< Receive callback
                                                        //   (promisc data)
  NetDevice::ReceiveCallback m_spiderrxCallback;

  uint32_t m_ifIndex; //!< Index of the interface
  bool m_linkUp;      //!< Identify if the link is up or not
  TracedCallback<> m_linkChangeCallbacks;  //!< Callback for the link change event

  static const uint16_t DEFAULT_MTU = 1500; //!< Default MTU

  /**
   * \brief The Maximum Transmission Unit
   *
   * This corresponds to the maximum 
   * number of bytes that can be transmitted as seen from higher layers.
   * This corresponds to the 1500 byte MTU size often seen on IP over 
   * Ethernet.
   */
  uint32_t m_mtu;

  Ptr<Packet> m_currentPkt; //!< Current packet processed

  /**
   * \brief PPP to Ethernet protocol number mapping
   * \param protocol A PPP protocol number
   * \return The corresponding Ethernet protocol number
   */
  static uint16_t PppToEther (uint16_t protocol);

  /**
   * \brief Ethernet to PPP protocol number mapping
   * \param protocol An Ethernet protocol number
   * \return The corresponding PPP protocol number
   */
  static uint16_t EtherToPpp (uint16_t protocol);
  typedef std::map<uint8_t, Ptr<Queue> > Queues;
  Queues m_queueMap;
  Ptr<Queue> m_queueQos3;
  Ptr<Queue> m_queueQos2;
  Ptr<Queue> m_queueQos1;
  Ptr<Queue> m_queueQos0;
  
  Queues m_queueForward;
  Ptr<Queue> m_queueForwardQos4;
  Ptr<Queue> m_queueForwardQos3;
  Ptr<Queue> m_queueForwardQos2;
  Ptr<Queue> m_queueForwardQos1;
  Ptr<Queue> m_queueForwardQos0;
  
  ObjectFactory m_queueFactory;         //!< Queue Factory


  
   EventId m_watingChannelRespEvent;
   EventId m_watingChannelConfEvent;
   EventId m_WaitChannelEvent;
   EventId m_SendChannelRequestPacketEvent;
   EventId m_SendChannelResponsePacketEvent;
   EventId m_watingChannelForwardEvent;
   EventId m_ResetTimeOutEvent;
   Ptr< UniformRandomVariable > m_rng;
   uint16_t m_type;
   //uint16_t m_channel0_used;
   //uint16_t m_channel1_used;
   uint16_t m_channel0_usedInside;
   uint16_t m_channel1_usedInside;
   uint16_t m_channel0_usedOutside;
   uint16_t m_channel1_usedOutside;        
   Mac48Address m_destAddress;
   Mac48Address m_destAddressResp;
   Mac48Address m_destAddressAck;
   Mac48Address m_nextHop0;
   Mac48Address m_nextHop1;
   uint32_t m_packetId;
   uint32_t m_ackid;
   uint32_t m_respid;
   uint32_t m_reqid;
   
   Time ChannelResp_delay; 
   Time ChannelConf_delay; 
   Time Channel_delay_packet; 
   Time ChannelWaiting_Interval; 
   uint32_t CCmax;
   uint32_t backoffcounter;
   Ptr<PointToPointNetDevice> m_DevSameNode;
   Ptr<PointToPointNetDeviceGES> m_GESSameNode;
   Time m_SendChannelResponsePacketTime;
   Time m_SendChannelRequestPacketTime;
   
   Time  m_watingChannelRespTime;  
   Time  m_watingChannelConfTime; 
   bool m_externalChSel;
   bool selectedTraced;
   uint16_t PACKETREPEATNUMBER;
   std::string  m_outputpath;

   
   
   typedef void (* ChannelSelectedCallback)
    (const Mac48Address addr, const Time ts, const uint32_t rx,
     const uint32_t tx);
   
   typedef void (* RecPacketCallback)
    (const Mac48Address addr, const Mac48Address from, const Mac48Address to, const Time ts, const uint32_t rx,
     const uint32_t tx, const uint32_t id);
   
   typedef void (* ARQRecPacketCallback)
    (const Mac48Address addr, const Mac48Address from, const Ptr<Packet> packet, const Time rx, const uint32_t size,
     const uint32_t qos, const uint32_t packetid);
   
   
      typedef void (* ARQTxPacketCallback)
    (const Mac48Address addr, const Mac48Address to, const uint32_t packetid,const uint32_t qos, const uint32_t retrans, const bool success);
     
    TracedCallback<Mac48Address, Time, uint32_t, uint32_t > m_channelSelected;
    TracedCallback<Mac48Address, Mac48Address, Mac48Address, Time, uint32_t, uint32_t, uint32_t > m_recPacketTrace;
    TracedCallback<Mac48Address, Mac48Address, Ptr<Packet>, Time, uint32_t, uint32_t, uint32_t > m_ARQrecPacketTrace; 
    TracedCallback<Mac48Address, Mac48Address, uint32_t, uint32_t, uint32_t, bool > m_ARQTxPacketTrace;
    
    
    
    
    
    
    uint32_t Topology[V][V];
    ShortPath  * CalPath;
    uint32_t * tablePoint;
    uint8_t m_Qos;
    bool m_GESSameNodeFlag;
    
    
  uint32_t m_indicator[V];
  uint32_t m_initPostion;
  uint32_t m_initPosGES[9999];
  uint32_t m_linkDst[9999];
  std::map<uint32_t, Mac48Address>  m_deviceMapPosition;
  uint32_t m_NumGES;
  uint32_t m_NumLEO;
  Time m_dstGESinterval;
  Mac48Address m_dstGESAddr;
  bool m_topologyCompleted;
  bool m_TopoInitialized;
  
  EventId LEOupdateLinkDstEvent;
  
  
   std::list< Mac48Address > RemoteAddressList;
   std::list< Mac48Address >::iterator  RemoteAddressListIterator;

  

 
   
    typedef std::vector <RemoteSatellite *> StatDevice;
    StatDevice m_statDevice;
    
    typedef std::vector <RemoteSatelliteTx *> StatDeviceTx;
    StatDeviceTx m_statDeviceTx;
    
    EventId ARQAckTimeoutEvent;
    Time ARQAckTimer;
    
    uint32_t ARQ_Packetid;
    
    uint32_t BUFFERSIZE;
    
    Ptr<UniformRandomVariable> m_random;  //!< Provides uniform random variables.

};


struct RemoteSatelliteARQBuffer
{
   uint32_t m_bufferStart;
   uint32_t m_bufferSize;
   
   uint32_t m_NACKNumStart;
   
   
  //std::list<Ptr<PointToPointNetDeviceGES> >::iterator m_nodeGESIterator;
  std::list<Ptr<Packet> > * m_listPacket;
  std::list<Time > * m_listRecTime;
  std::list<uint32_t > * m_listSize;
  std::list<uint32_t > * m_listProtocol;
  std::list<uint32_t > * m_listPacketId;
  std::list<uint32_t > * m_listNAackNum; //NACKNUM2903

  
};
    
 struct RemoteSatellite
 {
  virtual ~RemoteSatellite ();
  RemoteSatelliteARQBuffer *m_buffer;
  uint32_t m_retryCount;                  //!< STA short retry count
  Mac48Address m_remoteAddress;
};



struct RemoteSatelliteARQBufferTx
{
   uint32_t m_bufferStart;
   uint32_t m_bufferSize;
   
   
  std::list<Ptr<Packet> > * m_listPacket;
  std::list<Time > * m_listTxTime;
  std::list<uint32_t > * m_listPacketId;
  std::list<uint32_t > * m_listPacketStatus;
  std::list<uint32_t > * m_listPacketRetryNum; 
};
    
 struct RemoteSatelliteTx
 {
  virtual ~RemoteSatelliteTx ();
  RemoteSatelliteARQBufferTx *m_buffer;
  uint32_t m_retryCount;                  //!< STA short retry count
  Mac48Address m_remoteAddress;
};




} // namespace ns3

#endif /* POINT_TO_POINT_NET_DEVICE_H */
