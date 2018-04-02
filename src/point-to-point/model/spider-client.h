/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 *
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 *
 */

#ifndef SPIDER_CLIENT_H
#define SPIDER_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"

#include "ns3/mac48-address.h"
#include "point-to-point-net-device.h"
#include "ns3/net-device.h"

#include <iostream>
#include <fstream>






namespace ns3 {

class Packet;

/**
 * \ingroup udpclientserver
 * \class SpiderClient
 * \brief A Udp client. Sends UDP packet carrying sequence number and time stamp
 *  in their payloads
 *
 */
class SpiderClient : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  SpiderClient ();

  virtual ~SpiderClient ();

  /**
   * \brief set the remote address and port
   * \param ip remote IPv4 address
   * \param port remote port
   */
  void SetRemote (Mac48Address ip);
  //void InstallDevice (Ptr<PointToPointNetDevice> dev);
  //Ptr<PointToPointNetDevice> m_Device;
  
  void InstallDevice (Ptr<NetDevice> dev);
  Ptr<NetDevice> m_Device;
  
  virtual void StartApplication (Time start);
  virtual void StopApplication (Time end);
  virtual void Stop (void);
  virtual void SetMaxPackets (uint32_t max);
  virtual void SetQos (uint32_t qos);
  virtual void SetInterval (Time interval);
  virtual void SetPacketSize (uint32_t size);
  
  virtual void SetConstantRate (bool constant);
  virtual void SetPoissonRate (double rate);
  virtual double nextTime(double rateParameter);
  virtual void SetOutputpath (std::string path);






protected:
  virtual void DoDispose (void);

private:



  /**
   * \brief Send a packet
   */
  void Send (void);

  uint32_t m_count; //!< Maximum number of packets the application will send
  Time m_interval; //!< Packet inter-send time
  uint32_t m_size; //!< Size of the sent packet (including the SeqTsHeader)
  uint32_t m_Qos;

  uint32_t m_sent; //!< Counter for sent packets
  Mac48Address m_peerAddress; //!< Remote peer address
  EventId m_sendEvent; //!< Event to send the next packet
  bool m_constantRate;
  double m_poissonRate; //in the unit of seconds
  uint32_t chunksize;
  std::string m_Outputpath;
  
  std::ofstream myfile;
};

} // namespace ns3

#endif /* SPIDER_CLIENT_H */
