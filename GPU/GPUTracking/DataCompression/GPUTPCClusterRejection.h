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

/// \file GPUTPCClusterRejection.h
/// \author David Rohr

#ifndef GPUTPCCLUSTERREJECTION_H
#define GPUTPCCLUSTERREJECTION_H

#include "GPUTPCGMMergerTypes.h"

namespace GPUCA_NAMESPACE
{
namespace gpu
{
struct GPUTPCClusterRejection {
  template <bool C, class T = void, class S = void>
  static constexpr inline bool GetProtectionStatus(int attach, bool& physics, bool& protect, T* counts = nullptr, S* mev200 = nullptr)
  {
    (void)counts; // Avoid incorrect -Wunused-but-set-parameter warning
    (void)mev200;
    if (attach == 0) {
      return false;
    } else if ((attach & gputpcgmmergertypes::attachGoodLeg) == 0) {
      if constexpr (C) {
        counts->nLoopers++;
      }
      return true;
    } else if (attach & gputpcgmmergertypes::attachHighIncl) {
      if constexpr (C) {
        counts->nHighIncl++;
      }
      return true;
    } else if (attach & gputpcgmmergertypes::attachTube) {
      protect = true;
      if constexpr (C) {
        if (*mev200) {
          counts->nTube200++;
        } else {
          counts->nTube++;
        }
      }
      return false;
    } else if ((attach & gputpcgmmergertypes::attachGood) == 0) {
      protect = true;
      if constexpr (C) {
        counts->nRejected++;
      }
      return false;
    } else {
      physics = true;
      return false;
    }
  }

  static constexpr inline bool GetIsRejected(int attach)
  {
    bool physics = false, protect = false;
    return GetProtectionStatus<false>(attach, physics, protect);
  }
};
} // namespace gpu
} // namespace GPUCA_NAMESPACE

#endif
