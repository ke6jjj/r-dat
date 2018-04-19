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

#include "ECCFill_C3.h"

//
// 14.5.3 ECC3
//
// This class encapsulates the way in which DDS's C3 error correction
// scheme interprets the bytes from a BasicGroup object to be corrected.
//
ECCFill_C3::ECCFill_C3(BasicGroup& group)
  : mData(group.ModifiableData()),
    mDataIsValid(group.ModifiableValid()),
    mECCData(group.ModifiableECCData()),
    mECCDataIsValid(group.ModifiableECCValid()),
    mByteSlice(0), mTrackPair(0), mInterleaveSet(0)
{
}

//
// Advance to the next vector in the current basic group.
//
bool
ECCFill_C3::Next()
{
  unsigned int maxTrackPair;
  
  if (End())
    return false;

  //
  // Determine the maximum track pair for the current byte slice.
  //
  if (mByteSlice < (kByteSlices - 1)) {
    //
    // Every byte slice except the last has two track pairs.
    //
    maxTrackPair = 2;
  } else {
    //
    // The last byte slice only has one track pair.
    //
    maxTrackPair = 1;
  }
  
  //
  // Go to the next interleave set in the track pair.
  //
  if (mInterleaveSet == (kInterleaves - 1)) {
    //
    // We're finished with this track pair. Move to the next track
    // pair.
    //
    if (mTrackPair == (maxTrackPair - 1)) {
      //
      // We're finished with the track pairs in this byte slice. Move to
      // the next byte slice.
      //
      mByteSlice++;
      mTrackPair = 0;
    } else {
      mTrackPair++;
    }
    mInterleaveSet = 0;
  } else {
    mInterleaveSet++;
  }
  
  return !End();
}

//
// Returns true if the end of the correction session has been reached.
//
bool
ECCFill_C3::End() const
{
  return mByteSlice >= kByteSlices;
}

static inline size_t
compute_offset(size_t position, unsigned int byteSlice, unsigned int trackPair,
               unsigned int interleave, bool& isECC)
{
  const size_t g1_group = position / 2;
  size_t g1_offset;
  
  if (g1_group == 22) {
    //
    // The ECC group is stored in a separate array, so its offset is actually
    // zero within that array.
    //
    g1_offset = 0;
    isECC = true;
  } else {
    g1_offset = g1_group * DDSGroup1::kSize;
    isECC = false;
  }
  
  if ((position & 1) == 0) {
    return 8 * byteSlice + 2 * (trackPair + 1) + interleave + g1_offset;
  } else {
    return 8 * byteSlice + 6 * trackPair + interleave + g1_offset;
  }
}

uint8_t&
ECCFill_C3::Data(size_t position)
{
  bool isECC;
  const size_t offset = compute_offset(position, mByteSlice, mTrackPair,
    mInterleaveSet, isECC);
  
  if (!isECC)
    return mData[offset];
  else
    return mECCData[offset];
}

bool&
ECCFill_C3::Valid(size_t position)
{
  bool isECC;
  const size_t offset = compute_offset(position, mByteSlice, mTrackPair,
    mInterleaveSet, isECC);

  if (!isECC)
    return mDataIsValid[offset];
  else
    return mECCDataIsValid[offset];
}
