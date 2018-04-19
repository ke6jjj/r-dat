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
#include "Track.h"
#include "ECC_C1.h"
#include "ECC_C2.h"
#include "ECCFill_C1.h"
#include "ECCFill_C2.h"

static bool BlockHeaderIsValid(const DATBlock& block);

Track::Track(Head head)
  : mHead(head), mHaveLastBlock(false), mC1Errors(0), mC1UncorrectableErrors(0),
    mC2UncorrectableErrors(0), mHaveControlID(false), mHaveDataID(false)
{
  int i, j;
  
  //
  // Invalidate all parsed sub-codes.
  //
  for (i = 0; i < 16; i++)
    mSubcodeIsValid[i] = false;
  
  //
  // Invalidate the sub-code signature.
  //
  for (i = 0; i < 7; i++)
    mSubcodeSignature[i] = 0;
  
  //
  // Invalidate all blocks.
  //
  for (i = 0; i < kBlocks; i++) {
    for (j = 0; j < kBlockSize; j++) {
      mDataIsValid[i][j] = false;
      mData[i][j] = 0;
    }
    mHeaderIsValid[i] = false;
  }
}

Track::~Track()
{
}

Track::Head
Track::GetHead() const
{
  return mHead;
}

void
Track::SetHead(Head head)
{
  mHead = head;
}

void
Track::AddBlock(const DATBlock& block)
{
  if (!BlockHeaderIsValid(block)) {
    //
    // Block doesn't have a valid header, but there might reason to
    // accept it.
    //
    if (mHaveLastBlock && block.Size() == 36) {
      if ((mLastBlockNumber >= 0    && mLastBlockNumber < 0x7f) ||
          (mLastBlockNumber >= 0x88 && mLastBlockNumber < 0x8f) ||
          (mLastBlockNumber >= 0x80 && mLastBlockNumber < 0x88)) {
        //
        // This block doesn't have a valid header but we've received blocks
        // before it that are ok and we're still expecting blocks in this
        // region. It is likely that this block simply has a bad header but
        // its byte payload is just fine. Let's interpret this block as being
        // the next block in the sequence.
        //
        AddGuessedBlock(mLastBlockNumber + 1, block);
        mLastBlockNumber++;
      }
    }
    return;
  }
  
  //
  // Block header checks out. Add the block data to the location
  // that it identifies itself as.
  //
  AddVerifiedBlock(block);
}

//
// Get the contents of the specific sub-code, if it was correctly
// received.
//
bool
Track::GetSubcode(int id, const uint8_t **out) const
{
  if (id < 0 || id > 15 || !mSubcodeIsValid[id])
    return false;
  
  *out = &mSubcode[id][0];
  
  return true;
}

//
// Get access to the entire data block and meta-data.
//
const Track::DataArray&
Track::Data() const
{
  return mData;
}

const Track::ValidityArray&
Track::DataValid() const
{
  return mDataIsValid;
}

const Track::HeaderArray&
Track::Headers() const
{
  return mHeader;
}

const Track::HeaderValidityArray&
Track::HeaderValid() const
{
  return mHeaderIsValid;
}

Track::DataArray&
Track::ModifiableData()
{
  return mData;
}

Track::ValidityArray&
Track::ModifiableDataValid()
{
  return mDataIsValid;
}

const Track::SubcodeSignatureArray&
Track::SubcodeSignature() const
{
  return mSubcodeSignature;
}

//
// Track is supposedly complete.
// Correct errors and evaluate sub-codes.
//
void
Track::Complete()
{
  ECC_C1 Vp;
    
  //
  // Iterate over each pair of blocks, correcting the C1 errors in each.
  //
  for (ECCFill_C1 c1_fill(*this); !c1_fill.End(); c1_fill.Next()) {
    //
    // Fill the error check vector.
    //
    Vp.Fill(c1_fill);
    
    //
    // Detect errors in this vector and correct them.
    //
    ECC_C1::Status ok = Vp.Correct();
    switch (ok) {
    case ECC_C1::NO_ERRORS:
      //
      // No errors. Nothing to be done.
      //
      break;
    case ECC_C1::UNCORRECTABLE:
      mC1UncorrectableErrors++;
    case ECC_C1::CORRECTED:
      //
      // There were errors, they were either corrected or the whole vector
      // has been marked bad. Put the corrected or invalidated data back into
      // the track.
      //
      mC1Errors++;
      Vp.Dump(c1_fill);
      break;
    }
  }

  //
  // Now iterate over each block 4-group to perform C2 error correction.
  //
  ECC_C2 Vq;

  for (ECCFill_C2 c2_fill(*this); !c2_fill.End(); c2_fill.Next()) {
    //
    // Fill the error check vector.
    //      
    Vq.Fill(c2_fill);

    //
    // Detect errors in this vector and correct them.
    //
    ECC_C2::Status c2ok = Vq.Correct();
    switch (c2ok) {
    case ECC_C2::NO_ERRORS:
      //
      // No errors. Nothing to be done.
      //
      break;
    case ECC_C2::CORRECTED:
      //
      // There were errors but they were corrected.
      // Put the corrected data back into the track.
      //
      Vq.Dump(c2_fill);
      break;
    case ECC_C2::UNCORRECTABLE:
      //
      // Slice was uncorrectable. Leave it as is. The next level of
      // error handling (interpolation for Audio, C3 for DDS) will have
      // to deal with it.
      //
      mC2UncorrectableErrors++;
      break;
    }
  }

  //
  // Error correction is complete. Now dather data from the sub-code blocks.
  //
  //
  // Now all the sub-code blocks have had as much error correction applied
  // as they ever will. Build the sub-code signature.
  //
  // Examine blocks 0x80-0x8f, the sub-code blocks, and build up a complete
  // list of the seven sub-code identifiers used in this track and the order
  // in which they are specified.
  //
  bool have_subcode_slot[7];
  for (size_t i = 0; i < 7; i++)
    have_subcode_slot[i] = false;
  
  for (size_t i = 0; i < 16; i++) {
    //
    // Odd-numbered blocks have three sub-codes. Even blocks have four.
    //
    size_t limit = (i & 1) ? 3 : 4;
    size_t block_number = 0x80 + i;
    int    slot_start = (i & 1) ? 4 : 0;

    //
    // Grab the ControlID and DataID from the first valid even-numbered
    // Subcode block.
    //
    if ((block_number & 1) == 0) {
      if (!mHaveControlID && mHeaderIsValid[block_number]) {
        mControlID = (mHeader[block_number] & 0xf0) >> 4;
        mDataID    = (mHeader[block_number] & 0x0f);
        mHaveControlID = true;
        mHaveDataID = true;
      }
    }
    
    //
    // Examine the individal sub-codes in this block.
    //
    for (size_t j = 0; j < limit; j++) {
      //
      // Point straight at the 8-byte item.
      //
      const uint8_t *item = &(mData[block_number][8*j]);
      const bool    *validity = &(mDataIsValid[block_number][8*j]);

      //
      // What's the sub-code id?
      //
      if (!validity[0])
        //
        // Sub-code id byte isn't even valid. Don't bother.
        //
        continue;

      uint8_t subcode_id = (item[0] & 0xf0) >> 4;

      //
      // Do we already have this sub-code?
      //
      if (mSubcodeIsValid[subcode_id] && have_subcode_slot[slot_start+j])
        //
        // Already have it. Skip it.
        //
        continue;

      //
      // Check the whole subcode item parity and make sure there
      // are no erasure symbols here.
      //
      uint8_t parity;
      bool valid;
      size_t k;
      for (k = 0, parity = 0, valid = true; k < 8; k++) {
        parity ^= item[k] & 0xff;
        valid = valid && validity[k];
      }

      if (!valid || parity != 0)
        //
        // Item parity or validity is bad.
        //
        continue;
    
      if (!mSubcodeIsValid[subcode_id]) {
        //
        // Copy the sub-code into its slot and mark it valid.
        //
        for (k = 0; k < 7; k++)
          mSubcode[subcode_id][k] = item[k];
    
        mSubcodeIsValid[subcode_id] = true;
      }
      
      if (!have_subcode_slot[slot_start+j]) {
        //
        // Copy the sub-code identifier into the subcode signature.
        //
        mSubcodeSignature[slot_start+j] = subcode_id;
        have_subcode_slot[slot_start+j] = true;
      }
    }
  }
}

void
Track::AddVerifiedBlock(const DATBlock& block)
{
  //
  // What's this block's block number?
  //
  const uint16_t *bytes = block.FlaggedBytes();
  uint8_t block_number;
  if (bytes[2] & 0x80) {
    //
    // Sub-code block. Block number is 0x80-0x8f.
    //
    block_number = bytes[2] & 0x8f;
  } else {
    //
    // Data block. Block number is 0x00-0x7f
    //
    block_number = bytes[2] & 0xff;
  }
  
  
  //
  // Copy the header byte away.
  //
  mHeader[block_number] = bytes[1] & 0xff;
  mHeaderIsValid[block_number] = true;
  mHaveLastBlock = true;
  mLastBlockNumber = block_number;
  
  //
  // Copy the data.
  //
  DataFill(block_number, block);
}

void
Track::AddGuessedBlock(uint8_t block_number, const DATBlock& block)
{
  mHeaderIsValid[block_number] = false;
  DataFill(block_number, block);
}

void
Track::DataFill(uint8_t block_number, const DATBlock& block)
{
  //
  // Copy the bytes into our large block array while also marking
  // those bytes which are valid and invalid.
  //
  size_t count = block.Size();
  if (count < 4)
    return;

  count -= 4;

  if (count > 32)
    count = 32;
  
  const uint16_t *bytes = block.FlaggedBytes();
  for (size_t i = 0; i < count; i++) {
    mData[block_number][i] = bytes[i+4] & 0xff;
    mDataIsValid[block_number][i] = (bytes[i+4] & 0x8000) == 0;
  }
}

static bool
BlockHeaderIsValid(const DATBlock& block)
{
  //
  // (Blocks arrive with a SYNC byte prepended, hence the header starts
  // at byte 1).
  //
  if (block.Size() < 4) {
    //
    // Block doesn't even have enough bytes for a header.
    //
    return false;
  }
  
  const uint16_t *blockBytes = block.FlaggedBytes();
  
  //
  // Check that the header bytes are valid.
  //
  if ((blockBytes[1] | blockBytes[2] | blockBytes[3]) & DATBlock::INVALID) {
    //
    // One of the header bytes didn't even decode properly.
    //
    return false;
  }
  
  //
  // Check the parity of the block header.
  //
  uint8_t parity = (blockBytes[1] ^ blockBytes[2] ^ blockBytes[3]) & 0xff;
  
  if (parity != 0)
    return false;

  return true;
}

bool
Track::GetControlID(uint8_t& control_id) const
{
  if (!mHaveControlID)
    return false;
  
  control_id = mControlID;
  
  return true;
}

bool
Track::GetDataID(uint8_t& data_id) const
{
  if (!mHaveDataID)
    return false;
  
  data_id = mDataID;
  
  return true;
}

size_t
Track::C1Errors() const
{
  return mC1Errors;
}

size_t
Track::C1UncorrectableErrors() const
{
  return mC1UncorrectableErrors;
}

size_t
Track::C2UncorrectableErrors() const
{
  return mC2UncorrectableErrors;
}
