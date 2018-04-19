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
#include <stdlib.h>
#include "DDSFrameReceiver.h"
#include "DDSGroup1.h"

DDSFrameReceiver::DDSFrameReceiver()
  :  mOutputDirectory(NULL), mHaveGroup(false), mBasicGroup(NULL),
     mCurrentSession(0), mDumpSession(0), mState(DATA)
{
}

DDSFrameReceiver::~DDSFrameReceiver()
{
  delete mBasicGroup;
  free(mOutputDirectory);
}

//
// Dump recovered data to the given directory.
//
void
DDSFrameReceiver::DumpToDirectory(const char *dirname)
{
  free(mOutputDirectory);
  mOutputDirectory = strdup(dirname);
}

void
DDSFrameReceiver::DumpSession(unsigned int session_number)
{
  mDumpSession = session_number;
}

bool
DDSFrameReceiver::IsFrame(const Track& A, const Track& B)
{
  //
  // Are these two tracks a pair (a frame) or are they
  // separate? We must check subcode 3, the absolute frame number.
  //
  const uint8_t *A_absframe;
  const uint8_t *B_absframe;
  bool A_good;
  bool B_good;
      
  //
  // Examine their absolute frame numbers. (Subcode 3).
  //
  A_good = A.GetSubcode(3, &A_absframe);
  B_good = B.GetSubcode(3, &B_absframe);

  //
  // These tracks pair if their absolute frame numbers match.
  //
  return (A_good && B_good && memcmp(A_absframe, B_absframe, 7) == 0);
}

void
DDSFrameReceiver::ReceiveFrame(const Track& a, const Track& b)
{
  //
  // Construct a new DDS Group 3 object to receive the track pair
  //
  DDSGroup3 frame;
  
  //
  // Combine the tracks into a frame and do all the necessary
  // de-interleaving.
  //
  DDSGroup3::DecodeError result = frame.DecodeFrame(a, b);
  
  //
  // Dump information about the frame.
  //
  const char *area_name;
  switch (frame.Area()) {
  case DDSGroup3::DEVICE_AREA:
    area_name = "DEVICE";
    break;
  case DDSGroup3::REFERENCE_AREA:
    area_name = "REFERENCE";
    break;
  case DDSGroup3::SYSTEM_AREA:
    area_name = "SYSTEM";
    break;
  case DDSGroup3::DATA_AREA:
    area_name = "DATA";
    break;
  case DDSGroup3::EOD_AREA:
    area_name = "END-OF-DATA";
    break;
  default:
    area_name = "?";
    break;
  }
  
  printf("\n");

  if (result != DDSGroup3::DECODE_OK) {
    printf("Group 3 decode: %s\n",
      DDSGroup3::ErrorDescription(result)
    );
  }
  
  printf("Area          : %s\n", area_name);
  printf("Absolute frame: %06d\n", frame.AbsoluteFrameID());
  printf("Basic Group   : %05d\n", frame.BasicGroupID());
  printf("Sub frame     : %02d", frame.LogicalFrameID());
  if (frame.IsLastLogicalFrame())
    printf(" (Last of group)");
  if (frame.IsECC3Frame())
    printf(" (ECC3)");
  printf("\n");
  printf("File          : %04d\n", frame.Separator1Count());
  printf("Record        : 0x%08x\n", frame.RecordCount());

  //
  // Print out error statistics for the frame.
  //
  const DATFrame& underFrame = frame.Frame();

  unsigned int c1_errors = underFrame.C1Errors();
  unsigned int c1_uncorrectable = underFrame.C1UncorrectableErrors();
  unsigned int c1_corrected = c1_errors - c1_uncorrectable;
  unsigned int c2_uncorrectable = underFrame.C2UncorrectableErrors();
  unsigned int c2_corrected = c1_uncorrectable - c2_uncorrectable;

  printf("Errors  C1/C2 : %d/%d",
    c1_corrected,
    c2_corrected
  );

  if (c2_uncorrectable > 0)
    printf(" %d UNCORRECTED\n", c2_uncorrectable);
  else
    printf(" (all corrected)\n");

  switch (mState) {
  case DATA:
    //
    // In data state until we see an END-OF-DATA marker.
    //
    if (frame.Area() == DDSGroup3::EOD_AREA)
      mState = EOT;
    break;
  case EOT:
    //
    // We're in EOT state until we see another area.
    //
    if (frame.Area() != DDSGroup3::EOD_AREA) {
      //
      // We're in a new tape session.
      //
      mCurrentSession++;
      mState = DATA;
      printf("------------------------ START OF SESSION %d\n", mCurrentSession);
    }
  }
  
  //
  // If we hit an End-of-File mark we need to stop; any data after this mark
  // may have duplicate group identifiers and will corrupt any existing
  // groups before it.
  //
  if (mOutputDirectory != NULL && mCurrentSession == mDumpSession) {
    if (frame.Area() == DDSGroup3::EOD_AREA) {
      if (mHaveGroup) {
        DumpGroup();
      }
    } else if (frame.Area() == DDSGroup3::DATA_AREA) {
      AddFrame(frame);
    }
  }

  fflush(stdout);
}

void
DDSFrameReceiver::Stop()
{
  if (mHaveGroup && mOutputDirectory != NULL)
    DumpGroup();
}

void
DDSFrameReceiver::AddFrame(DDSGroup3& frame)
{
  //
  // Is this a continuation of the last group or a new group?
  //
  if (mHaveGroup && mGroupNumber != frame.BasicGroupID()) {
    //
    // We have a basic group but this frame is outside of it.
    // Dump the current group.
    //
    DumpGroup();
  }
  
  //
  // Is this the start of a new group?
  //
  if (!mHaveGroup) {      
    //
    // Prepare a new group with this frame's group number.
    // This may also entail loading anything we currently know about this
    // group from a previous decoding process.
    //
    NewGroup(frame.BasicGroupID());
  }
  
  //
  // De-whiten the data into a G1 group.
  //
  DDSGroup1 g1(frame);
  
  //
  // Copy it into the basic group in the appropriate spot.
  //
  mBasicGroup->AddSubFrame(g1);
    
  //
  // If this was the last frame of the group, dump the group.
  //
  if (frame.IsLastLogicalFrame())
    DumpGroup();
}

void
DDSFrameReceiver::NewGroup(uint32_t group_id)
{
  //
  // Construct a new group object.
  //
  mBasicGroup = new BasicGroup(group_id);
  
  //
  // Figure out what file name we would have previously stored this
  // group's data under, if available.
  //
  char *groupFilename, *validName, *eccName, *eccValidName;
  groupFilename = validName = eccName = eccValidName = NULL;
  GenerateGroupFilenames(group_id, groupFilename, validName, eccName,
    eccValidName);
  
  //
  // Load any previous data for this group.
  //
  mBasicGroup->LoadFromFile(groupFilename, validName, eccName, eccValidName);
  
  free(groupFilename);
  free(validName);
  free(eccName);
  free(eccValidName);

  //
  // Mark that we now have a group.
  //
  mHaveGroup = true;
  mGroupNumber = group_id;
}

void
DDSFrameReceiver::DumpGroup()
{
  //
  // Perform ECC3 correction on the group. It's likely complete now.
  //
  bool correct = mBasicGroup->Correct();
  uint32_t group_id = mBasicGroup->BasicGroupID();
  printf("Group ECC3    : %s (Group %d)\n", correct ? "GOOD" : "----BAD---",
         group_id);
  printf("------------------------------------------------------------\n");
  
  //
  // Figure out what file name we should store this group's data under.
  //
  char *groupFilename, *validName, *eccName, *eccValidName;
  groupFilename = validName = eccName = eccValidName = NULL;
  GenerateGroupFilenames(group_id, groupFilename, validName, eccName,
    eccValidName);
  
  //
  // Dump the group data to disk.
  //
  mBasicGroup->DumpToFile(groupFilename, validName, eccName, eccValidName);

  free(groupFilename);
  free(validName);
  free(eccName);
  free(eccValidName);

  //
  // Mark that we now have no group.
  //
  mHaveGroup = false;
  delete mBasicGroup;
  mBasicGroup = NULL;
}

void
DDSFrameReceiver::GenerateGroupFilenames(uint32_t group_id,
  char*& groupFilename, char*& validName, char*& eccName, char*& eccValidName)
{
  asprintf(&groupFilename, "%s/g%06d.bin", mOutputDirectory, group_id);
  asprintf(&validName,     "%s/g%06d.val", mOutputDirectory, group_id);
  asprintf(&eccName,       "%s/g%06d.ecc.bin", mOutputDirectory, group_id);
  asprintf(&eccValidName,  "%s/g%06d.ecc.val", mOutputDirectory, group_id);
}
