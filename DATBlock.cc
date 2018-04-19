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
#include "DATBlock.h"

DATBlock::DATBlock() : mByteCount(0)
{
}

DATBlock::DATBlock(const DATBlock& copy)
  : mByteCount(copy.mByteCount)
{
  memcpy(mLineWords, copy.mLineWords, sizeof(mLineWords[0]) * mByteCount);
  memcpy(mBytes, copy.mBytes, sizeof(mBytes[0]) * mByteCount);
}

DATBlock::~DATBlock()
{
}

//
// Reset, forgetting any current contents.
//
void
DATBlock::Reset()
{
  mByteCount = 0;
}

//
// Attempt to append a received byte to this block.
//
// Returns True if, after receiving this byte, the block
// is now complete.
//
bool
DATBlock::AddByte(uint16_t line_word, uint16_t flagged_byte)
{
  if (mByteCount == 36)
    return true;

  mLineWords[mByteCount] = line_word;
  mBytes[mByteCount] = flagged_byte;

  mByteCount++;

  return (mByteCount == 36);
}

//
// The number of bytes in the block.
//
size_t
DATBlock::Size() const
{
  return mByteCount;
}

//
// Get access to the underlying 10-bit words and 8-bit flagged
// bytes, respectively.
//
const uint16_t *
DATBlock::LineWords() const
{
  return &mLineWords[0];
}

const uint16_t *
DATBlock::FlaggedBytes() const
{
  return &mBytes[0];
}
