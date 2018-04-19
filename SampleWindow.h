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

#ifndef RDAT_SAMPLE_WINDOW_H
#define RDAT_SAMPLE_WINDOW_H

#include <stddef.h>

//
// Stores a history of samples and calculates statistics over the window.
//
class SampleWindow
{
public:
  SampleWindow(size_t size)
    : mSize(size), mWindow(new float[size]), mNextPosition(0)
  {
    Reset();
  };

  ~SampleWindow()
  {
    delete[] mWindow;
  };

  void Reset()
  {
    for (size_t i = 0; i < mSize; i++)
      mWindow[i] = 0.0;
  };

  void Add(float sample)
  {
    mWindow[mNextPosition] = sample;
    mNextPosition = (mNextPosition + 1) % mSize;
  };

  float Slope() const
  {
    size_t thisPos = (mNextPosition - 1) % mSize;
    size_t lastPos = mNextPosition;

    return (mWindow[thisPos] - mWindow[lastPos]) / mSize;
  };

  bool CrossesZero() const
  {
    size_t thisPos = (mNextPosition - 1) % mSize;
    size_t lastPos = mNextPosition;

    return (mWindow[thisPos] > 0) != (mWindow[lastPos] > 0);
  }

protected:
  const size_t mSize;
  float *mWindow;
  size_t mNextPosition;
};

#endif
