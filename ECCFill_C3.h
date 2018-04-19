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

#ifndef RDAT_ECCFILL_C3_H
#define RDAT_ECCFILL_C3_H

#include "ECCIterator.h"
#include "BasicGroup.h"

//
// 14.5.3 ECC3
//
// This class encapsulates the way in which DDS's C3 error correction
// scheme interprets the bytes from a BasicGroup object to be corrected.
//
class ECCFill_C3 : public ECCIterator {
public:

  //
  // Prepare a DDS ECC3 correction session.
  //
  // The DDS C3 error correction scheme fills its vectors with a 23 G1-
  // group interleave pattern. Since the "BasicGroup" object contains
  // all of these G1 groups concatenated together, we use the BasicGroup
  // object as our data source.
  //
  ECCFill_C3(BasicGroup& group);

  ////////////////////////////////////////////////////////////////////////////
  // Methods from parent ECCIterator interface.
  
  //
  // Advance to the next vector. For C3 this means advancing (least
  // significant to most significant):
  //
  // - The interleave set (even or odd).
  // - The track pair (0 or 1)
  // - The byte slice (0-719)
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
  static const unsigned int kByteSlices = 720;
  static const unsigned int kTrackPairs = 2;
  static const unsigned int kInterleaves = 2;

  //
  // A reference to the underlying data block array from the basic group whence
  // these bytes come and to which they go once they are corrected.
  //
  BasicGroup::DataArray& mData;
  BasicGroup::ValidArray& mDataIsValid;
  BasicGroup::ECCDataArray& mECCData;
  BasicGroup::ECCValidArray& mECCDataIsValid;
  
  //
  // The byte slice within the track pair within the interleave set being
  // evaluated.
  //
  unsigned int mByteSlice;
  
  //
  // The track pair within the interleave set being evaluated.
  //
  unsigned int mTrackPair;
  
  //
  // The interleave set being evaluated.
  //
  unsigned int mInterleaveSet;
};

#endif
