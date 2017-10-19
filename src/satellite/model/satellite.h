/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef SATELLITE_H
#define SATELLITE_H

#include "ns3/object.h"

namespace ns3 {

/* ... */
class Satellite : public Object
{
 public:
     static TypeId GetTypeId (void);
	Satellite();
        ~Satellite();     
};

}

#endif /* SATELLITE_H */

