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

#include "ECCFill_C2.h"


//
// This class encapsulates the way in which DAT's C2 error correction
// scheme interprets the bytes from a Track object to be corrected.
//
// For the purposes of C2 error correction, the bytes of a slice
// come from a vertical stripe of bytes taken from every fourth block
// in the data section of a track. There are four such block groups,
// 0, 1, 2 and 3.
//
// Groups 0 and 2 support slices through all 32-bytes of their respective
// blocks. Groups 1 and 3 only support 24-bytes of their respective blocks
// because the remaining eight bytes of groups 1 and 3 are the P parity bytes
// of the C1 scheme. They have already served their purpose and do not
// need futher error correction.
//
// This slicing scheme places the parity bytes of this code, known as the
// Q bytes, directly in the middle of the error checking vector. This
// is a direct result of the placement of the Q bytes in the middle
// blocks of the tracking scheme.
//
ECCFill_C2::ECCFill_C2(Track& track)
  : mData(track.ModifiableData()),
    mDataIsValid(track.ModifiableDataValid()),
    mByteSlice(0), mGroup(0)
{
}

//
// Query the current position.
//
bool
ECCFill_C2::CurrentPosition(unsigned int& group, unsigned int& slice)
{
  if (End())
    return false;

  group = mGroup;
  slice = mByteSlice;

  return true;
}

//
// Advance to the next vector in the current track.
//
bool
ECCFill_C2::Next()
{
  unsigned int maxSlice;
  
  if (End())
    return false;

  //
  // Determine the maximum byte slice number for the current group.
  //
  if ((mGroup & 1) == 0)
    maxSlice = kBytesEvenGroup;
  else
    maxSlice = kBytesOddGroup;
  
  if (mByteSlice == (maxSlice - 1)) {
    //
    // We're at the maximum slice for the current group. Move to the next
    // group.
    //
    mByteSlice = 0;
    mGroup++;
  } else {
    //
    // There are still more slices to process in the current group. Choose
    // the next one.
    //
    mByteSlice++;
  }
  
  return !End();
}

//
// Returns true if the end of the correction session has been reached.
//
bool
ECCFill_C2::End() const
{
  return mGroup >= kGroups;
}


uint8_t&
ECCFill_C2::Data(size_t position)
{
  size_t offset = (size_t) mGroup;
  
  return mData[position*4 + offset][mByteSlice];
}

bool&
ECCFill_C2::Valid(size_t position)
{
  size_t offset = (size_t) mGroup;

  return mDataIsValid[position*4 + offset][mByteSlice];
}
