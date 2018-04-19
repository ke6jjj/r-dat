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

#include "SyncDeframer.h"

SyncDeframer::SyncDeframer(DATWordReceiver *receiver)
  : mReceiver(receiver),
    mTrackDetected(false)
{
  Reset();
}

SyncDeframer::~SyncDeframer()
{
}

//
// Reset the deframer. Dumps any accumulated bits and reverts to sync search
// state.
//
void
SyncDeframer::Reset()
{
  mState = STATE_SYNC_SEARCH;
  mFrame = 0;
  mPreambleCheck = 0;
  mPreambleSymbolCount = 0;
  mTrackDetected = false;
}

//
// Called by lower-level bit slicer. Notifies decoder that the carrier/clock
// PLL state has changed.  When carrier is dropped, the deframer resets
// itself.
//
void
SyncDeframer::ClockDetected(bool detected)
{
  if (!detected)
    Reset();
}

//
// Called by lower-level bit slicer when it decodes a bit. This in turn may
// cause a frame to be delivered to the downstream receiver.
//
void
SyncDeframer::ReceiveBit(bool bit)
{
  //
  // Accumulate a new bit into the large shift register.
  //
  mFrame  &= 0x1ff;
  mFrame <<= 1;
  mFrame  |= bit;

  //
  // If the caller is trying to detect a track start, do pre-amble
  // detection.
  //  
  if (!mTrackDetected) {
    //
    // Check for pre-amble words every ten symbols.
    //
    if (++mPreambleCheck == 10) {
      //
      // If this is a pre-amble sequence (all ones) then increment the preamble
      // count. Otherwise, reset it. (This is used in track start/stop
      // detection)
      //
      mPreambleCheck = 0;
      if (mFrame == 0x3ff)
        mPreambleSymbolCount++;
      else
        mPreambleSymbolCount = 0;
    }
  }
  
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
}

void
SyncDeframer::TrackDetected(bool detected)
{
  //
  // A track has begun or ended. Cache the state.
  //
  mTrackDetected = detected;
  
  //
  // If a track has stopped, reset the preamble detection logic
  // so that it is ready to detect a new track.
  //
  if (!detected) {
    mPreambleSymbolCount = 0;
    mPreambleCheck = 0;
  }
  
  //
  // Let the downstream word collector know that the track detection
  // has changed.
  //
  mReceiver->TrackDetected(detected);
}

bool
SyncDeframer::PreambleDetected(void) const
{
  return mPreambleSymbolCount > 10;
}

void
SyncDeframer::Stop(void)
{
  mReceiver->Stop();
}
