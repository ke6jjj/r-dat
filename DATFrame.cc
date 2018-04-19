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

#include <string.h>
#include <stdio.h>
#include "DATFrame.h"


//
// A DAT frame is a pair of tracks, one read from the negative azimuth head
// and one read by the positive azimuth head. Together the form a logical unit
// with a specific interleave pattern and error correction scheme.
//
// Both DAT and DDS use this interleave pattern and correction scheme, so it
// makes sense to centralize it here.
//
DATFrame::DATFrame() : mC1Errors(0), mC1UncorrectableErrors(0),
  mC2UncorrectableErrors(0)
{
  size_t i;
  
  memset(mDataIsValid, 0, sizeof(mDataIsValid));
}

DATFrame::~DATFrame()
{
}

//
// Fill in a frame from a track pair.
//
void
DATFrame::FillFromTrackPair(const Track& A, const Track& B)
{
  bool everything_ok = true;

  //
  // Retrieve pointers to the data and meta-data in the A and B tracks passed
  // by the caller.
  //
  const Track::DataArray& a_bytes = A.Data();
  const Track::DataArray& b_bytes = B.Data();
  const Track::ValidityArray& a_valid = A.DataValid();
  const Track::ValidityArray& b_valid = B.DataValid();
  
  //
  // De-multiplex the bytes from the A and B tracks into their respective
  // positions in the frame.
  //
  // (Taken from DDS spec Section 9.3.4 G4 Sub-Group, there is likely the same
  // logic specified in the DAT Conference Standard as well).
  //
  for (size_t column = 0; column < 2; column++) {
    for (size_t word = 0; word < 1456; word++) {
      size_t source_block =
                                 (word %  52) + 
                            75 * (word %   2) +
                                 (word / 832);
      size_t u = (column + 1) % 2;
      size_t source_byte =
                            2  * (u + word /  52)     -
                                 (    word /  52) % 2 -
                            32 * (    word / 832);
      
      if ((word % 2) == 0) {
        mData[word][column]   = a_bytes[source_block][source_byte];
        mData[word][column+2] = b_bytes[source_block][source_byte];
        mDataIsValid[word][column]   = a_valid[source_block][source_byte];
        mDataIsValid[word][column+2] = b_valid[source_block][source_byte];
      } else {
        mData[word][column]   = b_bytes[source_block][source_byte];
        mData[word][column+2] = a_bytes[source_block][source_byte];
        mDataIsValid[word][column]   = b_valid[source_block][source_byte];
        mDataIsValid[word][column+2] = a_valid[source_block][source_byte];
      }
      everything_ok = everything_ok && mDataIsValid[word][column] &&
                      mDataIsValid[word][column+2];
    }
  }

  //
  // Gather up error statistics from the two constituent tracks.
  //
  mC1Errors = A.C1Errors() + B.C1Errors();
  mC1UncorrectableErrors = A.C1UncorrectableErrors() +
                           B.C1UncorrectableErrors();
  mC2UncorrectableErrors = A.C2UncorrectableErrors() +
                           B.C2UncorrectableErrors();

  if (mC2UncorrectableErrors && everything_ok) {
    //
    // This is supposed to be impossible.
    //
    printf("Inconsistency in C2 errors and erasures!!\n");
  }
}

const DATFrame::DataArray&
DATFrame::Data() const
{
  return mData;
}

const DATFrame::ValidityArray&
DATFrame::Valid() const
{
  return mDataIsValid;
}

DATFrame::DataArray&
DATFrame::ModifiableData()
{
  return mData;
}

DATFrame::ValidityArray&
DATFrame::ModifiableValidity()
{
  return mDataIsValid;
}

bool
DATFrame::OK() const
{
  return mC2UncorrectableErrors == 0;
}

size_t
DATFrame::C1Errors() const
{
  return mC1Errors;
}

size_t
DATFrame::C1UncorrectableErrors() const
{
  return mC1UncorrectableErrors;
}

size_t
DATFrame::C2UncorrectableErrors() const
{
  return mC2UncorrectableErrors;
}
