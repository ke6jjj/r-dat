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

#include <stdint.h>
#include <string.h>

#include "TimeCode.h"
#include "BCDDecode.h"

TimeCode::TimeCode(const uint8_t *item)
{
  uint8_t pno1 = item[0] & 0x7;
  uint8_t pno23 = item[1];
  
  //
  // Process the program number bits. Certain patterns are
  // special.
  //
  switch (pno23) {
  case 0xaa:
    mProgramId = PROGRAM_NOT_VALID;
    break;
  case 0xbb:
    mProgramId = PROGRAM_LEAD_IN;
    break;
  case 0xee:
    mProgramId = PROGRAM_LEAD_OUT;
    break;
  default:
    mProgramId = ((uint16_t) BCDDecode(pno23)) + 100 * pno1;
    break;
  }
  
  //
  // Process the index id bits.
  //
  uint8_t index_id = item[2];
  if (index_id == 0xaa)
    mIndexId = INDEX_NOT_VALID;
  else
    mIndexId = BCDDecode(item[2]);
  
  mHour = BCDDecode(item[3]);
  mMinute = BCDDecode(item[4]);
  mSecond = BCDDecode(item[5]);
  mFrame = BCDDecode(item[6]);
}

TimeCode::TimeCode(uint32_t absoluteFrame)
  : mIndexId(INDEX_NOT_VALID), mProgramId(PROGRAM_NOT_VALID)
{
  //
  // There are 120000 frames an hour.
  //
  mHour = absoluteFrame / 120000;
  absoluteFrame %= 120000;
  
  //
  // There are 2000 frames every minute.
  //
  mMinute = absoluteFrame / 2000;
  absoluteFrame %= 2000;
  
  //
  // There are 100 frames every three seconds.
  //
  uint8_t second = (absoluteFrame / 100) * 3;
  absoluteFrame %= 100;
  second += absoluteFrame / 33;
  uint8_t frame = absoluteFrame % 33;
  second -= absoluteFrame / 99;
  frame += (absoluteFrame / 99) * 33;
  mSecond = second;
  mFrame = frame;
}


uint16_t
TimeCode::Program() const
{
  return mProgramId;
}

uint8_t
TimeCode::Index() const
{
  return mIndexId;
}

uint8_t
TimeCode::Hour() const
{
  return mHour;
}

uint8_t
TimeCode::Minute() const
{
  return mMinute;
}

uint8_t
TimeCode::Second() const
{
  return mSecond;  
}

uint8_t
TimeCode::Frame() const
{
  return mFrame; 
}

uint32_t
TimeCode::AbsoluteFrame() const
{
  //
  // DAT timecode is structured to tick 100 frames every
  // three seconds. To calculate absolute time, first determine the number
  // of 3-second time periods that have elapsed.
  //
  uint32_t frames = (mSecond / 3) * 100 + mFrame;
  
  //
  // In a three-second group, the first two seconds have 33 frames, in the
  // last second there are 34.
  //
  frames += (mSecond % 3) * 33;
  
  //
  // There are 2000 frames every minute.
  //
  frames += static_cast<uint32_t>(mMinute) * 2000;
  
  //
  // And there are 120000 frames an hour.
  //
  frames += static_cast<uint32_t>(mHour) * 120000;
  
  return frames;
}
