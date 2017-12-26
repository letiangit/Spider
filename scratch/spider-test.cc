/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
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


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <fstream>
#include <sys/stat.h>
#include "ns3/rps.h"
#include <utility> // std::pair
#include <map>
#include "ns3/satposition.h"
#include "ns3/satgeometry.h"



using namespace std;
using namespace ns3;



SatGeometry Geometry;

void
PrintSat (TermSatPosition sat, double interval)
{
    
    coordinate current;
    double longitude, radius, latitude;
    current = sat.GetCoord ();
    longitude = RAD_TO_DEG(Geometry.get_longitude(current, Seconds (0)) );
    latitude = RAD_TO_DEG(Geometry.get_latitude(current) );
    Simulator::Schedule(Seconds(interval), PrintSat, sat, interval);
    std::cout << Simulator::Now ().GetSeconds () << "\t " << current.r << "\t" << RAD_TO_DEG(current.theta) << "\t"  << RAD_TO_DEG(current.phi) << ", lat =" << latitude << ", lon =" << longitude  << std::endl;
}

void
PrintGeo(GeoSatPosition sat, double interval)
{
    coordinate current;
    double longitude, radius, latitude;
    current = sat.GetCoord ();
    longitude = RAD_TO_DEG(Geometry.get_longitude(current, Seconds (0)) );
    latitude = RAD_TO_DEG(Geometry.get_latitude(current) );
    Simulator::Schedule(Seconds(interval), PrintGeo, sat, interval);
    std::cout << Simulator::Now().GetSeconds () <<"\t " << current.r << "\t" << RAD_TO_DEG(current.theta) << "\t"  << RAD_TO_DEG(current.phi) << ", lat =" << latitude << ", lon =" << longitude  << std::endl;
}

void
PrintPloar(PolarSatPosition sat, double interval)
{
    coordinate current;
    double longitude, radius, latitude;
    current = sat.GetCoord ();
    longitude = RAD_TO_DEG(Geometry.get_longitude(current, Seconds (0)) );
    latitude = RAD_TO_DEG(Geometry.get_latitude(current) );
    Simulator::Schedule(Seconds(interval), PrintPloar, sat, interval);
    std::cout << Simulator::Now().GetSeconds () << "\t " << current.r << "\t" << RAD_TO_DEG(current.theta) << "\t"  << RAD_TO_DEG(current.phi) << ", lat =" << latitude << ", lon =" << longitude  << std::endl;
}


void
PrintSunSyn(SunSynStaPosition sat, double interval)
{
    coordinate current;
    double longitude, radius, latitude;
    current = sat.GetCoord ();
    longitude = RAD_TO_DEG(Geometry.get_longitude(current, Seconds (0)) );
    latitude = RAD_TO_DEG(Geometry.get_latitude(current) );
    Simulator::Schedule(Seconds(interval), PrintSunSyn, sat, interval);
    std::cout << Simulator::Now().GetSeconds () << "\t " << current.r << "\t" << RAD_TO_DEG(current.theta) << "\t"  << RAD_TO_DEG(current.phi) << ", lat =" << latitude << ", lon =" << longitude  << std::endl;
}

void
MutuallyVisible(SunSynStaPosition sun, GeoSatPosition geo, TermSatPosition ter, double interval)
{
    coordinate current_geo, current_sun, current_ter;
    double longitude, radius, latitude;
    double elev;
    uint32_t visable;
    
    current_geo = geo.GetCoord ();
    current_sun = sun.GetCoord ();
    current_ter = ter.GetCoord ();
    
    std::cout << "At time (s)----------- " << Simulator::Now().GetSeconds ()  << std::endl;
    visable = SatGeometry::are_satellites_mutually_visible(current_geo, current_sun);
    std::cout << "GEO-SSO " << visable << std::endl;
    
    elev = SatGeometry::check_elevation(current_sun,  current_ter, 0);
    if (RAD_TO_DEG(elev) > 5)
     {
      std::cout << "TER-SSO 1" << std::endl;
     }
    else
     {
      std::cout << "TER-SSO 0" << std::endl;
     }
    
    elev = SatGeometry::check_elevation(current_geo,  current_ter, 0);
    if (RAD_TO_DEG(elev) > 5)
    {
        std::cout << "TER-GEO 1" << std::endl;
    }
    else
    {
        std::cout << "TER-GEO 0" << std::endl;
    }


    Simulator::Schedule(Seconds(interval), MutuallyVisible, sun, geo, ter, interval);
}


int main (int argc, char *argv[])
{

  uint32_t seed = 1;
  double simulationTime;
  simulationTime = 10*24*60*60;

    
  double  TermS_lat = 30;
  double  TermS_lon = 70;

  double  GeoS_lon = 100 ;
    
  double  Polar_Alti = 800;
  double  Polar_lon = 70;
  double  Polar_alpha = 30;
    
  //double interval = 10810.3095141;
    
  double interval = 108;
    
  CommandLine cmd;
  cmd.AddValue ("seed", "random seed", seed);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
    
  cmd.AddValue ("TermS_lat", "Size of payload", TermS_lat);
  cmd.AddValue ("TermS_lon", "number of total stations", TermS_lon);
    
  cmd.AddValue ("GeoS_lon", "number of total stations", GeoS_lon);
    
  cmd.AddValue ("Polar_Alti", "Size of payload", Polar_Alti);
  cmd.AddValue ("Polar_lon", "number of total stations", Polar_lon);
  cmd.AddValue ("Polar_alpha", "Size of payload", Polar_alpha);

  cmd.Parse (argc,argv);

  RngSeedManager::SetSeed (seed);
  
  TermSatPosition TermS;
  GeoSatPosition GeoS;
  PolarSatPosition PolarS;
  SunSynStaPosition SunSynS;
    
  TermS.set (TermS_lat, TermS_lon);
  GeoS.set (GeoS_lon);
  //PolarS.set(Polar_Alti, Polar_lon, Polar_alpha);
    
  SunSynS.set(Polar_Alti, Polar_lon, Polar_alpha);
  
 
  //Simulator::ScheduleNow(PrintSat, TermS, interval);
    
  //Simulator::ScheduleNow(PrintGeo, GeoS, interval);
  
  //Simulator::ScheduleNow(PrintPloar, PolarS, interval);
    
  // Simulator::ScheduleNow(PrintSunSyn, SunSynS, interval);
    
  Simulator::ScheduleNow(MutuallyVisible, SunSynS, GeoS, TermS, interval);
    

    
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();

  Simulator::Destroy ();

return 0;
}
