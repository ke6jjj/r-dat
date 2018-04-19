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

#include <stdint.h>
#include <string.h>
#include "AudioFrameReceiver.h"
#include "BCDDecode.h"
#include "TimeCode.h"
#include "XDR.h"

//
// Microsoft RIFF/WAV structures.
//

// typedef struct {
//   ID             chunkID;
//   long           chunkSize;
// 
//   short          wFormatTag;
//   unsigned short wChannels;
//   unsigned long  dwSamplesPerSec;
//   unsigned long  dwAvgBytesPerSec;
//   unsigned short wBlockAlign;
//   unsigned short wBitsPerSample;
// } FormatChunk;
static const size_t kWAVFormatChunkHeaderSize = 4+4+2+2+4+4+2+2;
static const size_t kWAVHeaderSize = 12;
static const size_t kWAVDataChunkHeaderSize = 4+4;

static uint64_t SecondsSince1900(int year, int mon, int day, int hour,
                                 int min, int sec);

AudioFrameReceiver::AudioFrameReceiver()
  : mFile(NULL), mHaveLastDateTime(false), mHaveDateTimeSync(false),
    mHaveLastChangeFrame(false), mHaveLastAbsoluteFrameNumber(false),
    mNextSessionFrameNumber(0)
{
}

AudioFrameReceiver::~AudioFrameReceiver()
{
  if (mFile != NULL)
    fclose(mFile);
}

//
// Is the given track pair a full frame?
// (This is called by the underlying tracking system to help synchronize
// the A/B framing system).
//
bool
AudioFrameReceiver::IsFrame(const Track& A, const Track& B)
{
  //
  // We have a previous track. Are these two a pair or are they
  // separate?
  //
  const uint8_t *A_abstime;
  const uint8_t *B_abstime;
  bool A_good;
  bool B_good;
      
  //
  // Examine their absolute times. (Subcode 2).
  //
  A_good = A.GetSubcode(2, &A_abstime);
  B_good = B.GetSubcode(2, &B_abstime);

  //
  // These two tracks are look like a pair if they have the same, measurable
  // absolute time.
  //
  bool timeGood = A_good && B_good && (memcmp(A_abstime, B_abstime, 7) == 0);
  
  //
  // Finally, just make sure that neither track is in the wrong spot.
  //
  printf("Pair times good: %d\n", timeGood);
  printf("Pairs          : %d %d\n", A.GetHead(), B.GetHead());

  return timeGood
         && A.GetHead() != Track::HEAD_B
         && B.GetHead() != Track::HEAD_A;
}

//
// Receive a full DAT frame (two tracks, A & B)
//
void
AudioFrameReceiver::ReceiveFrame(const Track& a, const Track& b)
{
  const uint8_t *item;
  uint32_t absoluteFrame = 0;

  printf("\n");

  //
  // Examine the Absolute Time subcode.
  //
  if (a.GetSubcode(2, &item)) {
    TimeCode time(item);
    absoluteFrame = time.AbsoluteFrame();
    printf("Absolute time: %02dh-%02dm-%02ds-%02df (%d)\n",
      time.Hour(),
      time.Minute(),
      time.Second(),
      time.Frame(),
      absoluteFrame
    );
    uint16_t program = time.Program();
    switch (program) {
    case TimeCode::PROGRAM_NOT_VALID:
      break;
    case TimeCode::PROGRAM_LEAD_IN:
    case TimeCode::PROGRAM_LEAD_OUT:
      printf("Program ID   : (%s)\n",
             program == TimeCode::PROGRAM_LEAD_IN ? "LEAD IN" : "LEAD_OUT");
      break;
    default:
      printf("Program ID   : %03d\n", program);
      break;
    }
    uint8_t index = time.Index();
    switch (index) {
    case TimeCode::INDEX_NOT_VALID:
      break;
    default:
      printf("Index ID     : %03d\n", index);
      break;
    }
  }
  
  //
  // If the absolute frame number is corrupted or the universal "I don't know"
  // value of 100h-100m-100s-100f (12203433), then display a session-psuedo
  // frame.
  //
  if (absoluteFrame == 0 || absoluteFrame == 12203433) {
    TimeCode time(static_cast<uint32_t>(mNextSessionFrameNumber));
    printf("Psuedo   time: %02dh-%02dm-%02ds-%02df (%d)\n",
      time.Hour(),
      time.Minute(),
      time.Second(),
      time.Frame(),
      mNextSessionFrameNumber
    );
    absoluteFrame = mNextSessionFrameNumber;
  }

  uint8_t control_id;
  if (a.GetControlID(control_id) && control_id != 0) {
    printf("Control      :");
    if (control_id & 0x1)
      // TOC ID
      printf(" TOC");
    if (control_id & 0x2)
      // Shortening ID
      printf(" SKIP");
    if (control_id & 0x4)
      // Start ID
      printf(" START");
    if (control_id & 0x8)
      printf(" PRIORITY");
    printf("\n");
  }
  
  //
  // Examine Program Time subcode.
  //
  if (a.GetSubcode(1, &item)) {
    TimeCode time(item);
    printf("Program time : %02dh-%02dm-%02ds-%02df\n",
      time.Hour(),
      time.Minute(),
      time.Second(),
      time.Frame()
    );
  }
    
  //
  // Examine Running Time / Pro R time.
  //
  if (a.GetSubcode(3, &item)) {
    TimeCode time(item);
    if (item[0] & 0x4) {
      //
      // Running Time.
      //
      printf("Running time : %02dh-%02dm-%02ds-%02df\n",
        time.Hour(),
        time.Minute(),
        time.Second(),
        time.Frame()
      );
    } else {
      //
      // Pro R Time
      //
      uint8_t sid = (item[0] & 3);
      uint8_t freq = (item[1] & 0xC0) >> 6;
      uint8_t xrate = (item[1] & 0x38) >> 3;
      uint8_t msb = (item[1] & 0x07);
      uint8_t lsb = item[2];
      const char *code_type;
      switch (sid) {
      case 0:
        code_type = "IEC/SMPTE";
        break;
      case 1:
        code_type = "Pro DIO; sample address";
        break;
      case 2:
        code_type = "Pro DIO; Time-of-day";
        break;
      case 3:
        code_type = "Reserved-3";
        break;
      default:
        code_type = "?";
        break;
      }
      const char *freq_str;
      switch (freq) {
      case 0:
        freq_str = "48 kHz";
        break;
      case 1:
        freq_str = "44.1 kHz";
        break;
      case 2:
        freq_str = "32 kHz";
        break;
      case 3:
        freq_str = "Reserved-3";
        break;
      default:
        freq_str = "?";
        break;
      }
      const char *smpte_xrate;
      switch (xrate) {
      case 0:
        smpte_xrate = "30 Hz";
        break;
      case 1:
        smpte_xrate = "29.97 Hz NDF";
        break;
      case 2:
        smpte_xrate = "29.97 Hz DF";
        break;
      case 3:
        smpte_xrate = "25 Hz";
        break;
      case 4:
        smpte_xrate = "24 Hz";
        break;
      case 5:
        smpte_xrate = "Reserved-5";
        break;
      case 6:
        smpte_xrate = "Reserved-6";
        break;
      case 7:
        smpte_xrate = "Reserved-7";
        break;
      default:
        smpte_xrate = "?";
        break;
      }
      printf("Pro R Time   : %02dh-%02dm-%02ds-%02df (%s-%s-%s)\n",
        time.Hour(),
        time.Minute(),
        time.Second(),
        time.Frame(),
        code_type,
        freq_str,
        smpte_xrate
      );
    }
  }

  //
  // Examine Table of Contents subcode.
  //
  if (a.GetSubcode(4, &item)) {
    printf("Table of Cont:\n");
    TimeCode time(item);
    uint16_t program = time.Program();
    switch (program) {
    case TimeCode::PROGRAM_NOT_VALID:
      break;
    case TimeCode::PROGRAM_LEAD_IN:
    case TimeCode::PROGRAM_LEAD_OUT:
      printf("  Program ID : (%s)\n",
             program == TimeCode::PROGRAM_LEAD_IN ? "LEAD IN" : "LEAD_OUT");
      break;
    default:
      printf("  Program ID : %03d\n", program);
      break;
    }
    uint8_t index = time.Index();
    switch (index) {
    case TimeCode::INDEX_NOT_VALID:
      break;
    default:
      printf("  Index ID   : %03d\n", index);
      break;
    }
    printf("  Time       : %02dh-%02dm-%02ds-%02df\n",
      time.Hour(),
      time.Minute(),
      time.Second(),
      time.Frame()
    );
  }

  //
  // Examine the Date & Time subcode.
  //
  if (a.GetSubcode(5, &item))
    HandleDateTime(item, absoluteFrame);
  else
    HandleDateTime(NULL, absoluteFrame);

  //
  // Examine Catalog Number subcode.
  //
  if (a.GetSubcode(6, &item)) {
  }

  //
  // Examine ISRC subcode.
  //
  if (a.GetSubcode(7, &item)) {
    printf("ISRC         : (not yet)\n");
  }

  //
  // Examine Pro Binary subcode.
  //
  if (a.GetSubcode(8, &item)) {
    printf("Pro Binary   : (not yet)\n");
  }

  //
  // Print the sub-code signature.
  //
  const Track::SubcodeSignatureArray& signature = a.SubcodeSignature();
  printf("Subcode packs:");
  for (size_t i = 0; i < 7; i++)
    printf(" %-2d", signature[i]);
  putchar('\n');

  //
  // Fetch and demultiplex all the data from the track pair.
  //
  mFrame.FillFromTrackPair(a, b);
  const DATFrame::DataArray& data = mFrame.Data();

  //
  // Print out error statistics for the frame.
  //
  unsigned int c1_errors = mFrame.C1Errors();
  unsigned int c1_uncorrectable = mFrame.C1UncorrectableErrors();
  unsigned int c1_corrected = c1_errors - c1_uncorrectable;
  unsigned int c2_uncorrectable = mFrame.C2UncorrectableErrors();
  unsigned int c2_corrected = c1_uncorrectable - c2_uncorrectable;

  printf("Errors  C1/C2: %d/%d",
    c1_corrected,
    c2_corrected
  );

  if (c2_uncorrectable > 0)
    printf(" %d UNCORRECTED\n", c2_uncorrectable);
  else
    printf(" (all corrected)\n");
  

  printf("Samples      : L    R\n");
  for (size_t i = 0; i < 8; i++) {
    printf("               %02x%02x %02x%02x\n",
      data[i][1], data[i][0],
      data[i][3], data[i][2]
    );
  }

  //
  // Ensure that everything printed so far is immediately visible.
  //
  fflush(stdout);

  //
  // Dump samples to file, if asked.
  //
  if (mFile != NULL) {
    for (size_t i = 0; i < DATFrame::kUserDataRows; i++) {
      fwrite(data[i], DATFrame::kBytesPerRow, 1, mFile);
    }
    mFramesWritten += DATFrame::kUserDataRows;
  }

  mHaveLastAbsoluteFrameNumber = true;
  mLastAbsoluteFrameNumber = absoluteFrame;
  mNextSessionFrameNumber++;
}

void
AudioFrameReceiver::HandleDateTime(const uint8_t *item, uint32_t absoluteFrame)
{
  uint8_t dow, year, mon, day, hour, min, sec;
  int likelyYear;
  bool timeIsValid = false;
  bool droppedSync = false;
  uint64_t abs_seconds;

  if (item != NULL) {
    //
    // Date and time.
    //
    dow = item[0] & 0xf;
    year = BCDDecode(item[1]);
    mon = BCDDecode(item[2]);
    day = BCDDecode(item[3]);
    hour = BCDDecode(item[4]);
    min = BCDDecode(item[5]);
    sec = BCDDecode(item[6]);

    if (year < 80)
      likelyYear = year + 2000;
    else
      likelyYear = year + 1900;
    
    //
    // Check if anything seems funny (bad BCD decodes)
    //
    if (mon != 100 && day != 100 && hour != 100 && min != 100 && sec != 100) {
      //
      // Determine the absolute date time in seconds since 1900.
      //
      abs_seconds = SecondsSince1900(likelyYear, mon, day, hour, min, sec);
      timeIsValid = true;
    }
  } else {
    dow = year = mon = day = hour = min = sec = 0;
  }

  if (mHaveDateTimeSync) {
    //
    // Syncronization is in effect. Make sure that everything is still ok.
    //
    if (timeIsValid && absoluteFrame == mLastAbsoluteFrameNumber + 1) {
      //
      // Everything seems ok so far. We've got a new timestamp and there
      // hasn't been a discontiuity in the frame stream.
      //
      // Advance our current clock by 30ms.
      //
      mCurrentDateTimeMilliseconds += 30;
      if (mCurrentDateTimeMilliseconds >= 1000) {
        mCurrentDateTimeMilliseconds %= 1000;
        mCurrentDateTimeSeconds += 1;
      }

      //
      // Does the second hand of our clock still match the transmitted
      // clock?
      //
      if (mCurrentDateTimeSeconds != abs_seconds) {
        //
        // No. We're no longer in sync.
        //
        mHaveDateTimeSync = false;
        droppedSync = true;
      }
    } else {
      //
      // We lost date and time sub-code or there was a jump in absolute
      // time.
      //
      mHaveDateTimeSync = false;
      droppedSync = true;
    }
  }

  //
  // Regardless of the current sync state, keep the closest-to-the-second
  // engine running, possibly establishing sync when it detects a long
  // (34-frame) second.
  //
  if (mHaveLastDateTime && timeIsValid) {
    //
    // We have a previous date and time stamp. Has the time changed
    // since then?
    //
    if (mLastDateTimeSeconds != abs_seconds) {
      //
      // The time has changed. Is it a discontinuity or is it a simple
      // advancement?
      //
      if (abs_seconds == mLastDateTimeSeconds + 1) {
        //
        // Simple advancement. This is a chance to achive a frame
        // lock and/or confirm that the lock is still in place.
        // In order to do so, however, we have to observe two successive
        // increments. Do we have a previous increment observed?
        //           
        if (mHaveLastChangeFrame) {
          //
          // DAT provides 33 1/3 frames per second. Since timestamps
          // can only be placed at the begining of frames, timestamps
          // should advance in the following pattern:
          //
          // 1. 34 frames (No millisecond offset)
          // 2. 33 frames (20 millisecond offset)
          // 3. 33 frames (10 millisecond offset)
          //
          // The timestamp that started with the 34-frame sequence is the
          // one that is closest to the real start of the second.
          //
          uint32_t frames_per_second = absoluteFrame - mLastChangeFrame;
          if (frames_per_second == 34) {
            //
            // We've just received received the frame that ends pattern
            // 1 and starts pattern 2. This means this frame marks the
            // start of pattern 2.
            //
            // If we aren't time synchronized then we can be now.
            //
            if (!mHaveDateTimeSync) {
              //
              // We don't currently have a frame lock. Let's mark
              // that we do and that the last second change occured as
              // pattern 1.
              //
              mHaveDateTimeSync = true;
              mCurrentDateTimeMilliseconds = 20;
              mCurrentDateTimeSeconds = abs_seconds;
            }
          }
        }

        //
        // Mark that this is the new last change frame.
        //
        mHaveLastChangeFrame = true;
        mLastChangeFrame = absoluteFrame;
      } else {
        //
        // There's been a bad discontinuity. Mark that we no longer
        // have a good roll-over frame.
        //
        mHaveLastChangeFrame = false;
      }
    }
  }
    
  //
  // Record that we now have a date and time.
  //
  mHaveLastDateTime = timeIsValid;
  mLastDateTimeSeconds = abs_seconds;

  if (droppedSync && mHaveDateTimeSync)
    printf("Date     time: ------- SYNC DROPPED AND REESTABLISHED --------\n");
  else if (droppedSync)
    printf("Date     time: ------- SYNC DROPPED --------------------------\n");

  if (mHaveDateTimeSync) {
    printf("Date     time: %02d %04d-%02d-%02d %02d:%02d:%02d.%03d (SYNCED)\n",
      dow,
      likelyYear,
      mon,
      day,
      hour,
      min,
      sec,
      mCurrentDateTimeMilliseconds
    );
  } else if (timeIsValid) {
    printf("Date     time: %02d %04d-%02d-%02d %02d:%02d:%02d\n",
      dow,
      likelyYear,
      mon,
      day,
      hour,
      min,
      sec
    );
  }
}

void
AudioFrameReceiver::Stop()
{
  //
  // Processing has stopped. If we were dumping audio to a file, go back
  // and fill in the file headers.
  //
  if (mFile != NULL) {
    //
    // Write WAV header.
    //
    XDR hdr(kWAVHeaderSize);
    hdr.AddString("RIFF", 4);
    hdr.AddU32(kWAVFormatChunkHeaderSize+
               kWAVDataChunkHeaderSize+
               mFramesWritten * 4+
               4);
    hdr.AddString("WAVE", 4);
    
    fseek(mFile, SEEK_SET, 0);
    fwrite(hdr.Data(), hdr.Size(), 1, mFile);

    //
    // Write format chunk.
    //
    XDR xdr(kWAVFormatChunkHeaderSize);
    
    // ID             chunkID;
    xdr.AddString("fmt ", 4);
    // long           chunkSize;
    xdr.AddU32(kWAVFormatChunkHeaderSize-8);
    
    // short          wFormatTag;
    xdr.AddI16(1); // PCM data
    // unsigned short wChannels;
    xdr.AddU16(2);
    // unsigned long  dwSamplesPerSec;
    xdr.AddU32(48000);
    // unsigned long  dwAvgBytesPerSec;
    xdr.AddU32(48000 * 4); // 4 bytes per frame (2 16-bit samples)
    // unsigned short wBlockAlign;
    xdr.AddU16(4); // 4 bytes per frame (2 16-bit samples)
    // unsigned short wBitsPerSample;
    xdr.AddU16(16);

    fwrite(xdr.Data(), xdr.Size(), 1, mFile);

    //
    // Write data chunk.
    //
    XDR data(kWAVDataChunkHeaderSize);

    data.AddString("data", 4);
    data.AddU32(mFramesWritten * 4);

    fwrite(data.Data(), data.Size(), 1, mFile);

    //
    // Close file.
    //
    fclose(mFile);
    mFile = NULL;
  }
}

bool
AudioFrameReceiver::SetDumpFile(const char *path)
{
  mFile = fopen(path, "wb");
  mFramesWritten = 0;
  
  if (mFile != NULL) {
    uint8_t header_space[kWAVHeaderSize + kWAVFormatChunkHeaderSize +
                         kWAVDataChunkHeaderSize];
    memset(header_space, 0, sizeof(header_space));
    fwrite(header_space, sizeof(header_space), 1, mFile);
  }
  
  return mFile != NULL;
}

static uint64_t
SecondsSince1900(int year, int mon, int day, int hour, int min, int sec)
{
  static const int kMonthDoyNormal[12] = {
    0, 31, 59, 91, 121, 152, 182, 213, 244, 274, 305, 335    
  };
  static const int kMonthDoyLeap[12] = {
    0, 31, 60, 92, 122, 153, 183, 214, 245, 275, 306, 336    
  };
  
  uint64_t seconds = ((uint64_t)year) * 86400 * 365;
  seconds += sec;
  seconds += min * 60;
  seconds += hour * 3600;
  seconds += (day - 1) * 86400;
  if ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0)) {
    seconds += kMonthDoyLeap[mon-1] * 86400;
  } else {
    seconds += kMonthDoyNormal[mon-1] * 86400;
  }
  
  //
  // Finally, account for number of leap days that have been inserted
  // since January 1, 1900.
  //
  if (year > 1904) {
    int leaps = ((year - 1)/ 4) - 475;
    leaps -= ((year - 1) / 100) - 19;
    leaps += ((year - 1) / 400) - 4;
    seconds += leaps * 86400;
  }
  
  return seconds;
}
