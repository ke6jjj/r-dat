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
#include "BasicGroup.h"
#include "ECC_C3.h"
#include "ECCFill_C3.h"

//
// A class for encapsulating a DDS "Basic Group" -- which is
// the smallest logical unit that can be written to the tape.
//
// A basic group is 126632 bytes in size and contains both user
// data and meta-data specifying where the set marks and record
// marks reside within that data.
//
BasicGroup::BasicGroup(uint32_t id)
  : mBasicGroupID(id)
{
  for (size_t i = 0; i < kSize; i++) {
    mData[i] = 0;
    mDataIsValid[i] = false;
  }
  
  for (size_t i = 0; i < DDSGroup1::kSize; i++) {
    mECCData[i] = 0;
    mECCDataIsValid[i] = false;
  }
}

BasicGroup::~BasicGroup()
{
}

bool
BasicGroup::LoadFromFile(const char *datapath, const char *validpath,
  const char *eccpath, const char *eccvalidpath)
{
  FILE *data_fd, *valid_fd, *ecc_fd, *eccvalid_fd;
  DataArray data, valid;
  DDSGroup1::DataArray ecc, eccvalid;
  bool res = false;
  
  data_fd = fopen(datapath, "rb");
  if (data_fd == NULL)
    goto CantOpenData;

  valid_fd = fopen(validpath, "rb");
  if (valid_fd == NULL)
    goto CantOpenValid;
  
  ecc_fd = fopen(eccpath, "rb");
  if (ecc_fd == NULL)
    goto CantOpenECC;
  
  eccvalid_fd = fopen(eccvalidpath, "rb");
  if (eccvalid_fd == NULL)
    goto CantOpenECCValid;
    
  //
  // Read in all the data items to temporary places.
  //
  if (fread(data, sizeof(data), 1, data_fd) != 1)
    goto ShortDataRead;

  if (fread(valid, sizeof(valid), 1, valid_fd) != 1)
    goto ShortValidRead;

  if (fread(ecc, sizeof(ecc), 1, ecc_fd) != 1)
    goto ShortECCRead;

  if (fread(eccvalid, sizeof(eccvalid), 1, eccvalid_fd) != 1)
    goto ShortECCValidRead;
  
  //
  // Copy the temporary data into our member variables, translating
  // booleans where needed.
  //
  for (size_t i = 0; i < kSize; i++) {
    mData[i] = data[i];
    mDataIsValid[i] = valid[i] != 0;
  }
  for (size_t i = 0; i < DDSGroup1::kSize; i++) {
    mECCData[i] = ecc[i];
    mECCDataIsValid[i] = eccvalid[i] != 0;
  }
  
  res = true;

ShortECCValidRead:
ShortECCRead:
ShortValidRead:
ShortDataRead:
  fclose(eccvalid_fd);
CantOpenECCValid:
  fclose(ecc_fd);
CantOpenECC:
  fclose(valid_fd);
CantOpenValid:
  fclose(data_fd);
CantOpenData:
  return res;
}

bool
BasicGroup::DumpToFile(const char *datapath, const char *validpath,
  const char *eccpath, const char *eccvalidpath)
{
  FILE *data_fd, *valid_fd, *ecc_fd, *eccvalid_fd;
  DataArray valid;
  DDSGroup1::DataArray eccvalid;
  bool res = false;
  
  data_fd = fopen(datapath, "wb");
  if (data_fd == NULL)
    goto CantOpenData;

  valid_fd = fopen(validpath, "wb");
  if (valid_fd == NULL)
    goto CantOpenValid;
  
  ecc_fd = fopen(eccpath, "wb");
  if (ecc_fd == NULL)
    goto CantOpenECC;
  
  eccvalid_fd = fopen(eccvalidpath, "wb");
  if (eccvalid_fd == NULL)
    goto CantOpenECCValid;
  
  //
  // Create character arrays from all boolean items.
  //
  for (size_t i = 0; i < kSize; i++)
    valid[i] = mDataIsValid[i] ? 0xff : 0;
  
  for (size_t i = 0; i < DDSGroup1::kSize; i++)
    eccvalid[i] = mECCDataIsValid[i] ? 0xff : 0;
  
  //
  // Write all data items, .
  //
  if (fwrite(mData, sizeof(mData), 1, data_fd) != 1)
    goto CantWriteData;
  if (fwrite(valid, sizeof(valid), 1, valid_fd) != 1)
    goto CantWriteValid;
  if (fwrite(mECCData, sizeof(mECCData), 1, ecc_fd) != 1)
    goto CantWriteECC;
  if (fwrite(eccvalid, sizeof(eccvalid), 1, eccvalid_fd) != 1)
    goto CantWriteECCValid;
  
  res = true;

CantWriteECCValid:
CantWriteECC:
CantWriteValid:
CantWriteData:
  fclose(eccvalid_fd);
CantOpenECCValid:
  fclose(ecc_fd);
CantOpenECC:
  fclose(valid_fd);
CantOpenValid:
  fclose(data_fd);
CantOpenData:
  return res;  
}

bool
BasicGroup::AddSubFrame(const DDSGroup1& frame)
{
  //
  // Make certain that this sub-frame belongs to this group.
  //
  if (frame.BasicGroupID() != mBasicGroupID) {
    printf("Attempt to add sub-frame to wrong basic group!\n");
    return false;
  }

  //
  // Don't dump sub-frame zero, if you ever encounter one.
  //
  if (frame.SubFrameID() == 0)
    return true;

  //
  // ECC frames get special treatment.
  //
  bool is_ecc = frame.IsECCFrame();
    
  //
  // Derive pointers into the appropriate place in out data to capture
  // this new frame.
  // 
  uint8_t *data_existing;
  bool *valid_existing;
  if (!is_ecc) {
    //
    // Normal data frame. Frames are numbered starting at 1 and are
    // about 5k in size.
    //
    size_t pos = DDSGroup1::kSize * (frame.SubFrameID() - 1);
    data_existing = &mData[pos];
    valid_existing = &mDataIsValid[pos];
  } else {
    //
    // ECC frames go into their own buffer.
    //
    data_existing = &mECCData[0];
    valid_existing = &mECCDataIsValid[0];
  }
  
  //
  // Get pointers to the frame data and validity buffer.
  //
  const DDSGroup1::DataArray& data = frame.Data();
  const DDSGroup1::ValidArray& valid = frame.Valid();
  
  //
  // Incorporate the frame's data as long as it is better than the existing
  // data.
  //
  for (size_t i = 0; i < DDSGroup1::kSize; i++) {
    if (valid[i] && !valid_existing[i]) {
      //
      // New valid data that replaces invalid data. Use it!
      //
      data_existing[i] = data[i];
      valid_existing[i] = true;
    } else if (valid[i] && valid_existing[i]) {
      //
      // We already have data at this spot. Check to make sure that the
      // update is the same.
      //
      if (data[i] != data_existing[i]) {
        //
        // Oh oh, mistmatch.
        //
        printf("Reread mismatch at Basic Group %d, SubGroup %d, offset %zd"
               ". Old/New = %02x/%02x. Keeping existing data.\n",
               frame.BasicGroupID(), frame.SubFrameID(), i, data_existing[i],
               data[i]);
      }
    } else if (!valid[i] && !valid_existing[i]) {
      //
      // Existing data is invalid and incoming data is also marked invalid,
      // but the incoming data likely has more information than the existing.
      // Sometimes entire blocks get marked invalid due to error detection
      // but a good deal of the block is actually correct. We have enough
      // storage space to represent the data invalidity yet retain the likely
      // correct data.
      //
      // Copy in the supposedly invalid data.
      //
      data_existing[i] = data[i];
      valid_existing[i] = valid[i];
    }
  }
  
  return true;
}  

//
// With the help of the ECC3 data, correct any erasures that are still
// present in this group.
//
bool
BasicGroup::Correct()
{
  ECC_C3 C3;
  size_t uncorrectableErrors = 0;
  
  for (ECCFill_C3 c3_fill(*this); !c3_fill.End(); c3_fill.Next()) {
    //
    // Fill the error check vector.
    //      
    C3.Fill(c3_fill);

    //
    // Detect errors in this vector and correct them.
    //
    ECC_C3::Status ok = C3.Correct();
    switch (ok) {
    case ECC_C3::NO_ERRORS:
      //
      // No errors. Nothing to be done.
      //
      break;
    case ECC_C3::CORRECTED:
      //
      // There were errors but they were corrected.
      // Put the corrected data back into the track.
      //
      C3.Dump(c3_fill);
      break;
    case ECC_C3::UNCORRECTABLE:
      //
      // Slice was uncorrectable. Leave it as is.
      //
      uncorrectableErrors++;
      break;
    }
  }
  
  return uncorrectableErrors == 0;
}

uint32_t
BasicGroup::BasicGroupID() const
{
  return mBasicGroupID;
}

const BasicGroup::DataArray&
BasicGroup::Data() const
{
  return mData;
}

const BasicGroup::ValidArray&
BasicGroup::Valid() const
{
  return mDataIsValid;
}

const BasicGroup::ECCDataArray&
BasicGroup::ECCData() const
{
  return mECCData;
}

const BasicGroup::ECCValidArray&
BasicGroup::ECCValid() const
{
  return mECCDataIsValid;
}

BasicGroup::DataArray&
BasicGroup::ModifiableData()
{
  return mData;
}

BasicGroup::ValidArray&
BasicGroup::ModifiableValid()
{
  return mDataIsValid;
}

BasicGroup::ECCDataArray&
BasicGroup::ModifiableECCData()
{
  return mECCData;
}

BasicGroup::ECCValidArray&
BasicGroup::ModifiableECCValid()
{
  return mECCDataIsValid;
}
