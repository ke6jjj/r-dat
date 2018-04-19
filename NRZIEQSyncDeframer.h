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

#ifndef RDAT_NRZI_EQ_SYNC_DEFRAMER_H
#define RDAT_NRZI_EQ_SYNC_DEFRAMER_H

#include <stdint.h>
#include <stddef.h>
#include "EQSymbolDecoder.h"
#include "DATWordReceiver.h"

//
// NRZI deframer which examines the last three samples to
// determine the middle sample's state, synchronizes on
// the R-DAT 0100010001 synchronization pattern and outputs
// ten-bit frames.
//
class NRZIEQSyncDeframer : public EQSymbolDecoder
{
public:
  NRZIEQSyncDeframer(DATWordReceiver *receiver);
  virtual ~NRZIEQSyncDeframer();
  
  //
  // Reset the deframer. Dumps any accumulated bits and reverts to sync search
  // state.
  //
  void Reset();
  
  //
  // Called by lower-level bit slicer. Notifies decoder that the carrier/clock
  // PLL state has changed.  When carrier is dropped, the deframer resets
  // itself.
  //
  void CarrierDetected(bool detected);
  
  //
  // Called by lower-level bit slicer when it determines that the time
  // is right to make a sample.
  //
  void ReceiveSample(float sample);

protected:
  //
  // 10-bit accumulating shift register.
  //
  int mFrame;

  //
  // Last two samples received.
  //
  float mLast1;
  float mLast2;
  
  //
  // Number of bits received since last sync position.
  //
  size_t mSyncBitCount;
  
  //
  // Current state.
  //
  enum { STATE_SYNC_SEARCH, STATE_SYNCED } mState;
  
  //
  // Recipient of our received frames.
  //
  DATWordReceiver *mReceiver;
};

#endif
