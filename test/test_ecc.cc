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

#include "tests.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "ECC_C1.h"

static const struct ECC_Test {
  const char *input[2];
  const char *erasures[2];
  const char *answer[2];
  ECC_C1::Status results[2];
} Tests[] = {
  {
    //
    // No errors in either vector.
    //
    {
      "20aaaa000024131756940729193914d820aaaa000024131720aaaa0000241317",
      "20aaaa000024131756940729193914d820aaaa0000241317abbbe79542da976d"
    },
    { NULL, NULL },
    { NULL, NULL },
    { ECC_C1::NO_ERRORS, ECC_C1::NO_ERRORS }
  },
  {
    // 
    // Two errors in odd vector (31, 0).
    //
    {
      "20abaa000024131756940729193914d820aaaa000024131720aaaa0000241317",
      "20aaaa000024131756940729193914d820aaaa0000241317abbbe79542da976e"
    },
    { NULL, NULL },
    {
      "20aaaa000024131756940729193914d820aaaa000024131720aaaa0000241317",
      "20aaaa000024131756940729193914d820aaaa0000241317abbbe79542da976d"
    },
    { ECC_C1::NO_ERRORS, ECC_C1::CORRECTED }
  },
  {
    // 
    // Three errors in odd vector (31, 30, 0).
    //
    {
      "20abaa010024131756940729193914d820aaaa000024131720aaaa0000241317",
      "20aaaa000024131756940729193914d820aaaa0000241317abbbe79542da976e"
    },
    { NULL, NULL },
    { NULL, NULL },
    { ECC_C1::NO_ERRORS, ECC_C1::UNCORRECTABLE }
  },
  {
    // 
    // Two errors in both vectors (30, 1).
    //
    {
      "20aaFFFF0024131756940729193914d820aaaa000024131720aaaa0000241317",
      "20aaaa000024131756940729193914d820aaaa0000241317abbbe795FFFF976d"
    },
    { NULL, NULL },
    {
      "20aaaa000024131756940729193914d820aaaa000024131720aaaa0000241317",
      "20aaaa000024131756940729193914d820aaaa0000241317abbbe79542da976d"
    },
    { ECC_C1::CORRECTED, ECC_C1::CORRECTED }
  },
  {
    //
    // One erasure indicator in odd vector (0), but correct byte is
    // still there.
    //
    {
      "20aaaa000024131756940729193914d820aaaa000024131720aaaa0000241317",
      "20aaaa000024131756940729193914d820aaaa0000241317abbbe79542da976d"
    },
    {
      "0000000000000000000000000000000000000000000000000000000000000000",
      "0000000000000000000000000000000000000000000000000000000000000011"
    },
    {
      "20aaaa000024131756940729193914d820aaaa000024131720aaaa0000241317",
      "20aaaa000024131756940729193914d820aaaa0000241317abbbe79542da976d"
    },
    { ECC_C1::NO_ERRORS, ECC_C1::CORRECTED }
  },
  {
    //
    // One erasure indicator in odd vector (0), and correct byte is
    // gone.
    //
    {
      "20aaaa000024131756940729193914d820aaaa000024131720aaaa0000241317",
      "20aaaa000024131756940729193914d820aaaa0000241317abbbe79542da97FF"
    },
    {
      "0000000000000000000000000000000000000000000000000000000000000000",
      "0000000000000000000000000000000000000000000000000000000000000011"
    },
    {
      "20aaaa000024131756940729193914d820aaaa000024131720aaaa0000241317",
      "20aaaa000024131756940729193914d820aaaa0000241317abbbe79542da976d"
    },
    { ECC_C1::NO_ERRORS, ECC_C1::CORRECTED }
  },
  {
    //
    // Eight erasures (four in each block). C1 will mark this as bad
    // because it should favor error detection over correction and will
    // not feed erasures into the corrector.
    //
    {
      "20aaaa000024131756940729193914d820aaaa000024131720aaaa0000241317",
      "20aaaa000024131756940729193914d820aaaa00002413170000000000000000"
    },
    {
      "0000000000000000000000000000000000000000000000000000000000000000",
      "0000000000000000000000000000000000000000000000001111111111111111"
    },
    { NULL, NULL },
    { ECC_C1::UNCORRECTABLE, ECC_C1::UNCORRECTABLE }
  },
  {
    //
    // A real-world vector that eluded error correction. This vector
    // has more errors than can be corrected, but a corner case in
    // my original algorithm perceived this as correctable and made
    // the wrong correction.
    //
    {
      "b10043005500af00fc00580029009f00d50073004800bd001a002100e5001700",
      "0a00420038009700af00770020000200a8009d00a5006a00670000000e00f100"
    },
    { NULL, NULL },
    { NULL, NULL },
    { ECC_C1::UNCORRECTABLE, ECC_C1::NO_ERRORS }
  }
};

static bool c1_test(const ECC_Test *test);
static const char * ecc_status(ECC_C1::Status result);

void
test_ecc(TestSession& ts)
{
  for (size_t i = 0; i < sizeof(Tests) / sizeof(Tests[0]); i++) {
    ts.BeginTest("ECC %zd", i);
    bool result = c1_test(&Tests[i]);
    ts.EndTest(result);
  }
}

class BlockPair : public ECCFill {
public:
  static const size_t kBlockSize = 32;
  static const size_t kBlocks = 2;

  BlockPair(const char * const data[kBlocks], const char * const erase[kBlocks]);
  ~BlockPair() {};

  void     FillFrom(size_t offset);
  
  //
  // ECCFill methods.
  //
  uint8_t& Data(size_t position);
  bool&    Valid(size_t position);
  
  size_t  mCurrentOffset;
  uint8_t mData[kBlocks][kBlockSize];
  bool    mValid[kBlocks][kBlockSize];
};

static bool
c1_test(const ECC_Test *test_vector)
{
  ECC_C1 vp;
  ECC_C1::Status stati[2];
  bool comparisons[2];

  BlockPair input(test_vector->input, test_vector->erasures);

  BlockPair *output = NULL;
  if (test_vector->answer[0] != NULL) {
    //
    // Caller has a specific expected result. Decode it.
    //
    output = new BlockPair(test_vector->answer, NULL);
  }

  //
  // Test both the even and odd vectors within this block
  // pair.
  //
  for (size_t i = 0; i < 2; i++) {
    //
    // Tell the block pair which interleave set we'll be pulling from.
    //
    input.FillFrom(i);
    
    //
    // Have the C1 error corrector pull in its data.
    //
    vp.Fill(input);
    
    //
    // Have the C1 error corrector do its thing.
    //
    stati[i] = vp.Correct();

    //
    // Dump the results back to the pair.
    //
    vp.Dump(input);
  }
  
  //
  // Check results.
  //
  bool ok = true;

  for (size_t i = 0; i < 2; i++) {
    ok = ok && stati[i] == test_vector->results[i];
  }

  if (output != NULL) {
    for (size_t i = 0; i < input.kBlocks; i++) {
       ok = ok && memcmp(
        input.mData[i],
        output->mData[i],
        sizeof(input.kBlockSize)
     ) == 0;
    }
    delete output;
  }

  return ok;
}

static bool hex_decode(const char *s, uint8_t *r, size_t n);

BlockPair::BlockPair(const char * const v[2], const char * const e[2])
  : mCurrentOffset(0)
{
  hex_decode(v[0], mData[0], kBlockSize);
  hex_decode(v[1], mData[1], kBlockSize);

  for (size_t i = 0; i < 2; i++) {
    if (e == NULL || e[i] == NULL) {
      for (size_t j = 0; j < kBlockSize; j++) {
        mValid[i][j] = true;
      }
    } else {
      uint8_t erasures[32];
      hex_decode(e[i], erasures, kBlockSize);
      for (size_t j = 0; j < kBlockSize; j++) {
        mValid[i][j] = (erasures[j] == 0);
      }
    }
  }
}

//
// Control whether the even or odd set gets pulled from this vector.
//
void
BlockPair::FillFrom(size_t offset)
{
  mCurrentOffset = offset;
}

//
// These methods allow the ECC_C1 vector to get its data from the
// right place within this test vector.
//
uint8_t&
BlockPair::Data(size_t position)
{
  return mData[position / 16][(position % 16) * 2 + mCurrentOffset];
}

bool&
BlockPair::Valid(size_t position)
{
  return mValid[position / 16][(position % 16) * 2 + mCurrentOffset];
}

static bool
hex_decode(const char *s, uint8_t *r, size_t n)
{
  char hex_byte[3];
  char *endp;

  hex_byte[2] = 0;

  for (size_t i = 0; i < n; i++) {
    hex_byte[0] = s[i*2];
    hex_byte[1] = s[i*2+1];
    r[i] = strtoul(hex_byte, &endp, 16);
    if (*endp != 0)
      return false;
  }

  return true;
}

static const char *
ecc_status(ECC_C1::Status result)
{
  switch (result) {
  case ECC_C1::NO_ERRORS:
    return "NO_ERRORS";
  case ECC_C1::CORRECTED:
    return "CORRECTED";
  case ECC_C1::UNCORRECTABLE:
    return "UNCORRECTABLE";
  default:
    return "?";
  }
}
