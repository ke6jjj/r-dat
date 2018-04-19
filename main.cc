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
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "RDATDecoder.h"
#include "NRZISyncDeframer.h"
#include "DATWordReceiver.h"
#include "DATTrackFramer.h"
#include "AudioFrameReceiver.h"
#include "DDSFrameReceiver.h"
#include "File.h"

enum { SAMPLES_PER_READ = 1000 };

static void usage(const char *prog);
static void sigint_handler(int);

static volatile bool running;

int
main(int argc, char *argv[])
{
  float buf[SAMPLES_PER_READ];
  size_t nread;
  bool do_raw = false;
  bool do_dat = false;
  bool do_dds = false;
  bool do_file = false;
  bool do_output = false;
  bool do_dds_session = false;
  enum { DECODE_RAW, DECODE_DAT, DECODE_DDS } decode_mode = DECODE_DAT;
  int c;
  const char *filename, *outfile;
  unsigned int dds_session;

  while ((c = getopt(argc, argv, "hdraf:o:s:")) != -1) {
    switch (c) {
    default:
    case 'h':
      usage(argv[0]);
      break;
    case 'd':
      do_dds = true;
      break;
    case 'r':
      do_raw = true;
      break;
    case 'a':
      do_dat = true;
      break;
    case 'f':
      do_file = true;
      filename = optarg;
      break;
    case 'o':
      do_output = true;
      outfile = optarg;
      break;
    case 's':
      do_dds_session = true;
      dds_session = strtoul(optarg, NULL, 0);
      break;
    }
  }

  //
  // Only one decode mode allowed.
  //
  if ((do_raw && (do_dds || do_dat)) || (do_dat && do_dds)) {
    usage(argv[0]);
  }

  //
  // Can only do output if DAT or DDS is selected.
  //
  if (do_output && !do_dat && !do_dds) {
    fprintf(stderr, "Can't dump result unless doing DAT audio or DDS.\n");
    usage(argv[0]);
  }
  
  //
  // Can only do DDS session if DDS is selected.
  //
  if (do_dds_session && !do_dds) {
    fprintf(stderr, "DDS session number is only valid for DDS.\n");
    usage(argv[0]);
  }

  //
  // Default to DAT if no choice specified.
  //
  if (!do_raw && !do_dds && !do_dat)
    do_raw = true;

  if (do_raw)
    decode_mode = DECODE_RAW;
  else if (do_dat)
    decode_mode = DECODE_DAT;
  else if (do_dds)
    decode_mode = DECODE_DDS;

  File in;

  if (do_file) {
    if (!in.Open(filename, sizeof(float)))  {
      fprintf(stderr, "Can't open file '%s'.\n", filename);
      exit(1);
    }
  } else {
    in.Open(STDIN_FILENO, sizeof(float));
  }

  RDATDecoder      *decoder;
  NRZISyncDeframer *deframer;
  DATWordReceiver  *blocker;
  DATTrackFramer   *tracker = NULL;
  DATFrameReceiver *streamer = NULL;

  switch (decode_mode) {
  case DECODE_DAT:
    {
    AudioFrameReceiver *audio = new AudioFrameReceiver();
    if (do_output) {
      if (!audio->SetDumpFile(outfile)) {
        fprintf(stderr, "Can't dump to output file '%s'.\n", outfile);
        exit(1);
      }
    }
    // Cast receiver to generic base class for rest of code.
    streamer = audio;
    }
    break;
  case DECODE_DDS:
    {
    DDSFrameReceiver *dds = new DDSFrameReceiver();
    if (do_output) {
      dds->DumpToDirectory(outfile);
    }
    if (do_dds_session) {
      dds->DumpSession(dds_session);
    }
    streamer = dds;
    }
    break;
  default:
    break;
  }

  if (decode_mode == DECODE_RAW) {
    blocker = new DATWordReceiver(NULL, true);
  } else {
    tracker = new DATTrackFramer(*streamer);
    blocker = new DATWordReceiver(tracker, false);
  }
    
  deframer = new NRZISyncDeframer(blocker);
  decoder = new RDATDecoder(9408000.0 * 8);
  decoder->SetSymbolDecoder(deframer);

  running = true;

  //
  // Install the sigint handler so that the user can stop the
  // processing safely.
  //
  struct sigaction int_handler = { .sa_handler = sigint_handler };
  ::sigaction(SIGINT, &int_handler, NULL);

  while (running) {
    nread = in.Read(buf, SAMPLES_PER_READ);
    if (nread == 0)
      break;
    decoder->Process(buf, nread);
  }

  decoder->Stop();

  in.Close();
  
  delete decoder;
  delete deframer;
  delete blocker;
  delete streamer;

  return 0;
}

static void
usage(const char *prog)
{
  fprintf(stderr,
    "usage: %s [-r|-d|-a] [-s <number>] [-f <filename>] [-o <path>]\n"
    "Decode DAT/DDS samples taken from an R-DAT RF head. Input must be in\n"
    "IEEE-float format, in native-endian order, and sampled at 75.264MHz.\n"
    " -a - Use DAT decode (Default)\n"
    " -d - Use DDS decoder.\n"
    " -r - Dump raw packets; don't interpret as DAT nor DDS.\n"
    " -o - DAT mode: Write raw audio to file <path>.\n"
    "      DDS mode: Dump basic groups to directory <path>.\n"
    " -f - Read data from filename. (Default is stdin).\n"
    " -s - Dump DDS session <number> (DDS only)\n",
    prog
  );
  exit(1);
}

static void
sigint_handler(int signal_received)
{
  running = false;
}
