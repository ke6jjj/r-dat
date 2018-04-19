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

#ifndef RDAT_DIFFERENTIAL_CLOCK_DETECTOR_H
#define RDAT_DIFFERENTIAL_CLOCK_DETECTOR_H

#include <stddef.h>

#include "ClockListener.h"

class DifferentialClockDetector {
public:
  DifferentialClockDetector(
    size_t samplesPerSymbol,
    float detectThresholdRatio,
    float clockAlpha
  );
  ~DifferentialClockDetector();

  bool AddAndDetect(float sample);
  void Reset();

  void SetClockListener(ClockListener *l);

protected:
  void EvaluateClock();

  //
  // The number of samples in a symbol, and hence, the number
  // of differential samples to keep track of.
  //
  const size_t mWindowSize;

  //
  // The update alpha value (stiffness) of the loop.
  //
  const float mAlpha;

  //
  // The ratio threshold for considering a clock to be detected.
  //
  const float mDetectionThresholdRatio;

  //
  // A history of the last differential samples received.
  //
  float *mWindow;
  float mLastSample;

  //
  // Whether or not a clock is currently detected.
  //
  bool mDetected;

  //
  // The window position where the signal currently seems to have completed
  // the most movement. This is also the position at which the next firing
  // position will be updated.
  //
  size_t mMaximumDiffPos;

  //
  // The window position that is farthest away from the maximum
  // movement and which marks the time at which a symbol should be
  // evaluated.
  //
  size_t mSyncPos;
  size_t mNextSyncPos;

  //
  // The current position within the window.
  //
  size_t mCurrentPos;

  ClockListener *mListener;
};

#endif
