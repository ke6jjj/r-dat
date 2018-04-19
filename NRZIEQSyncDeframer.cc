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

#include "NRZIEQSyncDeframer.h"

NRZIEQSyncDeframer::NRZIEQSyncDeframer(DATWordReceiver *receiver)
  : mReceiver(receiver)
{
  Reset();
}

NRZIEQSyncDeframer::~NRZIEQSyncDeframer()
{
}

//
// Reset the deframer. Dumps any accumulated bits and reverts to sync search
// state.
//
void
NRZIEQSyncDeframer::Reset()
{
  mState = STATE_SYNC_SEARCH;
  mFrame = 0;
  mLast1 = 0.0;
  mLast2 = 0.0;
}

// Called by lower-level bit slicer. Notifies decoder that the carrier/clock
// PLL state has changed.  When carrier is dropped, the deframer resets
// itself.
//
void
NRZIEQSyncDeframer::CarrierDetected(bool detected)
{
  if (!detected)
    Reset();
}

//
// Called by lower-level bit slicer when it decodes a bit. This in turn may
// cause a frame to be delivered to the downstream receiver.
//
void
NRZIEQSyncDeframer::ReceiveSample(float sample)
{
  bool bit;
  size_t bit_count;
  
  //
  // Find the slopes of the sides of the triangle formed by
  // the previous two samples and this bit.
  //
  float slope_a = mLast2 - mLast1;
  float slope_b = sample - mLast2;

  bool sign_a = slope_a > 0.0;
  bool sign_b = slope_b > 0.0;

  //
  // NRZI decode the bit.
  //
  bit = sign_a != sign_b;

  //
  // Accumulate a new bit into the large shift register.
  //
  mFrame  &= 0x1ff;
  mFrame <<= 1;
  mFrame  |= bit;
  
  //
  // Are we currently searching for frame sync and have we received
  // a sync word?
  //
  if ((mFrame & 0x1ff) == 0x111) {
    //
    // We've found a sync pattern. Enter sync state.
    //
    mSyncBitCount = 0;
    mState = STATE_SYNCED;

    //
    // Notify the upstream frame receiver of the sync word.
    //
    mReceiver->ReceiveWord(mFrame);
  } else if (mState == STATE_SYNCED) {
    //
    // We're in a synchronized state. We should keep accepting bits into
    // the accumulator until a full frame is built up. Has a frame been
    // built up?
    //
    mSyncBitCount += 1;
    if (mSyncBitCount == 10) {
      //
      // There's a full frame built up.
      //
      mSyncBitCount = 0;
      
      //
      // Deliver it upstream.
      //
      mReceiver->ReceiveWord(mFrame);
    }
  }

  //
  // Update the sample history.
  //
  mLast1 = mLast2;
  mLast2 = sample;
}
