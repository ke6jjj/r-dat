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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "DATTrackFramer.h"

DATTrackFramer::DATTrackFramer(DATFrameReceiver& receiver)
  : mReceiver(receiver), mLastTrack(NULL), mTracking(false),
    mCurrentTrack(new Track(Track::HEAD_UNKNOWN)),
    mATF2Count(0), mATF3Count(0), mATF3Threshold(10)
{
}

DATTrackFramer::~DATTrackFramer()
{
  delete mCurrentTrack;
  delete mLastTrack;
}

//
// Receive DAT 35-byte block.
//
void
DATTrackFramer::ReceiveBlock(const DATBlock& block)
{
  if (!mTracking)
    //
    // This shouldn't happen.
    //
    return;
  
  //
  // Incorporate the block into the current track.
  //
  mCurrentTrack->AddBlock(block);
}

//
// Handle a track start/stop indication.
//
void
DATTrackFramer::TrackDetected(bool up)
{
  mTracking = up;
  
  //
  // Wait until tracking is complete (goes down).
  //
  if (up)
    return;

  //
  // Our current track is complete. Give it a chance to perform
  // all error correction.
  //
  mCurrentTrack->Complete();
  
  //
  // If there were any ATF tones detected, use the majority count to
  // determine whether this was a negative azimuth track or a positive
  // azimuth track.
  //
  // Currently only ATF3 (negative azimuth signal) is likely to be detected,
  // so if its count is high enough, assume that the track is an A track.
  //
  printf("Track ATF3 Count: %d\n", mATF3Count);
  if (mATF3Count > mATF3Threshold) {
    mCurrentTrack->SetHead(Track::HEAD_A);
  }

  //
  // See if this track completes a frame (a pair of tracks).
  //
  if (mLastTrack == NULL) {
    //
    // There is no previous track. Just stash this one away.
    //
    mLastTrack = mCurrentTrack;
  } else {
    //
    // We have a previous track. Are these two a pair or are they
    // separate?
    //
    if (mReceiver.IsFrame(*mLastTrack, *mCurrentTrack)) {
      //
      // According to the downstream frame receiver, these two
      // tracks pair into a full frame. Give them to the
      // receiver as such.
      //
      mReceiver.ReceiveFrame(*mLastTrack, *mCurrentTrack);
      
      //
      // The pair have been shipped off. Prepare for a new set.
      //
      delete mLastTrack;
      delete mCurrentTrack;
      mLastTrack = NULL;
    } else {
      //
      // These two don't pair. Dump the last track and continue
      // searching.
      //
      delete mLastTrack;
      mLastTrack = mCurrentTrack;
    }
  }

  //
  // Reset the automatic track finding tone counts.
  //
  mATF2Count = 0;
  mATF3Count = 0;

  //
  // Prepare a new track.
  //
  mCurrentTrack = new Track(Track::HEAD_UNKNOWN);
}

//
// Handle detection of a specific automatic track finding tone.
//
void
DATTrackFramer::ReceiveATFTone(int toneNumber)
{
  if (toneNumber == 2)
    mATF2Count++;
  else if (toneNumber == 3)
    mATF3Count++;
}

void
DATTrackFramer::Stop()
{
  if (mTracking)
    TrackDetected(false);

  mReceiver.Stop();
}
