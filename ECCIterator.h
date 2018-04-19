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

#ifndef RDAT_ECC_ITERATOR_H
#define RDAT_ECC_ITERATOR_H

#include "ECCFill.h"

//
// This abstract class encapsulates the vector fill and iteration behavior
// for all DAT/DDS ECC schemes.
//
// Vector filling and dumping is encapsulated in this class's parent class,
// the ECCFill interface. This class extends that behavior by further
// encapsulating the logic used to determine how many vectors are in the
// data source. We accomplish this through a simple iterator interface.
//
// A user of this class simply instantiates the class with the correct
// data source object and then repeatedly:
//
// 1. Asks this class to fill itself with the next vector from the source.
// 2. Asks the relevant ECC class to evaluate that vector.
// 3. Asks this class to dump its results back to the source.
// 4. Asks this class to advance to the next vector in the source.
//
class ECCIterator : public ECCFill {
  //
  // As an interface, this class should not be constructed directly.
  //
protected:
  ECCIterator() {};

public:
  virtual ~ECCIterator() {};

  //
  // Advance to the next vector in the source, if one is available.
  // Returns true if the advancement results in the next vector, false if
  // there are no more vectors to process.
  //
  // See also: the End() method.
  //
  virtual bool Next() = 0;
  
  //
  // Returns false if there are further vectors to be processed, true
  // if all vectors have been processed.
  //
  virtual bool End() const = 0;
  
  ///////////////////////////////////////////////////////////////////////////
  // Methods inherited from ECCFill

  virtual uint8_t& Data(size_t position) = 0;
  virtual bool& Valid(size_t position) = 0;

  // End Methods inherited from ECCFill
  ///////////////////////////////////////////////////////////////////////////
};
 
#endif
