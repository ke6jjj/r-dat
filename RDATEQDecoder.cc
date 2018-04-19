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

#include <math.h>
#include <stdio.h>
#include "RDATEQDecoder.h"

static const float kSymbolRate = 9408000;

RDATEQDecoder::RDATEQDecoder(float sampleRate)
: mSyncWindowCurPos(0), mSyncWindowSyncPos(0),
  mClockDetected(false), mClockRatioThreshold(0.97),
  mClockAlpha(1.0 / 30.0)
{
  int i;
  
  //
  // Create a filter bank/window for the clock detector.
  //
  mSyncWindowSize = sampleRate / kSymbolRate;
  mSyncWindow = new float[mSyncWindowSize];
  mSyncWindowEvalPos = mSyncWindowSize / 2;
  mSyncWindowNextEvalPos = mSyncWindowEvalPos;
  
  for (i = 0; i < mSyncWindowSize; i++)
    mSyncWindow[i] = 0.0;
}

RDATEQDecoder::~RDATEQDecoder()
{
  delete mSyncWindow;
}

void
RDATEQDecoder::SetSymbolDecoder(EQSymbolDecoder *d)
{
  mDecoder = d;
}

void
RDATEQDecoder::Process(float *samples, size_t count)
{
  size_t i;
  float signal;
  bool sign, zeroCross;
  
  for (i = 0; i < count; i++) {
    //
    // Look at the sample.
    //
    signal = samples[i];

    //
    // Send the baseband signal to the clock detector.
    // If it returns true, then the clock detector has
    // achieved a good lock and suggests sampling the
    // signal now.
    //
    if (ClockDetect(signal)) {
      //
      // Signal should be sampled now.
      //
      mDecoder->ReceiveSample(signal);
    }
  }
}

bool
RDATEQDecoder::ClockDetect(float sample)
{
  bool syncNow = false;
  
  mSyncWindow[mSyncWindowCurPos] *= (1.0 - mClockAlpha);
  mSyncWindow[mSyncWindowCurPos] += fabs(sample) * mClockAlpha;

  //
  // If we're at the currently suggested sync window position,
  // remember so.
  //
  if (mSyncWindowCurPos == mSyncWindowSyncPos) {
    syncNow = true;
    
    //
    // Do a staggered update of the sync window evaluation
    // position based on the last position calculation.
    //
    mSyncWindowEvalPos = mSyncWindowNextEvalPos;
  }

  if (mSyncWindowCurPos == mSyncWindowEvalPos) {
      //
      // Take this opportunity to rescan the window,
      // looking for the highest energy peak and thus, the
      // best synchronization point.
      //
      EvaluateClock();
  }
  
  //
  // Advance the window, wrapping if necessary.
  //
  if (++mSyncWindowCurPos == mSyncWindowSize)
    //
    // We've reached the end of the window.
    // Wrap around.
    //
    mSyncWindowCurPos = 0;  
  
  return mClockDetected && syncNow;
}

void
RDATEQDecoder::EvaluateClock()
{
  int i, maxI, minI;
  float max = 0.0, min = 100.0, ratio;

  for (i = maxI = 0; i < mSyncWindowSize; i++) {
    //
    // Find the highest peak.
    //
    if (mSyncWindow[i] > max) {
      max = mSyncWindow[i];
      maxI = i;
    }
    //
    // And the lowest valley.
    //
    if (mSyncWindow[i] < min)
      min = mSyncWindow[i];
  }
  
  if (maxI != mSyncWindowSyncPos) {
    //
    // We've found a new sync position.
    // In the future we might calculate clock error here.
    //
    mSyncWindowSyncPos = maxI;
    mSyncWindowNextEvalPos = (maxI + mSyncWindowSize / 2) %
                              mSyncWindowSize;
  }
  
  //
  // If there is a significant difference between the highest
  // peak and the lowest valley then we have a good clock signal.
  //
  if (max > 0.0)
    ratio = min / max;
  else
    ratio = 0.0;

  if (ratio < mClockRatioThreshold)
    mClockDetected = true;
  else
    mClockDetected = false;
}

void
RDATEQDecoder::SetClockRatioThreshold(float threshhold)
{
  mClockRatioThreshold = threshhold;
}

void
RDATEQDecoder::SetClockAlpha(float alpha)
{
  mClockAlpha = alpha;
}
