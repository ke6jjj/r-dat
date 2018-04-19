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

#ifndef RDAT_TRACK_H
#define RDAT_TRACK_H

#include <stdint.h>
#include "DATBlock.h"

//
// A track is a collection of data that is read with one swipe of the
// R-DAT head. It consists of 8 sub-code blocks, 128 data blocks, and then
// 8 more sub-code blocks.
//
// There are two R-DAT heads, the negative azimuth head (A) and the positive
// azimuth head (B), in an R-DAT machine. By design, each head best reads
// the tracks that were made by that head and is not suited to reading
// the tracks deposited by the other head.
//
// The higher-level data schemes, which include DAT (digital audio) and DDS
// (computer data) parcel out specific data for specific tracks, so it is
// important to known which head a track came from because there
// is nothing within the logical track data itself to distinguish it. So
// the entity collecting these tracks needs to specifically mark the track
// as being either an "A" or "B" track when it is instantiated.
//
// The DDS specification calls tracks "channels".
//
class Track {
public:
  typedef enum {
    HEAD_A, // Track was read with the negative azimuth head
    HEAD_B, // Track was read with the positive azimuth head
    HEAD_UNKNOWN
  } Head;

  Track(Head head);
  ~Track();

  //
  // A track contains 144 data blocks, each 32 bytes long.
  // Each of these blocks also comes with a header byte.
  //
  // Every header byte and every data byte comes with an indicator
  // marking whether or not that byte is valid.
  //
  // Blocks 0-127 are data blocks.
  // Blocks 128-143 are sub-code blocks.
  //
  static const unsigned int kBlocks = 144;
  static const unsigned int kBlockSize = 32;

  typedef uint8_t DataArray[kBlocks][kBlockSize];
  typedef uint8_t HeaderArray[kBlocks];
  typedef bool    ValidityArray[kBlocks][kBlockSize];
  typedef bool    HeaderValidityArray[kBlocks];
  typedef uint8_t SubcodeSignatureArray[7];

  //
  // Add a received block to this track.
  //  
  void AddBlock(const DATBlock& block);

  //
  // Track is complete. Apply error correction and decode sub-codes.
  //
  void Complete();

  void SetHead(Head head);
  
  //
  // Get the contents of the specific sub-code, if it was correctly
  // received. (This should only be called after error correction
  // has been applied.)
  //
  bool GetSubcode(int id, const uint8_t **out) const;
  const SubcodeSignatureArray& SubcodeSignature() const;
  bool GetControlID(uint8_t& control_id) const;
  bool GetDataID(uint8_t& data_id) const;
  
  //
  // Get access to the entire data block and meta-data.
  //
  const DataArray& Data() const;
  const HeaderArray& Headers() const;
  const ValidityArray& DataValid() const;
  const HeaderValidityArray& HeaderValid() const;
  DataArray& ModifiableData();
  ValidityArray& ModifiableDataValid();

  //
  // Get the head/channel that this track was read from (if known).
  //             
  Head GetHead(void) const;

  //
  // Return the number of C1 errors that were encountered during
  // error correction.
  //
  size_t C1Errors() const;
  size_t C1UncorrectableErrors() const;

  //
  // Return the number of C2 uncorrectable errors. (The number of
  // C2 corrected errors is the same as the number of C1 uncorrectable
  // errors, so there's no use repeating that metric under another name).
  //
  size_t C2UncorrectableErrors() const;
   
protected:
  //
  // Add the data bytes found inside this block.
  //
  void AddVerifiedBlock(const DATBlock& block);
  
  //
  // Add the data bytes found inside this block, but assign them
  // explicitly to the given block number, rather than trying to decode
  // the block number from the block header.
  //
  void AddGuessedBlock(uint8_t number, const DATBlock& block);
  
  //
  // Take the data from a block and assign it to a specific block line
  // in our frame array.
  //
  void DataFill(uint8_t number, const DATBlock& block);
  
  //
  // Which head this track was read from.
  //
  Head mHead;
  
  //
  // All of the sub-code packs that were read with this track.
  // These data areas are indexed first by sub-code pack identifier.
  // These are filled in after the track has undergone error correction
  // and has parsed the sub-code blocks.
  //
  uint8_t mSubcode[16][7];
  
  //
  // The validity of the sub-code packs.
  //
  bool    mSubcodeIsValid[16];
  
  //
  // The Control ID and Data ID from the sub-code area.
  //
  uint8_t mControlID;
  bool    mHaveControlID;
  uint8_t mDataID;
  bool    mHaveDataID;
  
  //
  // The sub-code signature of this track.
  //
  // A sub-code signature is a list of the sub-code types (0-16) found
  // in the sub-code area and the order in which they were encountered.
  // This can be very useful in identifying the equipment which made the
  // recording.
  //
  SubcodeSignatureArray mSubcodeSignature;
  
  //
  // The blocks that were read with this track.
  //
  DataArray mData;
  
  //
  // The validity of the bytes that were read into the data.
  // (This helps some of the error-correcting codes do their jobs better
  // if certain bytes are known to have decoded improperly).
  //
  ValidityArray mDataIsValid;
  
  //
  // The header bytes for each data block.
  //
  HeaderArray mHeader;
  
  //
  // The validity of the header byte for each block.
  //
  HeaderValidityArray mHeaderIsValid;

  //
  // The sequence number (block number) of the last block
  // we received.
  //
  bool    mHaveLastBlock;
  uint8_t mLastBlockNumber;

  //
  // After error checking, the error metrics will be filled in here.
  //
  size_t mC1Errors;
  size_t mC1UncorrectableErrors;
  size_t mC2UncorrectableErrors;
};

#endif
