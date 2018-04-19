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

#ifndef RDAT_DDS_FRAME_RECEIVER_H
#define RDAT_DDS_FRAME_RECEIVER_H

#include "DATFrameReceiver.h"
#include "DDSGroup3.h"
#include "BasicGroup.h"

class DDSFrameReceiver : public DATFrameReceiver {
public:
  DDSFrameReceiver();
  ~DDSFrameReceiver();

  //
  // Dump recovered data to the given directory.
  //
  void DumpToDirectory(const char *dirname);
  
  //
  // Dump a specific session from the tape. Sessions are areas of the tape
  // in-between End-Of-Tape markers. Most tapes have only one session. In
  // data-recovery procedures, however, it can be useful to dump data beyond
  // an End-of-Tape marker.
  //
  void DumpSession(unsigned int number);
  
  ///////////////////////////////////////////////////////////////////////////
  // DATFrameReceiver interface
  //
  
  //
  // Check if two tracks pair into a frame.
  //
  bool IsFrame(const Track& a, const Track& b);

  //
  // Receive a DDS frame (a pair of tracks, A and B)
  //
  void ReceiveFrame(const Track& a, const Track& b);

  //
  // Handle end of input.
  //
  void Stop();

  // end of DATFrameReceiver methods
  ///////////////////////////////////////////////////////////////////////////

protected:
  void AddFrame(DDSGroup3& frame);
  void NewGroup(uint32_t group_id);
  void DumpGroup();
  void GenerateGroupFilenames(uint32_t group_id, char*& groupFilename,
         char*& validName, char*& eccName, char*& eccValidName);
  
  //
  // Things about the last frame that we've seen.
  // These items help the decoding process piece together the
  // frames it sees into a logical whole.
  //
  // Group.Frame -> (0-n).(0-23)
  //
  BasicGroup *mBasicGroup;
  bool     mHaveGroup;
  uint32_t mGroupNumber;
  
  //
  // Decoder state. Helps with session counting.
  //
  enum {
    DATA = 0,
    EOT
  } mState;

  //
  // The output directory for dumping, if configured.
  //
  char *mOutputDirectory;
  
  //
  // The session to dump.
  //
  unsigned int mDumpSession;
  
  //
  // The current session.
  //
  unsigned int mCurrentSession;
};

#endif
