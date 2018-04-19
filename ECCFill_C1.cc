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

#include "ECCFill_C1.h"


//
// This class encapsulates the way in which DAT's C1 error correction
// scheme interprets the bytes from a Track object to be corrected.
//
ECCFill_C1::ECCFill_C1(Track& track)
  : mData(track.ModifiableData()),
    mDataIsValid(track.ModifiableDataValid()),
    mBlockPairStart(0), mInterleaveSet(ECCFill_C1::EVEN)
{
}

bool
ECCFill_C1::CurrentPosition(unsigned int& block, unsigned int& offset)
{
  if (End())
    return false;

  block = mBlockPairStart;
  offset = mInterleaveSet == EVEN ? 0 : 1;

  return true;
}

//
// Advance to the next vector in the Track.
//
bool
ECCFill_C1::Next()
{
  if (End())
    return false;
  
  //
  // Choose the next interleave.
  //
  if (mInterleaveSet == ODD) {
    //
    // We're at the ODD set of the current block pair. We're done with
    // this block pair. Go to the EVEN set in the next pair.
    //
    mBlockPairStart += 2;
    mInterleaveSet = EVEN;
  } else {
    //
    // We're in the EVEN set. Go to the ODD set.
    //
    mInterleaveSet = ODD;
  }
  
  return !End();
}

//
// Return true if there are no more vectors to process.
//
bool
ECCFill_C1::End() const
{
  return mBlockPairStart >= Track::kBlocks;
}


//
// Given a C1 codeword position from 0-31, compute the relative block
// number (0-1), and relative byte offset (0-1) within the block, from
// which the byte that goes in this codeword position comes from.
//
static inline void
compute_offsets(size_t position, unsigned int interleave,
                unsigned int start_block, size_t& block, size_t& byte)
{
  block = start_block + position / 16;
  byte = ((position % 16) * 2) + interleave;
}

//
// The C1 error correction scheme works on every pair of blocks within a Track.
//
// The bytes of a block pair are evaluated in an interleaved fashion. The
// even bytes of both blocks go into a single error checking vector, and the
// odd bytes into another.
//
// The last four bytes placed into this vector are the P parity bytes.
//
uint8_t&
ECCFill_C1::Data(size_t position)
{
  size_t block, byte;
  
  compute_offsets(position, mInterleaveSet == ECCFill_C1::EVEN ? 0 : 1,
                  mBlockPairStart, block, byte);
  
  return mData[block][byte];
}

bool&
ECCFill_C1::Valid(size_t position)
{
  size_t block, byte;
  
  compute_offsets(position, mInterleaveSet == ECCFill_C1::EVEN ? 0 : 1,
                  mBlockPairStart, block, byte);
  
  return mDataIsValid[block][byte];
}
