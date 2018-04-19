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

#ifndef RDAT_BASIC_GROUP_H
#define RDAT_BASIC_GROUP_H

#include <stdint.h>
#include <stddef.h>
#include "DDSGroup1.h"

//
// A class for encapsulating a DDS "Basic Group" -- which is
// the smallest logical unit that can be written to the tape.
//
// A basic group is 126632 bytes in size and contains both user
// data and meta-data specifying where the set marks and record
// marks reside within that data.
//
class BasicGroup {
public:
  BasicGroup(uint32_t id);
  ~BasicGroup();
  
  static const size_t kSize = 126632;
  static const size_t kSubFrames = 22;
  
  typedef uint8_t DataArray[kSize];
  typedef bool    ValidArray[kSize];
  typedef uint8_t ECCDataArray[DDSGroup1::kSize];
  typedef bool    ECCValidArray[DDSGroup1::kSize];
  
  bool LoadFromFile(const char *datapath, const char *validpath,
               const char *eccpath, const char *eccvalidpath);
  bool AddSubFrame(const DDSGroup1& frame);
  bool DumpToFile(const char *datapath, const char *validpath,
             const char *eccpath, const char *eccvalidpath);
  
  //
  // Returns this group's Basic Group number. Basic group numbers are
  // unique and monotonically increasing across the entire tape.
  //           
  uint32_t BasicGroupID() const;
  
  //
  // With the help of the ECC3 data, correct any erasures that are still
  // present in this group.
  //
  bool Correct();
  
  //
  // Returns if every byte of this group has been corrected and deemed
  // valid.
  //
  bool OK() const;
  
  //
  // Returns the number of bytes with errors.
  //
  size_t ErrorCount() const;
  
  const DataArray& Data() const;
  const ValidArray& Valid() const;
  const ECCDataArray& ECCData() const;
  const ECCValidArray& ECCValid() const;
  
  DataArray& ModifiableData();
  ValidArray& ModifiableValid();
  ECCDataArray& ModifiableECCData();
  ECCValidArray& ModifiableECCValid();
  
protected:
  //
  // The group id.
  //
  uint32_t mBasicGroupID;
  
  DataArray mData;
  ValidArray mDataIsValid;

  //
  // Basic groups can optionally come with an error correction frame.
  //
  ECCDataArray mECCData;
  ECCValidArray  mECCDataIsValid;
};

#endif
