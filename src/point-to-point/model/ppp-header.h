/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 University of Washington
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

#ifndef PPP_HEADER_H
#define PPP_HEADER_H

#include "ns3/header.h"
#include "ns3/mac48-address.h"
#include "ns3/address-utils.h"



namespace ns3 {

/**
 * \ingroup point-to-point
 * \brief Packet header for PPP
 *
 * This class can be used to add a header to PPP packet.  Currently we do not
 * implement any of the state machine in \RFC{1661}, we just encapsulate the
 * inbound packet send it on.  The goal here is not really to implement the
 * point-to-point protocol, but to encapsulate our packets in a known protocol
 * so packet sniffers can parse them.
 *
 * if PPP is transmitted over a serial link, it will typically be framed in
 * some way derivative of IBM SDLC (HDLC) with all that that entails.
 * Thankfully, we don't have to deal with all of that -- we can use our own
 * protocol for getting bits across the serial link which we call an ns3 
 * Packet.  What we do have to worry about is being able to capture PPP frames
 * which are understandable by Wireshark.  All this means is that we need to
 * teach the PcapWriter about the appropriate data link type (DLT_PPP = 9),
 * and we need to add a PPP header to each packet.  Since we are not using
 * framed PPP, this just means prepending the sixteen bit PPP protocol number
 * to the packet.  The ns-3 way to do this is via a class that inherits from
 * class Header.
 */
class PppHeader : public Header
{
public:
    #define CHANNELREQ 17
    #define CHANNELRESP 18
    #define CHANNELCONF 19
    #define CHANNELNOTDEFINED 255

  /**
   * \brief Construct a PPP header.
   */
  PppHeader ();

  /**
   * \brief Destroy a PPP header.
   */
  virtual ~PppHeader ();

  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Get the TypeId of the instance
   *
   * \return The TypeId for this instance
   */
  virtual TypeId GetInstanceTypeId (void) const;


  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

  /**
   * \brief Set the protocol type carried by this PPP packet
   *
   * The type numbers to be used are defined in \RFC{3818}
   *
   * \param protocol the protocol type being carried
   */
  void SetProtocol (uint16_t protocol);

  /**
   * \brief Get the protocol type carried by this PPP packet
   *
   * The type numbers to be used are defined in \RFC{3818}
   *
   * \return the protocol type being carried
   */
  uint16_t GetProtocol (void);
  
  void SetType (uint8_t type);
  void SetQos (uint8_t qos);
  void SetTTL (uint8_t ttl);
  void SetID (uint32_t id);
  void SetSourceAddre (Mac48Address addr);
  void SetDestAddre (Mac48Address addr);
  
  uint8_t GetType (void) const;
  uint8_t GetQos (void) const;
  uint8_t GetTTL (void) const;
  uint32_t GetID (void) const;
  Mac48Address GetSourceAddre (void) const;
  Mac48Address GetDestAddre (void) const;
  
  void SetUsedChannel0(uint8_t channel);
  void SetUsedChannel1(uint8_t channel);
  uint8_t GetUsedChannel0(void) const;
  uint8_t GetUsedChannel1(void) const;


private:

  /**
   * \brief The PPP protocol type of the payload packet
   */
  uint16_t m_protocol;
  
  uint8_t m_type;
  uint8_t m_Qos;
  uint8_t m_ttl;
  uint32_t m_id;
  Mac48Address m_addressSrc;
  Mac48Address m_addressDest;
  uint8_t m_channel0;
  uint8_t m_channel1;
};

} // namespace ns3


#endif /* PPP_HEADER_H */
