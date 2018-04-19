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

#ifndef RDAT_ECCFILL_C2_H
#define RDAT_ECCFILL_C2_H

#include "ECCIterator.h"
#include "Track.h"

//
// This class encapsulates the way in which DAT's C2 error correction
// scheme interprets the bytes from a Track object to be corrected.
//
class ECCFill_C2 : public ECCIterator {
public:

  //
  // Copy the bytes and erasure indications into a C2 correction vector
  // from the given byte position from the given group.
  //
  ECCFill_C2(Track& track);

  //
  // Query the current position.
  //
  bool CurrentPosition(unsigned int& group, unsigned int& slice);

  ////////////////////////////////////////////////////////////////////////////
  // Methods from parent ECCIterator interface.
  
  //
  // Advance to the next vector. For C2 this means advancing to the next
  // byte slice in this group, or advancing to the next group.
  //
  bool Next();
  
  //
  // Query this iterator to determine if it has completed its run.
  //
  bool End() const;
  
  // End methods from parent ECCIterator interface.
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  // Methods from grandparent ECCFill interface.

  uint8_t& Data(size_t position);
  bool&    Valid(size_t position);

  // End methods from grandparent ECCFill interface.
  ////////////////////////////////////////////////////////////////////////////
  
protected:
  //
  // A reference to the underlying data block array from the track whence
  // these bytes come and to which they go once they are corrected.
  //
  Track::DataArray& mData;
  Track::ValidityArray& mDataIsValid;
  
  //
  // The byte slice within the current group that is being evaluated.
  // Even-numbered groups have 32 byte slices, odd-numbered groups only
  // have 24.
  //
  static const unsigned int kBytesEvenGroup = 32;
  static const unsigned int kBytesOddGroup = 24;
  unsigned int mByteSlice;
  
  //
  // Which group is being interpreted in this fill.
  //
  // The DAT C2 error correction scheme fills its vectors with a 4-block
  // interleave pattern. Each vector therefore belongs to one particular
  // 4-block group.
  //
  static const unsigned int kGroups = 4;
  unsigned int mGroup;
};

#endif
