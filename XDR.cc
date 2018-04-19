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

#include "XDR.h"
#include <string.h>

//
// Class for encapsulating writing little-endian data to a buffer of
// bytes.
//
XDR::XDR(size_t sz) : mSize(sz)
{
  mData = new uint8_t[mSize];
  Reset();
}

XDR::~XDR()
{
  delete[] mData;
}

void
XDR::Reset()
{
  mPosition = 0;
}

bool
XDR::AddString(const char *str, size_t len)
{
  if (!SizeCheck(len))
    return false;
  
  memcpy(&mData[mPosition], str, len);
  mPosition += len;
  
  return true;
}
  
bool
XDR::AddI16(int16_t i)
{
  if (!SizeCheck(2))
    return false;
  
  mData[mPosition] = (i & 0xff);
  mData[mPosition+1] = (i >> 8) & 0xff;
  mPosition += 2;
  
  return true;
}

bool
XDR::AddU16(uint16_t i)
{
  if (!SizeCheck(2))
    return false;
  
  mData[mPosition] = (i & 0xff);
  mData[mPosition+1] = (i >> 8) & 0xff;
  mPosition += 2;
  
  return true;
}

bool
XDR::AddI32(int32_t i)
{
  if (!SizeCheck(4))
    return false;
  
  mData[mPosition] = (i & 0xff);
  mData[mPosition+1] = (i >> 8) & 0xff;
  mData[mPosition+2] = (i >> 16) & 0xff;
  mData[mPosition+3] = (i >> 24) & 0xff;
  mPosition += 4;
  
  return true;
}


bool
XDR::AddU32(uint32_t i)
{
  if (!SizeCheck(4))
    return false;
  
  mData[mPosition] = (i & 0xff);
  mData[mPosition+1] = (i >> 8) & 0xff;
  mData[mPosition+2] = (i >> 16) & 0xff;
  mData[mPosition+3] = (i >> 24) & 0xff;
  mPosition += 4;
  
  return true;
}

const uint8_t *
XDR::Data() const
{
  return mData;
}

size_t
XDR::Size() const
{
  return mPosition;
}

bool
XDR::Complete() const
{
  return mPosition == mSize;
}

bool
XDR::SizeCheck(size_t add) const
{
  return (mPosition + add) <= mSize;
}
