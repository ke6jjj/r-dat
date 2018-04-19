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
#include "RDATDecoder.h"

static const float kSymbolRate = 9408000;

RDATDecoder::RDATDecoder(float sampleRate)
: mSyncWindowCurPos(0), mSyncWindowSyncPos(0),
  mClockDetected(false), mClockRatioThreshold(0.97),
  mClockAlpha(1.0 / 30.0),
  //
  // A track is 196 blocks in duration. Convert that to samples and
  // add padding.
  //
  mTrackDuration((sampleRate / kSymbolRate) * 10 * 36 * 196 * 1.05),
  mTrackInProgress(false)
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

  //
  // Keep a history of the sign of the last sample.
  //
  mLastSign = 0;
}

RDATDecoder::~RDATDecoder()
{
  delete mSyncWindow;
}

void
RDATDecoder::SetSymbolDecoder(SymbolDecoder *d)
{
  mDecoder = d;
}

void
RDATDecoder::Process(float *samples, size_t count)
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
    // Check if this sample has made a zero-crossing.
    //
    sign = signal > 0.0;
    zeroCross = sign != mLastSign;

    //
    // Send the baseband signal to the clock detector.
    // If it returns true, then the clock detector has
    // achieved a good lock and suggests sampling the
    // signal now.
    //
    if (ClockDetect(zeroCross)) {
      //
      // Signal should be sampled now. Dump our integrator.
      //
      mDecoder->ReceiveBit(mIntegrator > 0.0);
      mIntegrator = 0.0;
    }

    //
    // Update the integrator.
    //
    mIntegrator += signal;
    mLastSign = sign;

    //
    // If there's no track in progress, see if a new one has perhaps
    // started.
    //
    if (!mTrackInProgress) {
      //
      // We're in the idle time between tracks. Check if a track has started.
      //
      if (mDecoder->PreambleDetected()) {
        //
        // A track appears to have started. Start the track progress timer.
        //
        mTrackInProgress = true;
        mTrackSampleCount = mTrackDuration;

        //
        // Notify the downstream decoder.
        //
        mDecoder->TrackDetected(true);
      }
    } else {
      //
      // There's a track in progress. See if the track timer has expired.
      //
      mTrackSampleCount--;
      if (mTrackSampleCount == 0) {
        //
        // The track should have ended by now. Declare it over.
        //
        mTrackInProgress = false;
        mDecoder->TrackDetected(false);
      }
    }
  }
}

void
RDATDecoder::Stop()
{
  //
  // No further input is coming. Notify upstream symbol receiver.
  //
  mDecoder->Stop();
}

bool
RDATDecoder::ClockDetect(float sample)
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
RDATDecoder::EvaluateClock()
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

  if (ratio < mClockRatioThreshold) {
    if (!mClockDetected && mDecoder != NULL)
      mDecoder->ClockDetected(true);
    mClockDetected = true;
  } else {
    if (mClockDetected && mDecoder != NULL)
      mDecoder->ClockDetected(false);
    mClockDetected = false;
  }
}

void
RDATDecoder::SetClockRatioThreshold(float threshhold)
{
  mClockRatioThreshold = threshhold;
}

void
RDATDecoder::SetClockAlpha(float alpha)
{
  mClockAlpha = alpha;
}
