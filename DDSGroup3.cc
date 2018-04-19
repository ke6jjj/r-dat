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

#include <string.h>
#include "DDSGroup3.h"
#include "DDSSubcode.h"

static bool GetSubcodePack(unsigned int id, const Track& A, const Track& B,
                           const uint8_t **data);
                           
//
// This is an inline helper function for locating the row and column number
// for a given Group 3 data byte "i", called "Di".
//
// Being foremost an interpretation of a DATFrame, a DDS Group 3 frame
// has 1456 rows, each of which is 4-bytes long.
//
// 9.3.3 G3 Sub-Group
//
//               +-----------------+-----------------+
// Channel    -> |        A        |        B        |
//               +--------+--------+--------+--------+
// Byte name  -> | lower  | upper  | lower  | upper  |
//        +------+========+========+========+========+
// Header |    0 |0000DFID| LF-ID  |0000DFID|  LF-ID |
//        +------+--------+--------+--------+--------+
//        |    1 |   D0   |   D1   |   D2   |   D3   |
//        +------+--------+--------+--------+--------+
//        |    2 |   D4   |   D5   |   D6   |   D7   |
//        +------+--------+--------+--------+--------+
//        |  ... |   ...  |   ...  |   ...  |   ...  |
//        +------+--------+--------+--------+--------+
//        | 1439 | D5752  | D5853  | D5754  | D5755  |
//        +------+--------+--------+--------+--------+
//        | 1440 |   All bytes set to zero           |
//        |  ... |                                   |
//        | 1455 |                                   |
//        +------+-----------------------------------+
//
static inline uint8_t
Di(const DATFrame::DataArray& data, size_t i)
{
  size_t row = (i/4) + 1;
  size_t col = (i%4);
  return data[row][col];
}

//
// From ECMA DDS specification, section 9.3.3 G3 Sub-Group
//
// A G3 group is one of the 22 pieces that, together, make up a Basic
// Group, which is about 126k of continuous data from the host.
//
DDSGroup3::DDSGroup3()
  : mIsLastLogicalFrame(false), mAbsoluteFrameID(0),
    mLogicalFrameID(0)
{
}

DDSGroup3::~DDSGroup3()
{
}

//
// The underlying frame object upon which this group 3 is built.
//
const DATFrame&
DDSGroup3::Frame() const
{
  return mFrame;
}

//
// The tape area to which this frame purports to belong.
//
DDSGroup3::AreaID
DDSGroup3::Area() const
{
  return mAreaID;
}

//
// The partition to which this frame purports to belong.
//
DDSGroup3::PartitionID
DDSGroup3::Partition() const
{
  return mPartitionID;
}

//
// Absolute frame number. This frame's id relative to the entire
// tape.
//
uint32_t
DDSGroup3::AbsoluteFrameID() const
{
  return mAbsoluteFrameID;
}

//////////////////////////////////////////////
// Data area frames also support the following
//

//
// The running basic group count.
//
uint32_t
DDSGroup3::BasicGroupID() const
{
  return mBasicGroupID;
}

uint8_t
DDSGroup3::LogicalFrameID() const
{
  return mLogicalFrameID;
}

//
// Whether this is the last frame in the G1 group.
//
// Again, probably only valid for frames in the DATA area.
//
bool
DDSGroup3::IsLastLogicalFrame() const
{
  return mIsLastLogicalFrame;  
}

//
// Whether or not this is an ECC3 frame. Generally if this is true
// then IsLastLogicalFrame() should also be true.
//
bool
DDSGroup3::IsECC3Frame() const
{
  return mIsECC3Frame;
}

//
// The running Separator 1 count (file count).
//
uint32_t
DDSGroup3::Separator1Count() const
{
  return mSeparator1Count; 
}

//
// The running record count.
//
uint32_t
DDSGroup3::RecordCount() const
{
  return mRecordCount;
}

//
// Construct a group from a received frame (also called a G4 group pair, and
// also called a track pair).
//
DDSGroup3::DecodeError
DDSGroup3::DecodeFrame(const Track& A, const Track& B)
{
  //
  // Retrieve the items from subcode id #3 from both tracks.
  // 
  const uint8_t *a_subcode, *b_subcode;
  
  if (!A.GetSubcode(3, &a_subcode))
    return A_MISSING_SUBCODE_3;
  if (!B.GetSubcode(3, &b_subcode))
    return B_MISSING_SUBCODE_3;
  
  //
  // Every track should have a valid subcode pack 3. Let's fetch it from
  // both tracks and make certain they match. (This may be redundant as
  // the frame pairing protocol is supposed to have checked this earlier).
  //
  DDSSubcodePack3 a3, b3;
  a3.Decode(a_subcode);
  b3.Decode(b_subcode);
    
  //
  // Assert that both tracks contain the same absolute frame number.
  //
  if (a3.mAbsoluteFrameID != b3.mAbsoluteFrameID)
    return ABSOLUTE_FRAME_MISMATCH;
    
  //
  // Record frame numbers and such.
  //
  mPartitionID = PartitionID(a3.mPartitionID);
  mAreaID   = AreaID(a3.mAreaID);
  mAbsoluteFrameID = a3.mAbsoluteFrameID;

  //
  // Fetch additional items, depending on the Area of the tape
  // we're supposedly in.
  //
  switch (mAreaID) {
  case DEVICE_AREA:
  case REFERENCE_AREA:
  case SYSTEM_AREA:
  case EOD_AREA:
    //
    // Not handling these areas yet.
    //
    return DECODE_OK;
  case DATA_AREA:
    //
    // Data area frames also have data, which we should check and store.
    //
    // Assert that both tracks of this famre also agree on their logical
    // frame number (this frame's position within the larger Basic Group).
    //
    if (a3.mLogicalFrameID != b3.mLogicalFrameID)
      return LOGICAL_FRAME_MISMATCH;
    
    return HandleDataAreaFrame(a3, A, B);
    break;
  default:
    return UNKNOWN_AREA_ID;
  }
}

const char *
DDSGroup3::ErrorDescription(DDSGroup3::DecodeError r)
{
  switch (r) {
  case DECODE_OK:
    return "DECODE_OK";
  case A_MISSING_SUBCODE_3:
    return "A_MISSING_SUBCODE_3";
  case B_MISSING_SUBCODE_3:
    return "B_MISSING_SUBCODE_3";
  case MISSING_SUBCODE_4:
    return "MISSING_SUBCODE_4";
  case ABSOLUTE_FRAME_MISMATCH:
    return "ABSOLUTE_FRAME_MISMATCH";
  case LOGICAL_FRAME_MISMATCH:
    return "LOGICAL_FRAME_MISMATCH";
  case UNKNOWN_AREA_ID:
    return "UNKNOWN_AREA_ID";
  case C2_ERRORS_PRESENT:
    return "C2_ERRORS_PRESENT";
  case INVALID_HEADER:
    return "INVALID_HEADER";
  case ECC4_ERROR:
    return "ECC4_ERROR";
  case MISSING_SUBCODE_1:
    return "MISSING_SUBCODE_1";
  case MISSING_SUBCODE_2:
    return "MISSING_SUBCODE_2";
  default:
    return "?";
  }
}

/////////////////////////////////////////////////////////////////////////////
// PROTECTED methods
/////////////////////////////////////////////////////////////////////////////

DDSGroup3::DecodeError
DDSGroup3::HandleDataAreaFrame(const DDSSubcodePack3& sub3, const Track& A,
                               const Track& B)
{
  //
  // Data area frames also have the following, which come from sub-code
  // pack 3.
  //
  mLogicalFrameID = sub3.mLogicalFrameID;
  mIsLastLogicalFrame = sub3.mIsLastLogicalFrame;
  mIsECC3Frame = sub3.mIsECC3Frame;
  
  //
  // Retrieve sub-code pack 1 to get the group number and separator 1
  // counts.
  //
  const uint8_t *subcode;

  if (!GetSubcodePack(1, A, B, &subcode))
    return MISSING_SUBCODE_1;
  
  DDSSubcodePack1 sub1;
  sub1.Decode(subcode);
  mBasicGroupID = sub1.mGroup;
  mSeparator1Count = sub1.mSeparator1Count;
  
  //
  // Retrieve sub-code pack 2 to get the separator 2 count and record
  // counts.
  //
  DDSSubcodePack2 sub2;
  
  if (!GetSubcodePack(2, A, B, &subcode))
    return MISSING_SUBCODE_2;
  
  sub2.Decode(subcode);
  mSeparator2Count = sub2.mSeparator2Count;
  mRecordCount = sub2.mRecordCount;
  
  //
  // Recreate the "LFID" byte from the LogicalFrameID, the
  // IsECC3 and the IsLastFrame flags.
  //
  uint8_t original_lfid = 
    mLogicalFrameID |
    (mIsECC3Frame ? 0x40 : 0) |
    (mIsLastLogicalFrame ? 0x80 : 0);
  
  //
  // Also fetch sub-code 4 for extra error-checking.
  //
  
  if (!GetSubcodePack(4, A, B, &subcode))
      return MISSING_SUBCODE_4;
  
  //
  // Decode the pack 4 subcode.
  //
  DDSSubcodePack4 sub4;
  sub4.Decode(subcode);

  //
  // Fill in the frame data by demultiplexing the data from both tracks.
  //
  mFrame.FillFromTrackPair(A, B);
  
  //
  // Make certain everything was received ok.
  //
  if (!mFrame.OK())
    return C2_ERRORS_PRESENT;

  //
  // Perform some other basic checks on the frame. First, let's check
  // that the head bytes on the group look good.
  //
  // The logical frame id should be repeated bytes 1 and 3 of the first
  // row of data.
  //
  const DATFrame::DataArray& data = mFrame.Data();
  
  if (data[0][1] != original_lfid ||
      data[0][3] != original_lfid ||
      //
      // Check that the format id is also zero.
      //
      data[0][0] != 0 ||
      data[0][2] != 0) {
    return INVALID_HEADER;  
  }

  //
  // Pick up the half column checksums from packs 3 and 4.
  //
  uint8_t c1 = sub3.mChecksum1;
  uint8_t c2 = sub3.mChecksum2;
  uint8_t c3 = sub4.mChecksum3;
  uint8_t c4 = sub4.mChecksum4;
  
  //
  // Prepare to perform half-column checksuming of the received data.
  //
  uint8_t r_c1 = 0;
  uint8_t r_c2 = 0;
  uint8_t r_c3 = 0;
  uint8_t r_c4 = 0;
  
  //
  // C1 and C3 start with the recreated LFID byte.
  //
  r_c1 = r_c3 = original_lfid;
  
  //
  // The rest of the error checking is convoluted.
  //
  for (size_t i = 0; i < 719; i++) {
    //
    // 9.4.3.3.1.1 Byte No. 5; i = [0,718]
    //
    r_c1 ^= Di(data, (8*i)+3) ^
            Di(data, (8*i)+5) ^
            Di(data, 5755); // This is dumb to repeat

    //
    // 9.4.3.3.1.2 Byte No. 6; i = [0,718]
    //
    r_c2 ^= Di(data, (8*i)+2) ^
            Di(data, (8*i)+4) ^
            Di(data, 5754); // This is dumb to repeat

    //
    // 9.4.4.3.1.1 Byte No. 5; i = [1,719]
    //
    r_c3 ^= Di(data, 1) ^   // This is dumb to repeat
            Di(data, 8*(i+1)-1) ^
            Di(data, 8*(i+1)+1);

    //
    // 9.4.4.3.1.2 Byte No. 6 [1-719]
    //
    r_c4 ^= Di(data, 0) ^  // This is dumb to repeat
            Di(data, 8*(i+1)-2) ^
            Di(data, 8*(i+1));
  }
  
  //
  // Check that the half-column weird checksums match the data.
  //
#if 0
  if ((r_c1 != c1) ||
      (r_c2 != c2) ||
      (r_c3 != c3) ||
      (r_c4 != c4))
    return ECC4_ERROR;
#endif
  
  return DECODE_OK;
}


//
// Get the first valid sub-code pack item we can from either track.
//
static bool
GetSubcodePack(unsigned int id, const Track& A, const Track& B,
               const uint8_t ** subcode)
{
  if (!A.GetSubcode(id, subcode)) {
    //
    // A's sub-code is missing. Perhaps we can get it from B.
    //
    if (!B.GetSubcode(id, subcode)) {
      //
      // Nope. Missing from both. We should have it.
      //
      return false;
    }
  }
  
  return true;
}
