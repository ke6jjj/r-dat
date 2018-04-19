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

#include "RDATSlopeDecoder.h"
#include "SampleWindow.h"
#include "DifferentialClockDetector.h"

#include <math.h>
#include <stdio.h>

static const float kSymbolRate = 9408000;

RDATSlopeDecoder::RDATSlopeDecoder(float sampleRate)
: mkSamplesPerSymbol(sampleRate / kSymbolRate),
  //
  // Prime the differential clock detector.
  //
  mClockDetector(
    mkSamplesPerSymbol,
    0.97 /* Ratio threshold */,
    1.0 / 30.0 /* clock alpha */
  ),
  //
  // Make a window for one complete symbol pulse.
  //
  mSymbolWindow(mkSamplesPerSymbol),
  //
  // A track is 196 blocks in duration. Convert that to samples and
  // add padding.
  //
  mTrackDuration(mkSamplesPerSymbol * 10 * 36 * 196 * 1.05),
  mTrackInProgress(false),
  //
  // Reach a 99% agreement on the overall signal level by 2000 symbols.
  //
  mAlpha(pow(0.01, 1.0 / (2000.0 * mkSamplesPerSymbol))),
  mDecoder(NULL)
{
  Reset();
}

RDATSlopeDecoder::~RDATSlopeDecoder()
{
}

void
RDATSlopeDecoder::Reset()
{
  mClockDetector.Reset();
  mSymbolWindow.Reset();
  mOverallSignalLevel = 0.0;
  mTrackInProgress = false;
  mTrackSampleCount = 0;
#if 1
  mSampleNumber = 0;
#endif
}

void
RDATSlopeDecoder::SetSymbolDecoder(SymbolDecoder *d)
{
  mDecoder = d;
}

void
RDATSlopeDecoder::Process(float *samples, size_t count)
{
  size_t i;
  float signal;
  
  for (i = 0; i < count; i++) {
    //
    // Look at the sample.
    //
    signal = samples[i];
    mSampleNumber++;

    //
    // Update the overall signal level.
    //
    mOverallSignalLevel *= mAlpha;
    mOverallSignalLevel += (1.0 - mAlpha) * fabs(signal);

    //
    // Store the new sample in the symbol window
    //
    mSymbolWindow.Add(signal);

    //
    // Ask the clock detector if now is a good time to sample the signal.
    //
    // This clock detection algorithm needs to be 180 degrees out of phase
    // with the symbol period so that any magnetic pulse picked up by the
    // read head will be centered within the sample period.
    //
    if (mClockDetector.AddAndDetect(signal)) {
      //
      // Evaluate all the samples in the sample window to determine the
      // average slope amonst them.
      //
      float slope = mSymbolWindow.Slope();

      bool zeroCrossed = mSymbolWindow.CrossesZero();

      //
      // If the slope exceeds the detection threshold, mark that we've found
      // a pulse.
      //
      float threshold;
      if (zeroCrossed) {
        threshold = mOverallSignalLevel / mkSamplesPerSymbol * 0.72;
      } else {
        threshold = mOverallSignalLevel / mkSamplesPerSymbol * 3;
      }

      bool pulse = fabs(slope) > threshold;

#if 1
      if (mSampleNumber > 87387) {
#if 0
        printf("%zd overall %g, slope %g, target %g\n",
          mSampleNumber,
          mOverallSignalLevel,
          fabs(slope),
          threshold
        );
#endif
#if 1
        printf(" %zd pulse %d", mSampleNumber, pulse ? 1 : 0);
#endif
      }
#endif
     
      //
      // Signal the decision.
      //
      if (mDecoder != NULL)
        mDecoder->ReceiveBit(pulse);
    }

    //
    // If there's no track in progress, see if a new one has perhaps
    // started.
    //
    if (!mTrackInProgress) {
      //
      // We're in the idle time between tracks. Check if a track has started.
      //
      if (mDecoder != NULL && mDecoder->PreambleDetected()) {
        //
        // A track appears to have started. Start the track progress timer.
        //
        mTrackInProgress = true;
        mTrackSampleCount = mTrackDuration;

        //
        // Notify the downstream decoder.
        //
        mDecoder->TrackDetected(true);
#if 1
        printf("%zd Track started\n", mSampleNumber);
#endif
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
        if (mDecoder != NULL)
          mDecoder->TrackDetected(false);
#if 1
        printf("%zd Track stopped\n", mSampleNumber);
#endif
      }
    }
  }
}

void
RDATSlopeDecoder::Stop()
{
  //
  // No further input is coming. Notify upstream symbol receiver.
  //
  if (mDecoder != NULL)
    mDecoder->Stop();
}
