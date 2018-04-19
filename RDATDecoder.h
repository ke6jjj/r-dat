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

#include <sys/types.h>
#include "SymbolDecoder.h"

class RDATDecoder
{
public:
	RDATDecoder(float sampleRate);
	virtual ~RDATDecoder();
	
	void SetSymbolDecoder(SymbolDecoder *b);
	void Process(float *samples, size_t count);
	void Stop();

	void SetClockRatioThreshold(float threshhold);
	void SetClockAlpha(float alpha);
	
private:
	bool ClockDetect(float sample);
	void EvaluateClock(void);

	//
	// The upstream receive to whom we should send decoded bits.
	//
	SymbolDecoder *mDecoder;
	
	//
	// Clock detection variables.
	//
	float *mSyncWindow;
	int    mSyncWindowSize;
	int    mSyncWindowCurPos;
	int    mSyncWindowSyncPos;
	int    mSyncWindowEvalPos;
	int    mSyncWindowNextEvalPos;
	
	bool   mClockDetected;
	
	//
	// Minimum ratio between peak level and valley level that
	// is considered a good clock lock.
	//
	float mClockRatioThreshold;
	
	//
	// Alpha to use in clock window filter.
	//
	float mClockAlpha;

	//
	// The sign of the last sample.
	//
	bool mLastSign;

	//
	// Sample integrator.
	//
	float mIntegrator;

	//
	// The current "track-in-progress" state.
	//
	bool mTrackInProgress;

	//
	// If there's a track in progress, wait this many samples before
	// declaring it over.
	//
	const size_t mTrackDuration;

	//
	// How many samples are left in the current track duration.
	//
	size_t mTrackSampleCount;
};
