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

#include <iostream>
#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ppp-header.h"
#include "ns3/mac48-address.h"
#include "ns3/address-utils.h"



namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PppHeader");

NS_OBJECT_ENSURE_REGISTERED (PppHeader);

PppHeader::PppHeader ()
{
    m_Qos = 0;
    m_ttl = 0;
    m_channel0 = CHANNELNOTDEFINED;
    m_channel1 = CHANNELNOTDEFINED;
    m_type = DATATYPE;
}

PppHeader::~PppHeader ()
{
}

TypeId
PppHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PppHeader")
    .SetParent<Header> ()
    .SetGroupName ("PointToPoint")
    .AddConstructor<PppHeader> ()
  ;
  return tid;
}

TypeId
PppHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
PppHeader::Print (std::ostream &os) const
{
  std::string proto;

  switch(m_protocol)
    {
    case 0x0021: /* IPv4 */
      proto = "IP (0x0021)";
      break;
    case 0x0057: /* IPv6 */
      proto = "IPv6 (0x0057)";
      break;
    default:
      NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
  os << "Point-to-Point Protocol: " << proto; 
}

uint32_t
PppHeader::GetSerializedSize (void) const
{
  //return 2; 
 if (m_type == CHANNELREQ)
   {
       return 21; //(2 + 17);
   }
 else if (m_type == LINKSTATE)
   {
       return 31; //(2 + 17);
   }
 else
   {
       return 19; //(2 + 17);
   }
}

void
PppHeader::Serialize (Buffer::Iterator start) const
{
   //start.WriteHtonU16 (m_protocol);
   uint8_t temp;
   //printf (" m_type %d \n", m_type);
   NS_ASSERT (m_type < 64);

   
   temp = ((m_type & 0x3f) << 2 ) |  (m_Qos & 0x03);
   uint32_t temp_id;
   temp_id = (m_ttl << 24 ) |  m_id; 
   
  //NS_LOG_UNCOND ("m_type  " << m_type << ", m_Qos " << m_Qos << ", m_ttl " << uint16_t(m_ttl));

  //NS_LOG_UNCOND ("Serialize temp_id  " << temp_id << ", id " << m_id);
  start.WriteHtolsbU16 (m_protocol);
  start.WriteU8 (temp);
  start.WriteHtolsbU32 (temp_id);
  WriteTo (start, m_addressSrc);
  WriteTo (start, m_addressDest);
  if (m_type == CHANNELREQ)
   {
      start.WriteU8 (m_channel0);
      start.WriteU8 (m_channel1);
   }
  
  if (m_type == LINKSTATE)
   {
      WriteTo (start, m_nextHop0);
      WriteTo (start, m_nextHop1);
   }
  //NS_LOG_UNCOND ("m_protocol " << m_protocol << "\t" << m_addressSrc << "\t" << m_addressDest );
}

uint32_t
PppHeader::Deserialize (Buffer::Iterator start)
{
  //m_protocol = start.ReadNtohU16 ();
  //return GetSerializedSize ();
  
  uint8_t temp;
  uint32_t temp_id;
  
  Buffer::Iterator i = start;

  m_protocol = i.ReadLsbtohU16 ();
  temp = i.ReadU8 ();
  //printf ("Deserialize %d \n", temp);
  m_type = (temp >> 2) & 0x3f;
  m_Qos = temp & 0x03;
  //printf ("m_type %d \n", m_type);
  NS_ASSERT (m_type < 64);
  temp_id = i.ReadLsbtohU32 ();

  m_id = temp_id & 0xffffff;
  m_ttl = (temp_id >> 24) & 0xff;
  ReadFrom (i, m_addressSrc);
  ReadFrom (i, m_addressDest);
  if (m_type == CHANNELREQ)
   {
      m_channel0 = i.ReadU8 ();
      m_channel1 = i.ReadU8 ();
   } 
  
  if (m_type == LINKSTATE)
   {
      ReadFrom (i, m_nextHop0);
      ReadFrom (i, m_nextHop1);
   }
  //NS_LOG_UNCOND ("read m_type  " << m_type << ", m_Qos " << m_Qos << ", m_ttl " << uint16_t(m_ttl));
  return i.GetDistanceFrom (start);

  //NS_LOG_UNCOND ("m_protocol " << m_protocol << "\t" << m_addressSrc << "\t" << m_addressDest );
}

void 
PppHeader::SetProtocol (uint16_t protocol)
{
  m_protocol=protocol;
}

void
PppHeader::SetType (uint8_t type)
{
  m_type = type;
  //std::cout << "m_type is " << m_type << std::endl;
 // NS_LOG_UNCOND("set m_type  " << m_type );
  //printf("SetType %d \n", m_type);
  NS_ASSERT (m_type < 64);
}

void
PppHeader::SetQos (uint8_t qos)
{
  m_Qos=qos;
}

void
PppHeader::SetTTL (uint8_t ttl)
{
  m_ttl=ttl;
}

void
PppHeader::SetID (uint32_t id)
{
  m_id=id;
}

void
PppHeader::SetSourceAddre (Mac48Address addr)
{
  m_addressSrc=addr;
}

void
PppHeader::SetNextHop0 (Mac48Address addr)
{
  m_nextHop0=addr;
}

void
PppHeader::SetNextHop1 (Mac48Address addr)
{
  m_nextHop1=addr;
}

void
PppHeader::SetDestAddre (Mac48Address addr)
{
  m_addressDest=addr;
}


void
PppHeader::SetUsedChannel0(uint8_t channel)
{
  m_channel0 = channel;
}

void
PppHeader::SetUsedChannel1(uint8_t channel)
{
  m_channel1 = channel;
}


uint8_t 
PppHeader::GetUsedChannel0(void) const
{
  return m_channel0;
}

uint8_t
PppHeader::GetUsedChannel1(void) const
{
  return m_channel1;
}


uint8_t
PppHeader::GetType (void) const
{
  //printf("GetType is %d \n", m_type);
  return m_type;
}

uint8_t
PppHeader::GetQos (void) const
{
  return m_Qos;
}

uint8_t
PppHeader::GetTTL (void) const
{
  return m_ttl;
}

uint32_t
PppHeader::GetID (void) const
{
  return m_id;
}

Mac48Address
PppHeader::GetSourceAddre (void) const
{
  return m_addressSrc;
}

Mac48Address
PppHeader::GetNextHop0 (void) const
{
  return m_nextHop0;
}

Mac48Address
PppHeader::GetNextHop1 (void) const
{
  return m_nextHop1;
}

Mac48Address
PppHeader::GetDestAddre (void) const
{
  return m_addressDest;
}

uint16_t
PppHeader::GetProtocol (void)
{
  return m_protocol;
}


} // namespace ns3
