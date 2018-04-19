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

#ifndef RDAT_DDS_GROUP_3_H
#define RDAT_DDS_GROUP_3_H

#include <stdint.h>
#include "Track.h"
#include "DATFrame.h"
#include "DDSSubcode.h"

//
// From ECMA DDS specification, section 9.3.3 G3 Sub-Group
//
// A G3 group is one of the 22 pieces that, together, make up a Basic
// Group, which is about 126k of continuous data from the host.
//
class DDSGroup3 {
public:
  DDSGroup3();
  ~DDSGroup3();
  
  //
  // 9.4.9.1.1 SW1 Byte
  //
  // Although it is documented deep within the section of the DDS standard
  // that deals with the encoding of raw 35-byte blocks, frames come with an
  // area identifier that help mark which region of the tape is being read.
  //
  // Area identifiers are 3-bit quantities (xxx). They always appear together
  // with a single bit "partition" marker (Y) which appears at the
  // most-significant nibble of the two combined:
  //
  //    Yxxx
  //
  typedef enum {
    DEVICE_AREA = 0,
    REFERENCE_AREA = 1,
    SYSTEM_AREA = 2,
    DATA_AREA = 4,
    EOD_AREA = 5
  } AreaID;
  
  //
  // A tape can have two partitions.
  //
  typedef enum {
    PARTITION_ONE = 1,
    PARTITION_ZERO = 0
  } PartitionID;
  
  enum DecodeError {
    // Track pair ok. Frame complete.
    DECODE_OK,
    // A is missing its subcode 3 pack, can't determine frame number
    A_MISSING_SUBCODE_3,
    // B is missing its subcode 3 pack, can't determine frame number
    B_MISSING_SUBCODE_3,
    // Neither A nor B had a valid subcode 1 pack
    MISSING_SUBCODE_1,
    // Neither A nor B had a valid subcode 2 pack
    MISSING_SUBCODE_2,
    // Neither A nor B had a valid subcode 4 pack
    MISSING_SUBCODE_4,
    // A and B are not from the same absolute frame.
    ABSOLUTE_FRAME_MISMATCH,
    // A and B are not from the same logical frame.
    LOGICAL_FRAME_MISMATCH,
    // Tape area ID is weird.
    UNKNOWN_AREA_ID,
    // Some of the bytes of the frame are likely corrupted as the
    // C2 error correction failed.
    C2_ERRORS_PRESENT,
    // Frame purports to be ok, but header row doesn't look right.
    INVALID_HEADER,
    // Fourth checksum mismatch (very hard to explain)
    ECC4_ERROR
  };

  ///////////////////////////////////////////////////////////////////////////
  // Construction methods.
  ///////////////////////////////////////////////////////////////////////////
  
  //
  // Decode a frame from the given track pair.
  //
  DecodeError DecodeFrame(const Track& A, const Track& B);

  ///////////////////////////////////////////////////////////////////////////
  // Informational/meta-data methods.  
  ///////////////////////////////////////////////////////////////////////////

  //
  // Access the underlying DAT frame object.
  //
  const DATFrame& Frame() const;

  //
  // The tape area to which this frame purports to belong.
  //
  AreaID Area() const;
  
  //
  // The partition to which this frame purports to belong.
  //
  PartitionID Partition() const;
  
  //
  // Absolute frame number. This frame's id relative to the entire
  // tape.
  //
  uint32_t AbsoluteFrameID() const;
  
  //////////////////////////////////////////////
  // Data area frames also support the following
  //
    
  //
  // The running Basic Group count.
  //
  uint32_t BasicGroupID() const;

  //
  // The running Separator 1 count (file count).
  //
  uint32_t Separator1Count() const;
  
  //
  // The running record count.
  //
  uint32_t RecordCount() const;

  //
  // The logical frame number. (This G1 group's position within the larger
  // Basic Group it is a part of).
  //
  // I believe this is only valid for frames in the DATA area.
  //
  uint8_t LogicalFrameID() const;
  
  //
  // Whether this is the last frame in the Basic Group.
  //
  // Again, probably only valid for frames in the DATA area.
  //
  bool IsLastLogicalFrame() const;
  
  //
  // Whether or not this is an ECC3 frame. Generally if this is true
  // then IsLastLogicalFrame() should also be true.
  //
  bool IsECC3Frame() const;

  //
  // End Data area specific items.
  ///////////////////////////////////////////////  
  
  //
  // Give a human-readable description of the given decode error.
  //
  static const char * ErrorDescription(DecodeError r);
  
protected:
  DecodeError HandleDataAreaFrame(const DDSSubcodePack3& sub3, const Track& A,
                                  const Track& B);

  //
  // The tape area to which this frame purports to belong.
  //
  AreaID mAreaID;
  
  //
  // Identifiers which are common to all tape areas.
  //
  PartitionID mPartitionID;
  uint32_t mAbsoluteFrameID;
  
  //
  // For data areas, the following record the frame's position within
  // the data set.
  //
  // Group.Frame
  //
  uint32_t mBasicGroupID;
  uint8_t  mLogicalFrameID;
  bool     mIsLastLogicalFrame;
  bool     mIsECC3Frame;

  //
  // Within a group any number of boundaries can occur:
  //
  //  - A new file (Separator 1)
  //  - A new set mark (Separator 2)
  //  - A new record (Record mark)
  //
  // Each group records the current running tally of each of these marks:
  //
  uint32_t mSeparator1Count;
  uint32_t mSeparator2Count;
  uint32_t mRecordCount;
  
  //
  // The data in this group.
  //
  DATFrame mFrame;
};

#endif
