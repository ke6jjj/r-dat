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

#ifndef RDAT_AUDIO_FRAME_RECEIVER_H
#define RDAT_AUDIO_FRAME_RECEIVER_H

#include <stdio.h>
#include <stdint.h>
#include "DATFrameReceiver.h"
#include "DATFrame.h"

class AudioFrameReceiver : public DATFrameReceiver {
public:
  AudioFrameReceiver();
  ~AudioFrameReceiver();

  //
  // Check if two tracks pair into a valid frame.
  //
  bool IsFrame(const Track& a, const Track& b);

  //
  // Receive a full DAT frame (two tracks, A & B)
  //
  void ReceiveFrame(const Track& a, const Track& b);

  //
  // Dump received audio to the specified file.
  //
  bool SetDumpFile(const char *path);

  //
  // Mark that all processing is stopping.
  //
  void Stop();

protected:
  //
  // Handle any Date/Time subcodes present in a frame
  // and try to synchronize with the real-time clock that made
  // them.
  //
  void HandleDateTime(const uint8_t *item, uint32_t absoluteFrame);

  //
  // File handle to dump the sound to, if asked.
  //
  FILE *mFile;

  //
  // Number of sample frames written to dump file, if asked to dump.
  //
  size_t mFramesWritten;
  
  //
  // The data.
  //
  DATFrame mFrame;
  bool     mHaveLastAbsoluteFrameNumber;
  uint32_t mLastAbsoluteFrameNumber;
  uint32_t mNextSessionFrameNumber;
  
  //
  // Date and time sub-frame computation.
  //
  bool     mHaveLastDateTime;
  bool     mHaveLastChangeFrame;
  uint32_t mLastChangeFrame;
  uint64_t mLastDateTimeSeconds;

  bool     mHaveDateTimeSync;
  int      mCurrentDateTimeMilliseconds;
  uint64_t mCurrentDateTimeSeconds;
};

#endif
