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

#ifndef RDAT_DAT_WORD_RECEIVER_H
#define RDAT_DAT_WORD_RECEIVER_H

#include <stdint.h>
#include "DATBlockReceiver.h"

class DATWordReceiver
{
public:
  DATWordReceiver(DATBlockReceiver *r, bool dump = true);
  ~DATWordReceiver();
  
  //
  // Receive a word.
  //
  void ReceiveWord(int word);

  //
  // Note that track is starting or ending.
  //
  void TrackDetected(bool up);

  //
  // Note that all input is finished.
  //
  void Stop();

protected:
  void DumpWord(uint16_t raw, uint16_t decode);
  void ReceiveWord(uint16_t raw, uint16_t decode);
  void DumpCurrentBlock();

  //
  // Do raw dumps or send further downstream.
  //
  bool mDump;

  //
  // The current 35-character block being received.
  //
  DATBlock mBlock;

  //
  // The downstream block receiver.
  //
  DATBlockReceiver *mBlockReceiver;
};

#endif
