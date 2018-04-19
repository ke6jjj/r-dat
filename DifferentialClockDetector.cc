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

#include "DifferentialClockDetector.h"

#include <math.h>

//
// Given a window size, in samples, and the current position giving the
// maximum differential, return the position within the window where the
// samples, as a whole, should be evaluated for their symbol content.
//
static inline size_t FirePosition(size_t pos, size_t numSamples)
{
  //
  // In N samples there are N-1 differentials, and thus,
  // the best sample point is halfway from there.
  //
  return (pos + ((numSamples - 1) / 2)) % numSamples;
}

DifferentialClockDetector::DifferentialClockDetector(
  size_t samplesPerSymbol,
  float detectThresholdRatio,
  float clockAlpha
) :
  mWindowSize(samplesPerSymbol),
  mDetectionThresholdRatio(detectThresholdRatio),
  mWindow(new float[mWindowSize]),
  mAlpha(clockAlpha),
  mListener(NULL)
{
  Reset();
}

DifferentialClockDetector::~DifferentialClockDetector()
{
}

void
DifferentialClockDetector::Reset()
{
  mLastSample = 0.0;
  mMaximumDiffPos = 0;
  mNextSyncPos = FirePosition(mMaximumDiffPos, mWindowSize);
  mDetected = false;
  mCurrentPos = 0;
  for (size_t i = 0; i < mWindowSize; i++)
    mWindow[i] = 0.0;
}

bool
DifferentialClockDetector::AddAndDetect(float sample)
{
  bool syncNow = false;

  //
  // Calculate the difference from the last sample.
  //
  float difference = sample - mLastSample;
  
  //
  // Update the current position within the sample window with the
  // new differential value.
  //
  mWindow[mCurrentPos] *= (1.0 - mAlpha);
  mWindow[mCurrentPos] += fabs(difference) * mAlpha;

  if (mCurrentPos == mMaximumDiffPos) {
    //
    // Update the state machine to trigger on the next queued
    // trigger position.
    //
    mSyncPos = mNextSyncPos;
  } else if (mCurrentPos == mSyncPos) {
    //
    // Remember to return "evaluate symbol now" to the caller.
    //
    syncNow = true;
    
    //
    // Scan through the window to determine where the maximum
    // historical differential appears to be.
    //
    EvaluateClock();
  }
  
  //
  // Update positions and samples.
  //
  mCurrentPos += 1;
  mLastSample = sample;

  if (mCurrentPos == mWindowSize) {
    //
    // We've reached the end of the window.
    // Wrap around.
    //
    mCurrentPos = 0;  
  }
  
  return mDetected && syncNow;
}

void
DifferentialClockDetector::EvaluateClock()
{
  int i, maxI, minI;
  float max, min, ratio;

  for (max = min = mWindow[0], i = 1, maxI = 0; i < mWindowSize; i++)
  {
    //
    // Find the highest peak.
    //
    if (mWindow[i] > max) {
      max = mWindow[i];
      maxI = i;
    }
    //
    // And the lowest valley.
    //
    if (mWindow[i] < min)
      min = mWindow[i];
  }
  
  if (maxI != mMaximumDiffPos) {
    //
    // We've found a new maximum differential position.
    // In the future we might calculate clock error here.
    //
    mMaximumDiffPos = maxI;

    //
    // Queue up the next firing position.
    //
    mNextSyncPos = FirePosition(mMaximumDiffPos, mWindowSize);
  }
  
  //
  // If there is a significant difference between the highest
  // peak and the lowest valley then we have a good clock signal.
  //
  if (max > 0.0)
    ratio = min / max;
  else
    ratio = 0.0;

  bool newDetectionState = ratio <= mDetectionThresholdRatio;

  if (newDetectionState != mDetected && mListener != NULL) {
    mListener->ClockDetected(newDetectionState);
  }

  mDetected = newDetectionState;
}
