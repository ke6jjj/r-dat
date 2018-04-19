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

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "File.h"

//
// A simple AT&T streams-like file interface that is reliably cancelable.
//

File::File() : mOpen(false), mResidual(NULL), mQuanta(0)
{
}

File::~File()
{
  if (mOpen)
    Close();
  delete mResidual;
}

bool
File::Open(const char *path, size_t quanta)
{
  mFd = ::open(path, O_RDONLY);
  if (mFd == -1)
    return false;

  mOpen = true;
  Reset(quanta);

  return true;
}

//
// Open from an existing file descriptor.
//
bool
File::Open(int fd, size_t quanta)
{
  mFd = fd;
  if (mFd == -1)
    return false;

  mOpen = true;
  Reset(quanta);

  return true;
}

void
File::Close()
{
  if (!mOpen)
    return;

  ::close(mFd);
  
  mOpen = false;
}

void
File::Reset(size_t quanta)
{
  delete mResidual;
  mQuanta = quanta;
  mResidual = new char[mQuanta];
  mResidualCount = 0;
}

size_t
File::Read(void *buf, size_t count)
{
  char *cbuf = (char *) buf;

  if (!mOpen)
    return 0;

  if (count == 0)
    return 0;
  
  size_t total = count * mQuanta;

  //
  // Copy in any residual data first.
  //
  if (mResidualCount > 0) {
    memcpy(cbuf, mResidual, mResidualCount);
    total -= mResidualCount;
  }

  size_t pos = mResidualCount;

  while (total > 0) {
    int nread = ::read(mFd, &cbuf[pos], total);
    if (nread == -1 || nread == 0) {
      //
      // Some sort of error occurred. Probably a signal.
      //
      break;
    }
    total -= nread;
    pos += nread;
  }

  //
  // Is there any residual data?
  //
  size_t residue = pos % mQuanta;
  if (residue != 0) {
    memcpy(mResidual, &cbuf[pos - residue], residue);
    pos -= residue;
  }

  mResidualCount = residue;

  return pos / mQuanta;
}
