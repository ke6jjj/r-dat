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

#ifndef RDAT_DAT_BLOCK_H
#define RDAT_DAT_BLOCK_H

#include <stdint.h>
#include <stddef.h>

//
// A DAT block is a small packet of 35 bytes, generally bounded
// by a SYNC word. DAT blocks are common to both DAT and DDS.
//
// In this class we retain more than just the 8-bit bytes in the
// block. We also retain the SYNC word and the 10-bit raw line
// words. This allows for something further downstream to reinterpret
// errors if need be.
//
class DATBlock {
public:
  DATBlock();
  DATBlock(const DATBlock& copy);
  ~DATBlock();

  //
  // Reset, forgetting any current contents.
  //
  void Reset();

  //
  // Attempt to append a received byte to this block.
  //
  // Returns True if, after the reception, the block is now
  // complete.
  //
  bool AddByte(uint16_t raw_word, uint16_t flagged_byte);

  //
  // The number of bytes in the block.
  //
  size_t Size() const;

  //
  // Get access to the underlying 10-bit and 8-bit flagged
  // bytes, respectively.
  //
  const uint16_t * LineWords() const;
  const uint16_t * FlaggedBytes() const;
  
  enum { INVALID = 0x8000 };

protected:
  //
  // The ten-bit raw words received in this block. (Including
  // space for the SYNC word that starts the block.)
  //
  uint16_t mLineWords[36];

  //
  // The decoded 8-bit bytes and decoding flags for the ten-bit
  // words received.
  //
  uint16_t mBytes[36];

  //
  // The number of bytes received so far.
  //
  size_t mByteCount;
};

#endif
