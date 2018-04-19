# r-dat
A software suite for recovering and decoding data stored on Digital Audio Tape
(DAT) and Digital Data Storage (DDS) tapes from high-bandwidth digitized
recordings taken directly from the tape head.

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
