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
