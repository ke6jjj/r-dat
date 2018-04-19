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

#ifndef RDAT_ECC_FILL_H
#define RDAT_ECC_FILL_H

#include <stdint.h>
#include <stddef.h>

//
// This class is an interface definition for all classes that encapsulate the
// different ways that the ECC C1, C2 (and in DDS, C3) error correction schemes
// fill their codeword vectors form their various source materials.
//
// By having this interface we remove the need for the ECC correctors to know
// where their data comes from and how to obtain it. The source of the data
// simply instantiates a class that supports this interface and hands that
// instance to the ECC object to query at its leisure.
//
class ECCFill {
  //
  // As an interface, this class should not be constructed directly.
  //
protected:
  ECCFill() { };
  
public:
  virtual ~ECCFill() {};
  
  //
  // Obtain the data for the selected byte position in the codeword.
  // (Returns a reference so that the caller can also overwrite said byte).
  //
  virtual uint8_t& Data(size_t position) = 0;
  
  //
  // Obtain the validity for the selected byte position in the codeword.
  // Validity in this sense represents whether or not the byte at this
  // position represents a known quantity or a known erasure. Some codes
  // use this information to enhance error correction.
  //
  virtual bool& Valid(size_t position) = 0;
};

#endif
