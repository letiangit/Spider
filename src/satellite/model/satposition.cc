/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 * Modified to use staPeriod and offer isascending(), Lloyd Wood, March 2000. */



#include "satposition.h"
#include "satgeometry.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "ns3/simulator.h"

namespace ns3 {
SatPosition::SatPosition(Time initial)
{
    initialTime = initial;
    
}


SatPosition::SatPosition()
{    
}



Time
SatPosition::TimeElapse ()
{
   //std::cout << "initialTime " << initialTime << std::endl;
   return (Simulator::Now () - initialTime);
}


/////////////////////////////////////////////////////////////////////
// class TermSatPosition
/////////////////////////////////////////////////////////////////////

// Specify initial coordinates.  Default coordinates place the terminal
// on the Earth's surface at 0 deg lat, 0 deg long.


//TermSatPosition::TermSatPosition(double latitude, double longitude) 
TermSatPosition::TermSatPosition()
{
	initialCoord.r = EARTH_RADIUS;
	staPeriod = EARTH_PERIOD; // seconds
	staType = POSITION_SAT_TERM;
}



//
// Convert user specified latitude and longitude to our spherical coordinates
// Latitude is in the range (-90, 90) with neg. values -> south
// staPeriod.theta is stored from 0 to PI (spherical)
// Longitude is in the range (-180, 180) with neg. values -> west
// staPeriod.phi is stored from 0 to 2*PI (spherical)
//
//Good
void 
TermSatPosition::set(double latitude, double longitude)
{
	if (latitude < -90 || latitude > 90)
		fprintf(stderr, "TermSatPosition:  latitude out of bounds %f\n",
		   latitude);
        
	if (longitude < -180 || longitude > 180)
		fprintf(stderr, "TermSatPosition: longitude out of bounds %f\n",
		    longitude);
        
	initialCoord.theta = DEG_TO_RAD(90 - latitude);
	if (longitude < 0)
		initialCoord.phi = DEG_TO_RAD(360 + longitude);
	else
		initialCoord.phi = DEG_TO_RAD(longitude);
        
        NS_ASSERT (initialCoord.theta <= PI);
        NS_ASSERT (initialCoord.theta >= 0);
        
        NS_ASSERT (initialCoord.phi <= 2*PI);
        NS_ASSERT (initialCoord.phi >= 0);
}


//Get coord, Le
//Earth self rotating 
coordinate 
TermSatPosition::GetCoord()
{
	coordinate current;
        double partial;  // fraction of orbit period completed
        
        
	current.r = initialCoord.r;
	current.theta = initialCoord.theta;
        //std::cout << "current.theta " << current.theta << "\t" << RAD_TO_DEG(current.theta) << std::endl;
        
	//current.phi = fmod((initialCoord.phi + 
	 //   (fmod(NOW + time_advance_,staPeriod)/staPeriod) * 2*PI), 2*PI);
         
        partial = TimeElapse ().GetSeconds ()/staPeriod * 2*PI;
        current.phi = fmod(initialCoord.phi + partial, 2*PI);
        
        NS_ASSERT (current.theta <= PI);
        NS_ASSERT (current.theta >= 0);
        
        NS_ASSERT (current.phi <= 2*PI);
        NS_ASSERT (current.phi >= 0);
        
	return current;
}

/////////////////////////////////////////////////////////////////////
// class PolarSatPosition
/////////////////////////////////////////////////////////////////////
PolarSatPosition::PolarSatPosition(double altitude, double Inc, double Lon, 
    double Alpha, double Plane) : next_(0), plane_(0) 
{
	set(altitude, Lon, Alpha, Inc);
	plane_ = int(Plane);
	staType = POSITION_SAT_POLAR;
}

//
// Since it is easier to compute instantaneous orbit position based on a
// coordinate system centered on the orbit itself, we keep initial coordinates
// specified in terms of the satellite orbit, and convert to true spherical 
// coordinates in coord().
// Initial satellite position is specified as follows:
// initialCoord.theta specifies initial angle with respect to "ascending node"
// i.e., zero is at equator (ascending)-- this is the $alpha parameter in Otcl
// initialCoord.phi specifies East longitude of "ascending node"  
// -- this is the $lon parameter in OTcl
// Note that with this system, only theta changes with time
//
void 
PolarSatPosition::set(double Altitude, double Lon, double Alpha, double Incl)
{
	if (Altitude < 0) 
        {
		fprintf(stderr, "PolarSatPosition:  altitude out of \
		   bounds: %f\n", Altitude);
		exit(1);
	}
	initialCoord.r = Altitude + EARTH_RADIUS; // Altitude in km above the earth
        
	if (Alpha < 0 || Alpha >= 360) 
        {
		fprintf(stderr, "PolarSatPosition:  alpha out of bounds: %f\n", 
		    Alpha);
		exit(1);
	}
        
	initialCoord.theta = DEG_TO_RAD(Alpha); // larger than pi?
        
	if (Lon < -180 || Lon > 180) {
		fprintf(stderr, "PolarSatPosition:  lon out of bounds: %f\n", 
		    Lon);
		exit(1);
	}
        
	if (Lon < 0)
		initialCoord.phi = DEG_TO_RAD(360 + Lon);
	else
		initialCoord.phi = DEG_TO_RAD(Lon);
        
        
	if (Incl < 0 || Incl > 180) {
		// retrograde orbits = (90 < Inclination < 180)
		fprintf(stderr, "PolarSatPosition:  inclination out of \
		    bounds: %f\n", Incl); 
		exit(1);
	}
	inclination_ = DEG_TO_RAD(Incl);
        
	double num = initialCoord.r * initialCoord.r * initialCoord.r;
	staPeriod = 2 * PI * sqrt(num/MU); // seconds 
        //staPeriod=10810.3095141;
         std::cout << "SunSynStaPosition/polar staPeriod (h)" << staPeriod  << "\t" << initialCoord.r << ", initialCoord.theta " << (initialCoord.theta * 180)/PI << std::endl;
         std::cout << "SunSynStaPosition/polar inclination_" << inclination_  << std::endl;

}


//
// The initial coordinate has the following properties:
// theta: 0 < theta < 2 * PI (essentially, this specifies initial position)  
// phi:  0 < phi < 2 * PI (longitude of ascending node)
// Return a coordinate with the following properties (i.e. convert to a true
// spherical coordinate):
// theta:  0 < theta < PI
// phi:  0 < phi < 2 * PI
//
coordinate 
PolarSatPosition::GetCoord ()
{
	coordinate current;
	double partial;  // fraction of orbit period completed
        partial = fmod (TimeElapse ().GetSeconds (), staPeriod)/staPeriod * 2*PI;
        //std::cout <<  Simulator::Now ().GetSeconds () << "--partial " << RAD_TO_DEG(partial) << std::endl;

        
	double theta_cur, phi_cur, theta_new, phi_new;
        
  

	// Compute current orbit-centric coordinates:
	// theta_cur adds effects of time (orbital rotation) to initialCoord.theta
	theta_cur = fmod(initialCoord.theta + partial, 2*PI); //used for both theta and phi. Le
	phi_cur = initialCoord.phi;

	// Reminder:  theta_cur and phi_cur are temporal translations of 
	// initial parameters and are NOT true spherical coordinates.
	//
	// Now generate actual spherical coordinates,
	// with 0 < theta_new < PI and 0 < phi_new < 360
        
        //std::cout << "partial " << partial << "\t" << Simulator::Now().GetSeconds () << "\t " << current.r << "\t" << RAD_TO_DEG(theta_cur) << "\t"  << RAD_TO_DEG(phi_cur)   << std::endl;


	assert (inclination_ < PI);

	// asin returns value between -PI/2 and PI/2, so 
	// theta_new guaranteed to be between 0 and PI
        //std::cout <<  Simulator::Now ().GetSeconds () << "--theta_cur " << RAD_TO_DEG(theta_cur) << std::endl;
	theta_new = PI/2 - asin(sin(inclination_) * sin(theta_cur)); // should theta be [-90,90]?
	// if theta_new is between PI/2 and 3*PI/2, must correct
	// for return value of atan()
                //std::cout <<  Simulator::Now ().GetSeconds () << "--theta_new " << RAD_TO_DEG(theta_new) << std::endl;

        
         //std::cout << "........" << Simulator::Now().GetSeconds () <<  " theta_cur " << RAD_TO_DEG(theta_cur) << " phi_cur "  << RAD_TO_DEG(phi_cur)   << std::endl;

	if (theta_cur > PI/2 && theta_cur < 3*PI/2)
		phi_new = atan(cos(inclination_) * tan(theta_cur)) + 
			phi_cur + PI;
	else
		phi_new = atan(cos(inclination_) * tan(theta_cur)) + 
			phi_cur;
         
                  //std::cout << "===" << Simulator::Now().GetSeconds () <<  " theta_cur " << RAD_TO_DEG(theta_cur) << " phi_new "  << RAD_TO_DEG(phi_new)   << std::endl;

	phi_new = fmod(phi_new + 2*PI, 2*PI);
	
	current.r = initialCoord.r;
	current.theta = theta_new;
	current.phi = phi_new;
        
        //std::cout << "====" << Simulator::Now().GetSeconds () << "\t " << current.r << "\t" << RAD_TO_DEG(current.theta) << "\t"  << RAD_TO_DEG(current.phi)   << std::endl;
             //NS_ASSERT (initialCoord.theta <= PI);
      
        NS_ASSERT (current.theta >= 0);
        NS_ASSERT (current.theta <= PI);
        NS_ASSERT (current.phi <= 2*PI);
        NS_ASSERT (current.phi >= 0);
        
	return current;
}


//
// added by Lloyd Wood, 27 March 2000.
// allows us to distinguish between satellites that are ascending (heading north)
// and descending (heading south).
//
bool
PolarSatPosition::isascending()
{	
	double partial;
        
        partial = fmod (TimeElapse ().GetSeconds (), staPeriod) * 2*PI;

	double theta_cur= fmod(initialCoord.theta + partial, 2*PI);
        
	if ((theta_cur > PI/2)&&(theta_cur < 3*PI/2)) 
        {
		return 0;
	}
        else 
        {
		return 1;
	}
}

uint8_t 
PolarSatPosition::plane() 
{ return plane_; }


void 
SunSynStaPosition::set(double Altitude, double Lon, double Alpha)  
{
    double cosinc;
    initialCoord_r = Altitude + EARTH_RADIUS;
    cosinc = -1.0 * pow(initialCoord_r/12352.0, 3.5);
    
    //cosinc=2*PI*pow(initialCoord_r,1.5)*pow(MU,-0.5);
    //cosinc=-1.0*pow(cosinc/(3.795*3600),7.0/3);
            
    inclination_ = acos (cosinc) * 180 / PI;
    PolarSatPosition::set( Altitude,  Lon,  Alpha, inclination_);
    std::cout << "SunSynStaPosition inclination_ " << RAD_TO_DEG(inclination_)  << ", Alpha " << Alpha << std::endl;
}


coordinate 
SunSynStaPosition::GetCoord ()
{
	coordinate current;
	double partial;  // fraction of orbit period completed
        partial = fmod (TimeElapse ().GetSeconds (), staPeriod)/staPeriod * 2*PI;
        //std::cout <<  Simulator::Now ().GetSeconds () << "--partial " << RAD_TO_DEG(partial) << std::endl;

        
	double theta_cur, phi_cur, theta_new, phi_new;
        double theta_incr; 
  

	// Compute current orbit-centric coordinates:
	// theta_cur adds effects of time (orbital rotation) to initialCoord.theta
	theta_cur = fmod(initialCoord.theta + partial, 2*PI); //used for both theta and phi. Le
        theta_incr = fmod(partial, 2*PI); //used for both theta and phi. Le

	phi_cur = initialCoord.phi;

	// Reminder:  theta_cur and phi_cur are temporal translations of 
	// initial parameters and are NOT true spherical coordinates.
	//
	// Now generate actual spherical coordinates,
	// with 0 < theta_new < PI and 0 < phi_new < 360
        
        //std::cout << "partial " << partial << "\t" << Simulator::Now().GetSeconds () << "\t " << current.r << "\t" << RAD_TO_DEG(theta_cur) << "\t"  << RAD_TO_DEG(phi_cur)   << std::endl;


	assert (inclination_ < PI);

	// asin returns value between -PI/2 and PI/2, so 
	// theta_new guaranteed to be between 0 and PI
        //std::cout <<  Simulator::Now ().GetSeconds () << "--theta_cur " << RAD_TO_DEG(theta_cur) << std::endl;
	theta_new = PI/2 - asin(sin(inclination_) * sin(theta_incr) + sin(initialCoord.theta)); // should theta be [-90,90]?
	// if theta_new is between PI/2 and 3*PI/2, must correct
	// for return value of atan()
                //std::cout <<  Simulator::Now ().GetSeconds () << "--theta_new " << RAD_TO_DEG(theta_new) << std::endl;

        
         //std::cout << "........" << Simulator::Now().GetSeconds () <<  " theta_cur " << RAD_TO_DEG(theta_cur) << " phi_cur "  << RAD_TO_DEG(phi_cur)   << std::endl;

	if (theta_incr > PI/2 && theta_incr < 3*PI/2)
		phi_new = atan(cos(inclination_) * tan(theta_incr)) + 
			phi_cur + PI;
	else
		phi_new = atan(cos(inclination_) * tan(theta_incr)) + 
			phi_cur;
         
                  //std::cout << "===" << Simulator::Now().GetSeconds () <<  " theta_cur " << RAD_TO_DEG(theta_cur) << " phi_new "  << RAD_TO_DEG(phi_new)   << std::endl;

	phi_new = fmod(phi_new + 2*PI, 2*PI);
	
	current.r = initialCoord.r;
	current.theta = theta_new;
	current.phi = phi_new;
        
        //std::cout << "====" << Simulator::Now().GetSeconds () << "\t " << current.r << "\t" << RAD_TO_DEG(current.theta) << "\t"  << RAD_TO_DEG(current.phi)   << std::endl;
             //NS_ASSERT (initialCoord.theta <= PI);
      
        //NS_ASSERT (current.theta >= 0);
        //NS_ASSERT (current.theta <= PI);
        NS_ASSERT (current.phi <= 2*PI);
        NS_ASSERT (current.phi >= 0);
        
	return current;
}


    
/////////////////////////////////////////////////////////////////////
// class GeoSatPosition
/////////////////////////////////////////////////////////////////////
// Good
GeoSatPosition::GeoSatPosition(double longitude) 
{
	initialCoord.r = EARTH_RADIUS + GEO_ALTITUDE;
	initialCoord.theta = PI/2;
	set(longitude);
	staType = POSITION_SAT_GEO;
	staPeriod = EARTH_PERIOD;    
}

coordinate 
GeoSatPosition::GetCoord()
{
	coordinate current;
	current.r = initialCoord.r;
	current.theta = initialCoord.theta;
        double partial;  // fraction of orbit period completed
        partial = fmod(TimeElapse ().GetSeconds (), staPeriod)/staPeriod * 2*PI;
        current.phi = fmod(initialCoord.phi+partial, 2*PI);
        
        NS_ASSERT (initialCoord.phi <= 2*PI);
        NS_ASSERT (initialCoord.phi >= 0);
        
	return current;
}

//
// Longitude is in the range (0, 180) with negative values -> west
// Good
void 
GeoSatPosition::set(double longitude)
{
	if (longitude < -180 || longitude > 180)
		fprintf(stderr, "GeoSatPosition:  longitude out of bounds %f\n",
		    longitude);
        
	if (longitude < 0)
		initialCoord.phi = DEG_TO_RAD(360 + longitude);
	else
		initialCoord.phi = DEG_TO_RAD(longitude);
                
        NS_ASSERT (initialCoord.phi <= 2*PI);
        NS_ASSERT (initialCoord.phi >= 0);
}

} //namespace ns3

 
