//**************************************************************************\
//* This file is property of and copyright by the ALICE Project            *\
//* ALICE Experiment at CERN, All rights reserved.                         *\
//*                                                                        *\
//* Primary Authors: Matthias Richter <Matthias.Richter@ift.uib.no>        *\
//*                  for The ALICE HLT Project.                            *\
//*                                                                        *\
//* Permission to use, copy, modify and distribute this software and its   *\
//* documentation strictly for non-commercial purposes is hereby granted   *\
//* without fee, provided that the above copyright notice appears in all   *\
//* copies and that both the copyright notice and this permission notice   *\
//* appear in the supporting documentation. The authors make no claims     *\
//* about the suitability of this software for any purpose. It is          *\
//* provided "as is" without express or implied warranty.                  *\
//**************************************************************************

/// \file GPUParamRTC.h
/// \author David Rohr

#ifndef GPUPARAMRTC_H
#define GPUPARAMRTC_H

#include "GPUParam.h"
#include <string>

namespace GPUCA_NAMESPACE
{
namespace gpu
{
namespace gpu_rtc
{
#define QCONFIG_GENRTC
#define BeginNamespace(...)
#define EndNamespace(...)
#include "utils/qconfig.h"
#undef QCONFIG_GENRTC
#undef BeginNamespace
#undef EndNamespace
} // namespace gpu_rtc

struct GPUParamRTC : public internal::GPUParam_t<gpu_rtc::GPUSettingsRec, gpu_rtc::GPUSettingsParam> {
  void setFrom(const GPUParam& param);
  static std::string generateRTCCode(const GPUParam& param, bool useConstexpr);
};

} // namespace gpu
} // namespace GPUCA_NAMESPACE

#endif
