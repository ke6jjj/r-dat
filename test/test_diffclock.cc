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

#include "tests.h"
#include "DifferentialClockDetector.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

void
test_diffclock(TestSession& ts)
{
  const size_t kSamplesPerSymbol = 8;
  const float kDetectRatio = 0.97;
  const float kAlpha = 1.0 / 30.0;

  DifferentialClockDetector det(kSamplesPerSymbol, kDetectRatio, kAlpha);
  
  ts.BeginTest("DiffClockDet feed 10 sync pulses");
  for (size_t i = 0; i < 10 * kSamplesPerSymbol; i++) {
    float submitValue;

    switch (i % kSamplesPerSymbol) {
    case 3:
      submitValue = -1.0;
      break;
    case 4:
      submitValue = 1.0;
      break;
    default:
      submitValue = 0.0;
    } 

    det.AddAndDetect(submitValue);
  }
  ts.EndTest(true);

  size_t detectCount = 0;
  size_t detectPosition;

  for (size_t i = 0; i < kSamplesPerSymbol; i++) {
    bool sampleNow = det.AddAndDetect(0.0);
    if (sampleNow) {
      detectCount++;
      detectPosition = i;
    }
  }

  ts.BeginTest("DiffClockDet fired once");
  ts.EndTest(detectCount == 1);

  ts.BeginTest("DiffClockDet fired at the right time");
  ts.EndTest(detectCount > 0 && detectPosition == 7);
}
