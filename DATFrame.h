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

#ifndef RDAT_DAT_FRAME_H
#define RDAT_DAT_FRAME_H

#include <stdint.h>
#include "Track.h"

//
// A DAT frame is a pair of tracks, one read from the negative azimuth head
// and one read by the positive azimuth head. Together they form a logical unit
// with a specific interleave pattern and error correction scheme.
//
// Both DAT and DDS use this interleave pattern and correction scheme, so it
// makes sense to centralize it here.
//
class DATFrame {
public:
  DATFrame();
  ~DATFrame();
  
  void FillFromTrackPair(const Track& A, const Track& B);

  bool   OK() const;
  size_t C1Errors() const;
  size_t C1UncorrectableErrors() const;
  size_t C2UncorrectableErrors() const;

  static const size_t kUserDataRows = 1440;
  static const size_t kParityRows = 16;
  static const size_t kBytesPerRow = 4;

  typedef uint8_t DataArray[kUserDataRows+kParityRows][kBytesPerRow];
  typedef bool    ValidityArray[kUserDataRows+kParityRows][kBytesPerRow];

  const DataArray& Data() const;
  const ValidityArray& Valid() const;
  
  DataArray& ModifiableData();
  ValidityArray& ModifiableValidity();
  
protected:
  //
  // The data in this group.
  //
  DataArray mData;
  
  //
  // The validity of each byte of data. (Sometimes the lower-level reading
  // routines can definitively detect that certain bytes were read
  // incorrectly).
  //
  ValidityArray mDataIsValid;

  //
  // The total C1 correctable, uncorrectable and C2 uncorrectable
  // errors found when decoding this frame. Any C2 uncorrectable errors
  // constitute unfixable data corruption; if the frame has no C2
  // uncorrectable errors then everything is intact.
  //
  size_t mC1Errors;
  size_t mC1UncorrectableErrors;
  size_t mC2UncorrectableErrors;
};

#endif
