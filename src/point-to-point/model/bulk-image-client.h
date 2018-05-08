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

#ifndef BULK_IMAGE_CLIENT_H
#define BULK_IMAGE_CLIENT_H

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
class BulkImageClient : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  BulkImageClient ();

  virtual ~BulkImageClient ();

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
  
  virtual void SetImageBundleCount (uint32_t max);
  virtual void SetImageQoS (uint32_t qos);
  virtual void SetImageBundleSize (uint32_t size);
  
  virtual void SetPriorityBundleCount (uint32_t max);
  virtual void SetPriorityQoS (uint32_t qos);
  virtual void SetPriorityBundleSize (uint32_t size);
  
  virtual void SetTMTCBundleCount (uint32_t max);
  virtual void SetTMTCQoS (uint32_t qos);
  virtual void SetTMTCBundleSize (uint32_t size);
  
  virtual void SetPriorityDelay (Time delay);
  virtual void SetTMTCDelay (Time delay);
  
  virtual void EnableImage(bool enable);
  virtual void EnablePriority(bool enable);
  virtual void EnableTMTC(bool enable);
  
  virtual void SetOutputpath (std::string path);



protected:
  virtual void DoDispose (void);

private:



  /**
   * \brief Send a packet
   */
  void SendImage();
  void SendPriorityTraffic();
  void SendTMTCTraffic();

  uint32_t m_img_bundle_count;
  uint32_t m_img_qos;
  uint32_t m_img_bundle_size;
  
  Time m_prio_delay;
  
  uint32_t m_prio_bundle_count;
  uint32_t m_prio_qos;
  uint32_t m_prio_bundle_size; 
  
  Time m_tmtc_delay;
  
  uint32_t m_tmtc_bundle_count;
  uint32_t m_tmtc_qos;
  uint32_t m_tmtc_bundle_size; 
  
  bool m_tmtc_enabled;
  bool m_img_enabled;
  bool m_prio_enabled;
  
  Mac48Address m_peerAddress; //!< Remote peer address
  EventId m_prioritySendEvent; //!< Event to send the next packet
  EventId m_imageSendEvent; //!< Event to send the next packet
  EventId m_tmtcSendEvent; //!< Event to send the next packet
  bool m_constantRate;
  uint32_t chunksize;
  std::string m_Outputpath;
  std::ofstream myfile;
};

} // namespace ns3

#endif /* BULK_IMAGE_CLIENT_H */
