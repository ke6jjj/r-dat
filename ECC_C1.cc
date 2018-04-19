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

#include "ECC_C1.h"
#include "ECC_GF28.h"
#include "ReedSolomon_EUA.h"

//
// The Reed-Solomon check matrix for DAT and DDS.
//
// This matrix is multiplied by a vector containing the bytes being
// checked. The result is a four element vector containing the error
// syndrome.
//
const uint8_t gHp[ECC_C1::kTwoT][ECC_C1::kN] = {
  {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01
  },
  {
    0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x8f,
    0xc9, 0xea, 0x75, 0xb4, 0x5a, 0x2d, 0x98, 0x4c,
    0x26, 0x13, 0x87, 0xcd, 0xe8, 0x74, 0x3a, 0x1d,
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
  },
  {
    0xde, 0xb9, 0x69, 0x5d, 0x50, 0x14, 0x05, 0x46,
    0x9f, 0xee, 0xb5, 0x6a, 0x94, 0x25, 0x4e, 0x9d,
    0x60, 0x18, 0x06, 0x8f, 0xea, 0xb4, 0x2d, 0x4c,
    0x13, 0xcd, 0x74, 0x1d, 0x40, 0x10, 0x04, 0x01
  },
  {
    0xb6, 0xdf, 0x7f, 0x6b, 0xe7, 0x78, 0x0f, 0x65,
    0x2f, 0x61, 0xa1, 0xb9, 0xba, 0x50, 0x0a, 0x46,
    0xc1, 0xb5, 0x35, 0x25, 0x27, 0x60, 0x0c, 0x8f,
    0x75, 0x2d, 0x26, 0xcd, 0x3a, 0x40, 0x08, 0x01
  }  
};
  
ECC_C1::ECC_C1()
{
  for (size_t i; i < kN; i++)
    mDataIsValid[i] = false;
}

ECC_C1::~ECC_C1()
{
}
  
void
ECC_C1::Fill(ECCFill& filler)
{
  for (size_t i = 0; i < kN; i++) {
    mData[i] = filler.Data(i);
    mDataIsValid[i] = filler.Valid(i);
  } 
}

bool
ECC_C1::ComputeSyndrome(uint8_t (&syndrome)[ECC_C1::kTwoT])
{
  //
  // Multiply the filled in vector by the check matrix to produce a
  // result vector.
  //
  bool ok = true;
  for (size_t i = 0; i < kTwoT; i++) {
    uint8_t result = 0;
    for (size_t j = 0; j < kN; j++) {
      result ^= ECC_GF28_multiply(mData[j], gHp[i][j]);
    }

    syndrome[i] = result;

    //
    // Everything is ok if all previous entries are zero and this
    // entry is zero.
    //
    ok = ok && result == 0;
  }

  return ok;
}

ECC_C1::Status
ECC_C1::Correct()
{
  uint8_t result_vector[kTwoT];
  uint8_t dummy_erasures_vector[kTwoT];
  size_t erasures;
  
  bool ok = true;
  bool corrected = false;
  
  //
  // Scan the input to determine how many erasures there are.
  //
  // Since this is the first level of error checking and the two
  // levels of error correction in DAT work best if the first level
  // corrects errors with 100% confidence, we will not use our known
  // erasure locations when it comes to processing this vector.
  //
  // Every erasure position we add reduces the error checking capability
  // of the code and we would rather have assurance that all bytes are
  // either correct or unknown when it comes to the C1 checking because
  // we are going to run the C2 check in erasures-only mode; it will have
  // no error detection at all.
  //
  erasures = 0;
  for (size_t i = 0; i < kN; i++) {
    if (!mDataIsValid[i]) {
      if (erasures >= kTwoT) {
        //
        // Too many erasures encountered. This vector will not be
        // correctable.
        //
        ok = false;
        break;
      }
      erasures++;
    }
  }
  
  if (ok) {
    //
    // The known erasures (if any) are under control.
    // Compute the syndrome of the current vector.
    //
    ok = ComputeSyndrome(result_vector);

    if (!ok) {
      //
      // There's a non-zero syndrome. Attempt to correct the errors.
      //
      ok = HandleSyndrome(result_vector, dummy_erasures_vector, 0);
      if (ok)
        corrected = true;
    } else {
      mCorrectionCount = 0;
    }
  }
  
  if (ok) {
    if (erasures || corrected) {
      //
      // The data entered with some erasures. It has now been fully
      // validated, so mark every byte as good.
      //
      for (size_t j = 0; j < kN; j++)
        //
        // If the code corrects two errors then it is much safer to assume
        // that there are further uncorrected errors. If not, then it
        // is safe to mark the whole vector good.
        //
        mDataIsValid[j] = mCorrectionCount < kTwoT;

      return CORRECTED;
    }
    //
    // Everything came in good. No modifications necessary.
    //
    return NO_ERRORS;
  } else {
    //
    // There are uncorrectable errors. Mark the whole vector as
    // invalid.
    //
    for (size_t j = 0; j < kN; j++)
      mDataIsValid[j] = false;

    return UNCORRECTABLE;
  }
}

void
ECC_C1::Dump(ECCFill& filler)
{
  for (size_t i = 0; i < kN; i++) {
    filler.Data(i) = mData[i];
    filler.Valid(i) = mDataIsValid[i];
  } 
}

bool
ECC_C1::HandleSyndrome(
  uint8_t syndrome[ECC_C1::kTwoT],
  const uint8_t erasures[ECC_C1::kTwoT],
  size_t numErasures)
{
  uint8_t locator[kTwoT+1], magnitude[kTwoT];
  
  //
  // Run the extended Euclidean algorithm, with erasures, to find the
  // error locator polynomial and the error magnitude polynomial.
  //
  bool correctable = RS_Solve<kT>(syndrome, erasures, numErasures, locator,
                                 magnitude);
  if (!correctable)
    return false;
  
  //
  // The errors should be correctable. Find the error locations by
  // testing roots of the error locator polynomial. Only test those
  // roots that correspond to locations in the code word.
  //
  uint8_t corrections[ECC_C1::kTwoT];
  size_t  correction_locations[ECC_C1::kTwoT];
  mCorrectionCount = 0;
  bool corrected = false;

  for (size_t i = 0; i < kN; i++) {
    //
    // Compute alpha^(-i)
    //
    uint8_t alpha_inv = ECC_GF28_invert(ECC_GF28_pow_alpha(i));
    
    //
    // Compute locator(alpha^(-i)).
    //
    uint8_t res = ECC_GF28_evaluate(locator, alpha_inv, kTwoT+1);

    //
    // If the result is zero then this location, i, has an error that
    // we can fix.
    //
    if (res == 0) {
      //
      // There's an error at this position. Use Forney's formula to
      // calculate the error value.
      //
      uint8_t correction = RS_GetErrorAtLocation<kT>(
        locator,
        magnitude,
        alpha_inv
      );
     
      //
      // The locations derived from this algorithm are relative
      // to the lowest-order element of the code word. This is
      // the opposite order that we store the codeword in memory.
      // Translate the algorithmic location to the memory location.
      // 
      size_t loc = kN - i - 1;

      //
      // Mark that there's a correction at this location.
      // 
      corrections[mCorrectionCount] = correction;
      correction_locations[mCorrectionCount] = loc;
      mCorrectionCount++;

      //
      // Update the syndrome with this correction.
      //
      corrected = true;
      for (size_t j = 0; j < kTwoT; j++) {
        syndrome[j] ^= ECC_GF28_multiply(correction, gHp[j][loc]);
        corrected = corrected && syndrome[j] == 0;
      }
    }
  }

  //
  // Do the planned corrections fix the syndrome completely?
  //
  if (corrected) {
    //
    // Yes. The planned corrections fix the syndrome. Let's
    // apply them.
    //
    for (size_t i = 0; i < mCorrectionCount; i++) {
      size_t location = correction_locations[i];
      uint8_t correction = corrections[i];

      //
      // Apply this correction.
      //
      mData[location] ^= correction;
    }
  }
  
  //
  // If the corrections didn't fix the problem then we have an uncorrectable
  // error syndrome.
  //
  return corrected;
}
