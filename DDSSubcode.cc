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

#include "DDSSubcode.h"

static uint16_t DecodeU16(const uint8_t *bytes);
static uint32_t DecodeU24(const uint8_t *bytes);
static uint32_t DecodeU32(const uint8_t *bytes);

//
// 9.4.1 Pack Item No. 1
//
// Describes the current running file number (separator 1) and
// the current running basic group (~128k of data) in that file.
//
void
DDSSubcodePack1::Decode(const uint8_t *bytes)
{
  uint8_t position;
  uint16_t group;
  uint32_t separator_1_count;
  bool changed;
  
  position = bytes[0] & 7;
  group = DecodeU16(&bytes[1]);
  separator_1_count = DecodeU32(&bytes[3]);
  
  mValid = true;
  mPosition = position;
  mGroup = group;
  mSeparator1Count = separator_1_count;
}

//
// 9.4.2 Pack Item No. 2
//
// Describes the current running separator 2 count (not used in UNIX)
// and record count.
//
void
DDSSubcodePack2::Decode(const uint8_t *bytes)
{
  uint8_t repetitions;
  uint16_t separator_2_count;
  uint32_t record_count;
  bool changed;
  
  repetitions = bytes[0] & 7;
  separator_2_count = DecodeU16(&bytes[1]);
  record_count = DecodeU32(&bytes[3]);

  mValid = true;
  mRepetitions = repetitions;
  mSeparator2Count = separator_2_count;
  mRecordCount = record_count;
}

//
// 9.4.3 Pack Item No. 3
//
// Describes the absolute frame number of this frame (unique for the
// whole tape) and the logical frame number (relative to current basic
// group). It also contains two of the four super-redundant data column
// checksum values for this frame.
//
void
DDSSubcodePack3::Decode(const uint8_t *bytes)
{
  uint8_t part;
  uint8_t area_id;
  uint32_t abs_frame_num;
  uint8_t csum1;
  uint8_t csum2;
  uint8_t lfid;
  bool is_last;
  
  part = (bytes[0] & 8) ? 1 : 0;
  area_id = bytes[0] & 7;
  abs_frame_num = DecodeU24(&bytes[1]);
  csum1 = bytes[4];
  csum2 = bytes[5];
  lfid = bytes[6];
  
  mValid = true;
  mPartitionID = part;
  mAreaID = area_id;
  mAbsoluteFrameID = abs_frame_num;
  mChecksum1 = csum1;
  mChecksum2 = csum2;
  mLogicalFrameID = lfid & 0x3f;
  mIsLastLogicalFrame = (lfid & 0x80) == 0x80;
  mIsECC3Frame = (lfid & 0x40) == 0x40;
}

//
// 9.4.4 Pack Item No. 4
//
// Mostly redundant with pack 3 but it contains the two remaining
// column checksum values.
//
void
DDSSubcodePack4::Decode(const uint8_t *bytes)
{
  uint8_t part;
  uint8_t area_id;
  uint32_t abs_frame_num;
  uint8_t csum3;
  uint8_t csum4;
  uint8_t lfid;
  bool is_last;
  
  part = (bytes[0] & 8) ? 1 : 0;
  area_id = bytes[0] & 7;
  abs_frame_num = DecodeU24(&bytes[1]);
  csum3 = bytes[4];
  csum4 = bytes[5];
  lfid = bytes[6];

  mValid = true;
  mPartitionID = part;
  mAreaID = area_id;
  mAbsoluteFrameID = abs_frame_num;
  mChecksum3 = csum3;
  mChecksum4 = csum4;
  mLogicalFrameID = lfid & 0x3f;
  mIsLastLogicalFrame = (lfid & 0x80) == 0x80;
  mIsECC3Frame = (lfid & 0x40) == 0x40;
}

static uint16_t
DecodeU16(const uint8_t *bytes)
{
  return (bytes[0] << 8) |
         (bytes[1]     );
}

static uint32_t
DecodeU24(const uint8_t *bytes)
{
  return (bytes[0] << 16) |
         (bytes[1] <<  8) |
         (bytes[2]      );  
}

static uint32_t
DecodeU32(const uint8_t *bytes)
{
  return (bytes[0] << 24) |
         (bytes[1] << 16) |
         (bytes[2] <<  8) |
         (bytes[3]      );
}
