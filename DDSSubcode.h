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

#ifndef RDAT_DDS_SUBCODE_H
#define RDAT_DDS_SUBCODE_H

#include <stdint.h>

//
// Helper classes for decoding various sub-codes used by the DDS
// standard.
//
class DDSSubcodePack {
protected:
  DDSSubcodePack() : mValid(false) {};
  ~DDSSubcodePack() {};
public:

  bool IsValid() const { return mValid; };

protected:
  bool mValid;
};

//
// 9.4.1 Pack Item No. 1
//
// Describes the current running file number (separator 1) and
// the current running basic group (~128k of data) in that file.
//
class DDSSubcodePack1 : public DDSSubcodePack {
public:
  DDSSubcodePack1() {};
  
  static const unsigned int kID = 1;

  void Decode(const uint8_t *bytes);
  uint8_t  mPosition;
  uint16_t mGroup;
  uint32_t mSeparator1Count;
};


//
// 9.4.2 Pack Item No. 2
//
// Describes the current running separator 2 count (not used in UNIX)
// and record count.
//
class DDSSubcodePack2 : public DDSSubcodePack {
public:
  DDSSubcodePack2() {};
  
  static const unsigned int kID = 2;

  void Decode(const uint8_t *bytes);
  uint8_t  mRepetitions;
  uint16_t mSeparator2Count;
  uint32_t mRecordCount;
};

//
// 9.4.3 Pack Item No. 3
//
// Describes the absolute frame number of this frame (unique for the
// whole tape) and the logical frame number (relative to current basic
// group). It also contains two of the four super-redundant data column
// checksum values for this frame.
//
class DDSSubcodePack3 : public DDSSubcodePack {
public:
  DDSSubcodePack3() {};
  
  static const unsigned int kID = 3;

  void Decode(const uint8_t *bytes);
  uint8_t  mPartitionID;
  uint8_t  mAreaID;
  uint32_t mAbsoluteFrameID;
  uint8_t  mChecksum1;
  uint8_t  mChecksum2;
  uint8_t  mLogicalFrameID;
  bool     mIsLastLogicalFrame;
  bool     mIsECC3Frame;
};

//
// 9.4.4 Pack Item No. 4
//
// Mostly redundant with pack 3 but it contains the two remaining
// column checksum values.
//
class DDSSubcodePack4 : public DDSSubcodePack {
public:
  DDSSubcodePack4() {};
  
  static const unsigned int kID = 4;

  void Decode(const uint8_t *bytes);
  uint8_t  mPartitionID;
  uint8_t  mAreaID;
  uint32_t mAbsoluteFrameID;
  uint8_t  mChecksum3;
  uint8_t  mChecksum4;
  uint8_t  mLogicalFrameID;
  bool     mIsLastLogicalFrame;
  bool     mIsECC3Frame;
};

//
// 9.4.5 Pack Item No. 5
//
// This is a statistics frame that should only appear in the system
// area of the tape (I think). It gives a summary of what was written.
//
class DDSSubcodePack5 : public DDSSubcodePack {
public:
  DDSSubcodePack5() {};
  
  static const unsigned int kID = 5;

  void Decode(const uint8_t *bytes);
  uint32_t mLastRecordedDataGroups;
  uint32_t mTotalRecordedDataGroups;
};

#endif
