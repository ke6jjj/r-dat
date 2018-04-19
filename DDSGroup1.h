//
// Copyright 2018, Jeremy Cooper
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#ifndef RDAT_DDS_GROUP_1_H
#define RDAT_DDS_GROUP_1_H

#include <stdint.h>
#include <stddef.h>
#include "DDSGroup3.h"

class DDSGroup1 {
public:
  DDSGroup1(DDSGroup3& g3);
  ~DDSGroup1();
  
  static const size_t kSize = 1439 * 4;
  
  typedef uint8_t DataArray[kSize];
  typedef bool    ValidArray[kSize];
  
  const DataArray& Data() const;
  const ValidArray& Valid() const;
  uint32_t BasicGroupID() const;
  uint8_t  SubFrameID() const;
  bool     IsLastFrame() const;
  bool     IsECCFrame() const;

protected:
  //
  // The data in this group.
  //
  DataArray mData;
  ValidArray mDataIsValid;
  
  //
  // The group and subframe id.
  //
  uint32_t mBasicGroupID;
  uint8_t  mSubFrameID;
  bool     mIsLastFrame;
  bool     mIsECCFrame;
};

#endif
