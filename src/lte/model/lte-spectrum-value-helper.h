/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
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
 * Author: Giuseppe Piro  <g.piro@poliba.it>
 */

#ifndef LTE_SPECTRUM_VALUE_HELPER_H
#define LTE_SPECTRUM_VALUE_HELPER_H


#include <ns3/spectrum-value.h>
#include <vector>

namespace ns3 {


/**
 * \brief This class defines all functions to create spectrum model for lte
 */
class LteSpectrumValueHelper
{
public:


  /**  
   * Calculates the carrier frequency from the E-UTRA Absolute
   * Radio Frequency Channel Number (EARFCN) according to 3GPP TS
   * 36.101 section 5.7.3 "Carrier frequency and EARFCN".
   *
   * \param earfcn the EARFCN
   * 
   * \return the carrier frequency in MHz
   */
  static double GetCarrierFrequency (uint16_t earfcn);

  /**  
   * Calculates the dowlink carrier frequency from the E-UTRA Absolute
   * Radio Frequency Channel Number (EARFCN) using the formula in 3GPP TS
   * 36.101 section 5.7.3 "Carrier frequency and EARFCN".
   *
   * \param earfcn the EARFCN
   * 
   * \return the dowlink carrier frequency in MHz
   */
  static double GetDownlinkCarrierFrequency (uint16_t earfcn);

  /**  
   * Calculates the uplink carrier frequency from the E-UTRA Absolute
   * Radio Frequency Channel Number (EARFCN) using the formula in 3GPP TS
   * 36.101 section 5.7.3 "Carrier frequency and EARFCN".
   *
   * \param earfcn the EARFCN
   * 
   * \return the uplink carrier frequency in MHz
   */
  static double GetUplinkCarrierFrequency (uint16_t earfcn);  

  /**
   * \brief create spectrum value
   * \param powerTx the power transmission in dBm
   * \param channels the list of sub channels where the signal will be sent
   * \return a Ptr to a newly created SpectrumValue instance
   */
  static Ptr<SpectrumValue> CreateDownlinkTxPowerSpectralDensity (double powerTx, std::vector <int> channels);

  /**
   * \brief create spectrum value
   * \param powerTx the power transmission in dBm
   * \param channels the list of sub channels where the signal will be sent
   * \return a Ptr to a newly created SpectrumValue instance
   */
  static Ptr<SpectrumValue> CreateUplinkTxPowerSpectralDensity (double powerTx, std::vector <int> channels);


  /**
   * create a SpectrumValue that models the power spectral density of AWGN
   * 
   * \param noiseFigure the noise figure in dB w.r.t. a reference temperature of 290K
   * 
   * \return a Ptr to a newly created SpectrumValue instance
   */
  static Ptr<SpectrumValue> CreateDownlinkNoisePowerSpectralDensity (double noiseFigure);

  /**
   *  create a SpectrumValue that models the power spectral density of AWGN
   * 
   * \param noiseFigure the noise figure in dB w.r.t. a reference temperature of 290K
   * 
   * \return a Ptr to a newly created SpectrumValue instance
   */
  static Ptr<SpectrumValue> CreateUplinkNoisePowerSpectralDensity (double noiseFigure);

  /** 
   *  create a SpectrumValue that models the power spectral density of AWGN
   * 
   * \param noiseFigure  the noise figure in dB  w.r.t. a reference temperature of 290K
   * \param spectrumModel the SpectrumModel instance to be used
   * 
   * \return a Ptr to a newly created SpectrumValue instance
   */
  static Ptr<SpectrumValue> CreateNoisePowerSpectralDensity (double noiseFigure, Ptr<SpectrumModel> spectrumModel);

};


} // namespace ns3



#endif /*  LTE_SPECTRUM_VALUE_HELPER_H */