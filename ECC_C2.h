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

#ifndef RDAT_ECC_C2_H
#define RDAT_ECC_C2_H

#include <stdint.h>
#include <stddef.h>
#include "ECCFill.h"

class ECC_C2 {
public:
  ECC_C2();
  ~ECC_C2();
  
  typedef enum {
    NO_ERRORS = 0,
    CORRECTED,
    UNCORRECTABLE
  } Status;

  //
  // The block size and number of parity bytes for this code,
  // respectively.
  //
  static const unsigned int kN = 32;
  static const unsigned int kTwoT = 6;
  static const unsigned int kT = kTwoT / 2;

  //
  // Fill this vector from the given source.
  //
  void Fill(ECCFill& fill);

  //
  // Correct this vector, if possible. Returns the correction
  // status.
  //
  Status Correct();

  //
  // Dump this corrected (or invalidated) vector back to its
  // source.
  //
  void Dump(ECCFill& fill);

protected:
  bool ComputeSyndrome(uint8_t syndrome[kTwoT]);
  bool HandleSyndrome(uint8_t syndrome[kTwoT],
     const uint8_t erasures[kTwoT], size_t numErasures);
  
  //
  // The data in the vector.
  //
  uint8_t mData[kN];
  
  //
  // Known erasures in this vector.
  //
  bool    mDataIsValid[kN];
};

#endif
