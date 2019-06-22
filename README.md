# r-dat
A software suite for recovering and decoding data stored on Digital Audio Tape
(DAT) and Digital Data Storage (DDS) tapes from high-bandwidth digitized
recordings taken directly from the tape head.

R-DAT can:

* Assemble and decode DAT audio as WAV files and meta-data

   When reading a DAT with digital audio, R-DAT produces a WAV file and a
   textual diagnostic output stream, which includes all sub-code blocks and ECC
   error information. If a block can't fully be decoded, it will still produce
   output but will give you insight into those samples which failed error checking.

* Assemble and decode DDS data

   When reading a DAT with DDS (computer data), R-DAT produces binary "basic
   block" files which are the quanta in which data are stored in the DDS format.
   Using the provided Python utility, you can further process the data to re-create
   the files which were stored on the tape. Any badly decoded blocks are marked but
   do not stop you from accessing the healthy areas of the tape, unlike a traditional
   DDS drive.

# Rationale
R-DAT is the first, and perhaps only, Open-source tookit for forensic recovery
of DAT and DDS tapes -- a culturally relevant medium that is quickly going
obsolete and which will one day be unrecoverable without very expensive
remanufacturing of tape heads and hardware.

## Timecode recovery

I originally wrote r-dat for one purpose: to recover the date and time-of-day
signals that were recorded on my personal DAT tapes made in the 1990's
on the then very popular Sony TCD-D7 portable DAT machine, but which was no
longer working. My remaining DAT machine, a Tascam DA-P1, could play back the
tapes but couldn't display the date data. The frustration of knowing that the
data were there but simply not displayable, combined with the knowledge that
my remaining deck and the tapes themselves would one day be unusable, led me
on a several month quest that led to this software.

## Data recovery

After recovering the timestamps from my audio tapes, I then remembered a DDS
(data) back tape I had made in 1998 and had been saving for a future day
where I would have the time to salvage it from a very simple mistake: I had
overwritten the very first portion of the tape with another backup. In the
DDS system this simple act renders all the remaining data in the tape
unaccessible; there simply is no commercially available DDS drive that
will "step over" the end of tape mark and allow the data from the untouched,
previous data session to be retrieved.

After poring over many freely available ECMA descriptions of the DDS format
and carefully extending of my DAT audio decoding framework, I used the same
Tascam DA-P1 audio deck I used in my timecode recovery project, I was finally
recovered this long lost data, 20 years after I thought it was gone forever.

I'm releasing R-DAT so that it might be useful to others and in the hopes that
future digital archivists might be able to use it to recover and preserve
recordings that belong to our common, human heritage.

# Requirements

In its current state, r-dat is just a software decoder. To use it, you must
obtain a high-bandwith recording of the electrical signal from a working
DAT machine, made while the tape in question is being "played". This is not
an easy task.

# Theory of operation / Signal flow

This section shows how an R-DAT signal should be acquired
before feeding it to the software, and then how the software
goes about processing the signal, step by step. In each step,
the file containing the source code most relevant to that
step is shown.

## The Rotary Tape Head

```
         ,------------------,
         | Rotary Tape Head |
         `------------------'
                  |
      [ 9.4 MHz baseband signal ]
                  |
                  v
```

R-DAT signals are encoded on DAT tapes as chains of magnetic field
reversals, generally around 9,408,000 reversals per second. When
picked up by the R-DAT tape head, these reversals appear as
momentary pulses.

## Tape fields and voltages expressed

```
N = North-going field 
S = South-going field

        |
Tape    |-------------------------------------------------------------------
surface |   nNNn sSSs nNNn sSSs nNNn sSSs nNNn sSSs nNNn sSSSSSSSs nNNn
        |-------------------------------------------------------------------
        |
Head    |
style A |   ,,      ,,,,      ,,,,      ,,,,      ,,,,           ,,,,
output  |   ||      |  |      |  |      |  |      |  |           |  |
signal  |---''-,  ,-'  '-,  ,-'  '-,  ,-'  '-,  ,-'  '-,  ,------'  '--
        |      |  |      |  |      |  |      |  |      |  |     
        |      ''''      ''''      ''''      ''''      ''''
Head    |
style B |    ,,        ,,        ,,        ,,        ,,             ,,
output  |   /  \      /  \      /  \      /  \      /  \           /  \
signal  |--'    \    /    \    /    \    /    \    /    \         /
        |        \__/      \__/      \__/      \__/      \_______/
        |
        
                                   --- TIME --->
```

I've found that the literature describing how R-DAT magnetizes the tape and
how that magnetization pattern appears after being picked up by the tape
head can be a bit conflicting. I've seen one style of pickup
illustrated, yet, in practice, I've seen a different style actually being
used. The preceeding illustration shows how magnetic fields are stored on
the tape and how these two styles of pickup will interpret that pattern.

Style "A" is the theoretical pickup style, which is described as a time-differential
probe, interprets the magnetic fields on the tape, while style "B" illustrates
the signal that I've seen in my player. It would more accurately be described as a
integrated version of the differential signal.

Whereas style "A" shows more of a pulse chain, where pulses in voltage occur when
the field is _changing_, style "B" shows more of a square wave signal, where voltage
plateaus appear where the magnetic field is the strongest.

## Small Signal Amplifier

```
                  |
      [ 9.4 MHz baseband signal ]
                  |
                  v
 ,-----------------------------------,
 | Amplifier  (wideband, 20 dB gain) |
 `-----------------------------------'
                  |
      [ 9.4 MHz baseband signal ]
                  |
                  v
```

The signal directly from the R-DAT tape head is generally to
weak to digitally sample reliably with off-the-shelf A/D
samplers. I've found it useful to insert a small signal
amplifier, such as a MiniCircuits ZFL-500+, to bring the
signal cleanly up to a level where it can be sampled by
a typical software-defined radio interface.

## Software-Defined Radio A/D converter

```
                  |
      [ 9.4 MHz baseband signal ]
                  |
                  v
 ,--------------------------------------, 
 | Software-Defined Radio A/D converter |
 `--------------------------------------'
                  |
   [ Digital samples, > 6 bit depth ]
                  |
                  v
```
Once the signal is amplified to an adequate level, it is sampled
by a high-speed A/D converter, such as those available in off-the-shelf
software-defined radio boards, like National Instruments' "URSP".

The A/D converter used must sample the tape head signal at a rate that,
at the theoretically very _least_, is greater than twice the R-DAT magnetic
reversal rate of 9.408 MHz. This means a sample rate of at least 18.816 MHz.
In practice, you should give even more room than this bare minimum to account
for analog filter roll-off and other effects. My rule of thumb is to sample
at at least 1.2 times this minimum rate, or 22.6 MHz.

In my projects I ran my converter at its maximum rate or 25 MHz.

## Resampler, depth expander

```
                    |
    [ Digital samples, > 6 bit depth ]
                    |
                    v
 ,--------------------------------------,
 | Resampler (up-sampler) to 75.264 MHz |
 `--------------------------------------'
                    |
       [ IEEE Floats @ 76.264 MHz ]
                    |
                    v
 ```
To make clock detection and other signal extraction techniques in the
software easier to perform, the software currently requires its input
signal be sampled to a rate that is exactly equal to eight times the
base R-DAT signal rate: 75.264 MHz. Additionally, it requires that the
input be in native-endian IEEE 32-bit floating point format.

In my projects, I used a GNURadio rational resampler block to convert
from the 25 MHz A/D sample rate up to the desired 75.264 MHz. In this process I
also converted the samples from my A/D unit's 16-bit format into IEEE 32-bit
floating point format, natively-endian output.

## Enter the Software Domain: Magnetic pulse detector
```
                    |
       [ IEEE Floats @ 76.264 MHz ]
                    |
                    | 
 =========== Software domain =====================
                    |
                    v 
 ,---------------------------------------------------------,
 | Magnetic pulse clock detector and symbol slicer         |
 | ( RDATDecoder.cc)                                       |
 `---------------------------------------------------------'
               |              |                |
      [ Clock change ] [ Track start/stop ] [ Bit ]
               |              |                |
               v              v                v
```
Now that the input has been digitized and formatted correctly, the software
takes over. From here on, I will describe the software components as
signal blocks, showing each block's input signals and output signals.

Signal processing begins with a magnetic pulse detector, clock detector,
and general energy detector. These are really divided into two separate
tasks.

### Energy/envelope following

The energy/envelope follower monitors the incoming samples to discern when
a head pass across the tape begins and ends. This is an important step
because the interpretation of the magnetic pulses that make up the R-DAT
signal depends somewhat on the pulse's position within the head pass. The
follower outputs just one signal: a head pass appears to have started,
and a head pass appears to have completed.

### Clock detection and symbol decider
```
                                       INPUT
        |
Head    |
style B |    ,,        ,,        ,,        ,,        ,,             ,,
output  |   /  \      /  \      /  \      /  \      /  \           /  \
signal  |--'    \    /    \    /    \    /    \    /    \         /
        |        \__/      \__/      \__/      \__/      \_______/
        |
        |
        |    |    |    |    |    |    |    |    |    |    |    |    |
Symbol  |    | 1  | 1  | 1  | 1  | 1  | 1  | 1  | 1  | 1  | 0  | 1  |  
        |    |    |    |    |    |    |    |    |    |    |    |    |
        |     (      PREAMBLE                            ) ( SYNC WORD ...
                                   INTERPRETATION
                                   
                                   --- TIME --->
```
Alongside the head pass detector, the software runs the input samples
through a clock detection algorithm and, thereafter, a symbol "slicer".

Each R-DAT head swipe begins with a simple repeated pulse train that
carries no information other than the clock signal of the R-DAT machine
which wrote the tape. The software trains itself on this sequence so
that, when the more important information-bearing sections of the head
swipe begin, the software will be correctly synchronized with the signal.

This is necessary because R-DAT signals information bits merely by the
presence or absence of a magnetic reversal during each time period. When
correctly synchronized, the software will accurately be able to report
that a pulse is _absent_ during a period, rather than incorrectly
determining that it is absent because it was merely looking at the wrong
time.

In the end, clock detector and symbol slicer emit two signals: a
"clock has started/clock has stopped" signal, and a "bit detected
(1 or 0)" signal.

## NRZI-decoder, SYNC detector, 10-bit word framer

```
               |              |                |
      [ Clock change ] [ Track start/stop ] [ Bit ]
               |              |                |
               v              v                v
 ,---------------------------------------------------------,
 | SYNC detector, 10-bit WORD deframer                     |
 | (NRZISyncDeframer.cc)                                   |
 `---------------------------------------------------------'
                 |                       |
         [ Track start/stop ]     [ 10-bit Word ]
                 |                       |
                 v                       v
```
Interprets incoming detected bits under the NRZI scheme
(where an input of '1' signals that the current bit
differs from the previous bit, whereas a '0' signals
that it remains the same). Watches the decoded stream
of signaled bits for the special SYNC pattern, and
thereafter, collects every ten bits into a "word" which
is then produced as the output.

## 10-to-8 decoder, block assembler

```
 ,---------------------------------------------------------,
 | 10-to-8 decoder, block assembler                        |
 | (DATWordReceiver.cc)                                    |
 `---------------------------------------------------------'
           |                 |
    [ Track change ] [ 36-byte Block ]
           |                 |
           v                 v
```
Decodes incoming 10-bit words into 8-bit bytes, noting
if the word is valid or invalid. Starting at every SYNC
symbol assembles every 36 received bytes into a DAT
"block". (For resiliency, it will also start a new
block after 36 received bytes, even if no SYNC is
Emits blocks as output.

```
 ,---------------------------------------------------------,
 | Track/swipe assembler                                   |
 |=========================================================|
 | Watches incoming track-change signals and 36-byte       |
 | blocks, performs ECC-1 error correction on each block,  |
 | inspects the block address field, then places the block |
 | into its proper place in the 144-block array that makes |
 | up a "track".                                           |
 |                                                         |
 | Uses a state machine that is a bit forgiving if a       |
 | block's address field is corrupted and assumes that the |
 | block fits in the slot just after its parent block, if  |
 | this happens.                                           |
 |                                                         |
 | When a full track has been assembled, discerns whether  |
 | the track is an negative azimuth head swipe ("A") or a  |
 | positive azimuth head swipe ("B").                      |
 |                                                         |
 | Emits the assembled track (and A/B indicator) as output.|
 |                                                         |
 | (DATTrackFramer.cc)                                     |
 `---------------------------------------------------------'
                             |
                      [ Track frame ]
                             |
                             v
 ,---------------------------------------------------------,
 | Track pairer, decoder                                   |
 |=========================================================|
 | From here, the code forks into two paths, depending on  |
 | whether the user has asked for DAT-Audio decoding or    |
 | Digital Data Storage (DDS 1) decoding.                  |
 |
 | In both cases, a state machine waits until it has       |
 | received both an "A" head swipe frame and a "B" head    |
 | swipe frame. The absolute time sub-code of the swipes   |
 | are compared to ensure that they agree, and then the    |
 | data sections of the two swipes are de-interleaved and  |
 | assmbled into a full "DAT Frame" consisting of 5824     |
 | data bytes.                                             |
 |                                                         |
 | The code then perfoms ECC-2 error correction on the     |
 | assembled 5824-byte block, yielding a 5760-byte user-   |
 | data block.                                             |
 |                                                         |
 |---------------------------------------------------------|
 | DAT Audio pairer, decoder                               |
 |---------------------------------------------------------|
 | If the DAT Audio mode has been configured, then         |
 | interesting sub-code information such as the program    |
 | number, date and time, ISRC codes, and SMPTE timestamps.|
 | If so configured, it will also dump the user-data to a  |
 | WAV file as 16-bit signed, stereo samples.              |----> WAV file
 |                                                         |
 | (AudioFrameReceiver.cc)                                 |
 |                                                         |
 |---------------------------------------------------------|
 | DDS 1 pairer, decoder                                   |
 |---------------------------------------------------------|
 | If the DDS1 mode has been selected, then incoming DAT   |
 | frames are inspected for a Basic Group ID sub-code      |
 | marker (which ranges from 0 to 22). When all 23 basic   |
 | groups have been collected, their user-data areas are   |
 | demultiplexed, "de-whitened", and then merged into a    |
 | 132,480-byte "G1" block. The G1 block is then run       |
 | a third and final error correction routine (ECC-3),     |
 | yielding a 126,632-byte block known as a "Basic Group". |
 |                                                         |
 | A basic group consists of concatenated data records and |
 | a "footer" which describes the size of each record.     |
 |                                                         |
 | Each assembled basic group is dumped to a '.bin' file,  |
 | which can be processed later via Python to assemble     |---> "g.bin" file
 | recovered tape files.                                   |
 |                                                         |
 | (DDSFrameReceiver.cc)                                   |
 `---------------------------------------------------------'
```

# Example output

## An audio session

```
Absolute time: 00h-00m-10s-20f
Date     time: 06 1994-07-29 19:39:01
Errors  C1/C2: 12/0 (all corrected)
Samples      : L    R
               ff38 00e2
               ff69 0123
               ff73 013c
               ff62 0166
               ff75 0195
               ff7e 018f
               ff86 0196
               ffa1 01a5

Absolute time: 00h-00m-10s-21f
Date     time: 06 1994-07-29 19:39:01
Errors  C1/C2: 8/0 (all corrected)
Samples      : L    R
               00fa 01ee
               00f6 021f
               00fe 0237
               0113 0251
               00ee 0278
               00e5 02b4
               00ea 02ff
               00b3 0339

Absolute time: 00h-00m-10s-22f
Date     time: 06 1994-07-29 19:39:01
Errors  C1/C2: 4/0 (all corrected)
Samples      : L    R
               fe31 009f
               fe4a 006f
               fe4f 004a
               fe3d 0025
               fe37 ffeb
               fe48 ffc3
               fe5b ffab
               fe64 ff91
```

## A DDS session

```
Area          : DATA
Absolute frame: 006830
Basic Group   : 00289
Sub frame     : 20
File          : 0002
Errors  C1/C2 : 129/17 (all corrected)

Area          : DATA
Absolute frame: 006831
Basic Group   : 00289
Sub frame     : 21
File          : 0002
Errors  C1/C2 : 130/15 (all corrected)

Area          : DATA
Absolute frame: 006832
Basic Group   : 00289
Sub frame     : 22
File          : 0002
Errors  C1/C2 : 114/11 (all corrected)

Area          : DATA
Absolute frame: 006833
Basic Group   : 00289
Sub frame     : 23 (Last of group) (ECC3)
File          : 0002
Errors  C1/C2 : 122/9 (all corrected)
Group ECC3    : GOOD
------------------------------------------------------------

Area          : DATA
Absolute frame: 006834
Basic Group   : 00290
Sub frame     : 01
File          : 0002
Errors  C1/C2 : 132/17 (all corrected)

Area          : DATA
Absolute frame: 006835
Basic Group   : 00290
Sub frame     : 02
File          : 0002
Errors  C1/C2 : 121/12 (all corrected)
```
