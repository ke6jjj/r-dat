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

#ifndef RDAT_DAT_TRACK_FRAMER_H
#define RDAT_DAT_TRACK_FRAMER_H

//
// A DAT track framer receives DAT blocks and carrier information.
// It tries to organize the blocks into tracks and then tries to
// pair the tracks into frames with the help of an underlying frame
// receiver to direct the pairing.
//
// Once track pairs are detected, they are handed to the frame receiver
// for further handling. The frame handler is in charge of interpreting
// the data as DAT audio or DDS.
//

#include "DATBlockReceiver.h"
#include "DATFrameReceiver.h"
#include "Track.h"

class DATTrackFramer : public DATBlockReceiver {
public:
  DATTrackFramer(DATFrameReceiver& receiver);
  ~DATTrackFramer();

  //
  // A track is starting (when going up) or stopping
  // (when going down).
  //
  void TrackDetected(bool up);

  //
  // Receive DAT 35-byte block.
  //
  void ReceiveBlock(const DATBlock& block);

  //
  // An automatic track finding (ATF) tone has been identified within a
  // track.  
  //
  void ReceiveATFTone(int toneNumber);
  
  //
  // All input is done, nothing else will be coming.
  //
  void Stop();

protected:
  //
  // The current tracking state.
  //
  bool mTracking;
  
  //
  // The number of ATF2 and ATF3 tones received in this track. These
  // tones help distinguish negative azimuth (A) tracks from positive azimuth
  // (B) tracks.
  //
  const int mATF3Threshold;
  int mATF2Count;
  int mATF3Count;
  
  //
  // Track object for collecting the blocks we receive.
  //
  Track *mCurrentTrack;
  Track *mLastTrack;
  
  //
  // Full frame receiver. This is where DAT and DDS begin
  // to diverge.
  //
  DATFrameReceiver& mReceiver;
};

#endif
