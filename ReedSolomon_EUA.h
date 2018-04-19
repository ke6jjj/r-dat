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

#ifndef RDAT_REEDSOLOMON_EUA_H
#define RDAT_REEDSOLOMON_EUA_H

#include <stddef.h>
#include "ECC_GF28.h"

//
// These are helper polynomials. Skip down further to find the Reed-Solomon
// algorithms.
//
template <size_t n>
void
poly_multiply_by_x(const uint8_t (&in)[n], uint8_t (&out)[n])
{
  //
  // Perform this in a manner that will be safe even if in and out
  // are the same array.
  //
  for (size_t i = n - 1; i > 0; i--)
    out[i] = in[i-1];
  out[0] = 0;
};

template <size_t n>
void
poly_multiply_scalar(const uint8_t (&in)[n], const uint8_t s, uint8_t (&out)[n])
{
  for (size_t i = 0; i < n; i++)
    out[i] = ECC_GF28_multiply(in[i], s);
};

template <size_t n>
void
poly_add(const uint8_t (&a)[n], const uint8_t (&b)[n], uint8_t (&out)[n])
{
  for (size_t i = 0; i < n; i++)
    out[i] = a[i] ^ b[i];
};

template <size_t n>
void
poly_copy(const uint8_t (&in)[n], uint8_t (&out)[n])
{
  for (size_t i = 0; i < n; i++)
    out[i] = in[i];
};

template <size_t n>
uint8_t
poly_evaluate(const uint8_t (&in)[n], uint8_t x)
{
  uint8_t y = x;
  uint8_t r = in[0];
  
  for (size_t i = 1; i < n; i++) {
    r ^= ECC_GF28_multiply(in[i], y);
    y = ECC_GF28_multiply(y, x);
  }
  
  return r;
};

//
// This algorithm template comes from a paper titled "Modified Euclidean
// Algorithms for Decoding Reed-Solomon Codes" by Dilip V. Sarwate and
// Zhiyuan Yan, 2009.
//
// It is especially useful because it can handle the extra "erasure"
// information that is available during R-DAT decoding. The ten-to-eight
// decoding process generates one level of erasure indications when it
// detects and invalid 10-bit symbol.
//
// Likewise, the C2 error correction process works best when it is given
// vectors with bytes that are either entirely known to be correct or
// entirely known to be faulty. This can be done because the C1 correction
// layer that comes before it can give us that assurance. When run in this
// assurance mode, the C2 corrector can fix 6 symbols per vector instead of
// just three.
//
// Given an already-computed syndrome vector, and an array of known error
// locations, this algorithm computes an error-locator polynomial, a
// error-magnitude polynomial, and an indication as to whether or not
// the vector from which the syndrome was obtained can be corrected.
//
// kT - The value "t" for the code. This is the number of symbols that
//      can be corrected by the code. A code capable of "t" corrections
//      requires 2*t parity symbols and, consequently, has 2*t syndrome
//      values.
//
//      Conversely, if the code has x syndrome values, you must instantiate
//      this function template with the parameter x/2.
//
// syndrome - A vector containing the computed syndrome to be corrected.
//      The first element of this vector must be the lowest-order term
//      of the syndrome.
//
// erasures - A vector containing the known erasure locations, in terms
//      of the erasure's position in the code word's polynomial. 
//
//      For example, if the caller knows that an erasure has occured in
//      the byte that represents the lowest-order coefficient in the
//      codeword polynomial, the caller must encode that erasure
//      as the value 0. Likewise, an erasure in the highest-term
//      coefficient is represented as the value n-1, where n is the
//      code word size.
//
//      The caller can pass no more than 2*kT error locations as the
//      Reed-Solomon decoder can solve no more than that many erasures.
//
// erasure_count - The number of erasure locations that are set in
//      the erasures vector.
//
//      The caller can pass no more than 2*kT error locations as the
//      Reed-Solomon decoder can solve no more than that many erasures.
//
// r_sigma - A vector to receive the computed error-locator polynomial.
//      The error-locator polynomial can reach a degree one higher than the
//      syndrome vector size, so the caller must provide enough room for this
//      additional term.
//
// r_omega - A vector to receive the computed error-magnitude polynomial.
//      Like the error-locator polynomial, the error-magnitude polynomial
//      can reach a degree one higher than the syndrome vector, and as such,
//      the caller must provide room for the additional term.
//
// RETURN VALUE
//
// Returns true if the code can possibly be corrected, false if it cannot
// be corrected. Codes can only absolutely be corrected if the return
// value is true and the caller can find a root to the error-locator
// polynomial within the code word size.
//
template <size_t kT>
bool
RS_Solve(const uint8_t syndrome[2*kT], const uint8_t erasures[2*kT],
  size_t erasure_count, uint8_t (&r_sigma)[2*kT+1], uint8_t (&r_omega)[2*kT])
{
  const size_t kTwoT = kT * 2;
  uint8_t U[kTwoT+1];
  uint8_t V[kTwoT+1];
  uint8_t W[kTwoT+1];
  uint8_t X[kTwoT+1];
  
  for (size_t i = 0; i < kTwoT; i++) {
    //
    // Clear out the parts of the big polynomials that will not
    // be otherwise initialized.
    //
    X[i+1] = 0;    
    W[i+1] = 0;
    U[i] = 0;

    //
    // Copy in the syndrome value.
    //
    V[i] = syndrome[i];
  }
  
  U[kTwoT] = 1;
  V[kTwoT] = 0;
  X[0] = 1;
  W[0] = 0;
  int D = -1;
  size_t p = 0;
  
  for (size_t i = 0; i < kTwoT; i++) {
    //
    // During the first phase of this algorithm, we incorporate erasures.
    // Determine if we're still in erasure-processing phase.
    //
    bool first = (p < erasure_count);
    
    //
    // Next we determine if the Euclidean algorithm requires us to swap
    // the parameters being processed.
    //
    bool swap = !first && (V[kTwoT - 1] != 0) && (D < 0);
    
    //
    // Examine the next element to be processed. In the erasures
    // phase we pull in the next erasure locator. In the error locator
    // phase we pull in the next thingamjig.
    //
    uint8_t G, Z;
    if (first) {
      //
      // Erasures processing. Get next erasure symbol and convert it
      // into a power of alpha.
      //
      G = ECC_GF28_pow_alpha(erasures[p++]);
      Z = 1;
    } else {
      //
      // Done with erasures. Get a thingamjig.
      //
      G = U[kTwoT];
      Z = V[kTwoT-1];
    }
    
    if (swap) {
      D = -D - 1;
    } else if (!first) {
      D = D - 1;
    }
    
    uint8_t v_adjust[kTwoT+1];
    uint8_t x_adjust[kTwoT+1];
    if (first) {
      poly_multiply_scalar<kTwoT+1>(V, Z, v_adjust);
      poly_multiply_scalar<kTwoT+1>(X, Z, x_adjust);
    } else {
      poly_multiply_scalar<kTwoT+1>(U, Z, v_adjust);
      poly_multiply_scalar<kTwoT+1>(W, Z, x_adjust);
    }
    
    //
    // Compute z V(z) and
    //         z X(z)
    //
    uint8_t new_V[kTwoT+1];
    uint8_t new_X[kTwoT+1];

    poly_multiply_by_x<kTwoT+1>(V, new_V);
    poly_multiply_by_x<kTwoT+1>(X, new_X);

    //
    // Compute G z V(z) and
    //         G z X(z)
    //
    poly_multiply_scalar<kTwoT+1>(new_V, G, new_V);
    poly_multiply_scalar<kTwoT+1>(new_X, G, new_X);
    
    poly_add<kTwoT+1>(new_V, v_adjust, new_V);
    poly_add<kTwoT+1>(new_X, x_adjust, new_X);
    
    if (swap) {
      poly_multiply_by_x<kTwoT+1>(V, U);
      poly_multiply_by_x<kTwoT+1>(X, W);
    }
    
    poly_copy<kTwoT+1>(new_V, V);
    poly_copy<kTwoT+1>(new_X, X);
  }

  if (D < 0 && (p == erasure_count)) {
    //
    // Everything should be correctable.
    // Return the location polynomial and the magnitude polynomial.
    //
    poly_copy<kTwoT+1>(X, r_sigma);
    for (size_t i = 0; i < kTwoT; i++)
      r_omega[i] = V[i];
    
    return true;
  }
  
  return false;
};

//
// Document this.
//
template <size_t kT>
uint8_t
RS_GetErrorAtLocation(const uint8_t (&sigma)[2*kT+1],
  const uint8_t (&omega)[2*kT], uint8_t location)
{
  const size_t kTwoT = kT * 2;

  //
  // Evaluate the top part of Forney's formula: send the error locator
  // through omega.
  //
  uint8_t top = poly_evaluate<kTwoT>(omega, location);

  // 
  // To complete the top of Forney's formula we must multiply the previous
  // result by location^(b0). b0 is the power of the lowest power of
  // alpha in the code. It's a variable that is often zero or one, but
  // is up to the particular implementation of the code.
  // 
  // DAT happens to use a b0 of zero. You can see this because the C1
  // and C2 check matricies that DAT uses in the code all begin with rows
  // of "1"'s. In other words, they all begin with rows of alpha^0.
  // Some codes begin with rows of incrementing powers of alpha. Those
  // codes use value of '1' for b0.
  // 
  // Since our code has a b0 value of zero, location^b0 is one.
  // Multiplying by 1 can be skipped. So we skip it.
  // 
  // Otherwise we would write:
  // 
  // top *= location^b0
  //
   
  //
  // Now, evaluate the error position indicator through the derivative
  // of the error locator polynomial.
  //
  // Due to some handy math and that we're working in GF(2), the derivative
  // of the error-locator polynomial is just every other of its terms!
  //
  // Here we're going to use a variable "y" to hold the current power of
  // the error locator value as it goes through the loop.
  //
  uint8_t res = 0; // End result
  uint8_t y = 1;   // location^0
  for (size_t i = 1; i < kTwoT+1; i++) {
    if (i & 1) {
      //
      // This is one of the coefficients we want to evaluate.
      // Multiply it by the current power of location.
      //
      res ^= ECC_GF28_multiply(sigma[i], y);
    }
    //
    // Increase the power of the location.
    //
    y = ECC_GF28_multiply(y, location);
  }
  
  //
  // Like the top portion, the bottom portion must be multiplied by a value.
  // In this case it is just the error position indicator.
  //
  uint8_t bottom = ECC_GF28_multiply(res, location);
  
  //
  // The return value is the top value divided by the bottom value.
  //
  return ECC_GF28_multiply(top, ECC_GF28_invert(bottom));
};

#endif
