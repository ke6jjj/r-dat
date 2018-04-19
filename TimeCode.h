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

#ifndef RDAT_TIMECODE_H
#define RDAT_TIMECODE_H

#include <stdint.h>

class TimeCode {
public:
  TimeCode(const uint8_t *bytes);
  TimeCode(uint32_t absoluteFrame);
  ~TimeCode() {};
  
  enum {
    PROGRAM_NOT_VALID = 0x80aa,
    PROGRAM_LEAD_IN = 0x80bb,
    PROGRAM_LEAD_OUT = 0x80ee
  };
    
  enum {
    INDEX_NOT_VALID = 0xaa
  };
    
  uint16_t Program() const;
  uint8_t Index() const;
  
  uint8_t Hour() const;
  uint8_t Minute() const;
  uint8_t Second() const;
  uint8_t Frame() const;
  uint32_t AbsoluteFrame() const;

protected:
  //
  // The program ID represented in this code.
  //
  uint16_t mProgramId;
  
  //
  // The index ID represented in this code.
  //
  uint8_t mIndexId;
  
  //
  // The hour, minute, second, frame represented by this code.
  //
  uint8_t mHour;
  uint8_t mMinute;
  uint8_t mSecond;
  uint8_t mFrame;
};

#endif
