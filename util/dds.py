#
# Copyright 2018, Jeremy Cooper
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

import sys
import struct
import exceptions
import getopt

ascii = False
relax = False
verbose = 0

def usage(code):
  msg = \
  '''usage: dds [-hqva] <start-group> <file-no> <outfile>
  This utility extracts files from a raw DDS dump, parsing the Basic Group 
  files in a directory and checking the data validity reported by the decode
  process.

  <start-group>   The number of the basic group to start scanning.
  <file-no>       The number of the file to extract.
  <outfile>       Pathname of the file to receive the data.

  -h              Print this help message.
  -q              Relax requirements for data validity; invalid bytes will
                  not be checked.
  -v              Verbose output during extraction.
  -a              ASCII extract; add newlines after every record.'''
  print >>sys.stderr, msg
  sys.exit(code)
  
(opts, args) = getopt.getopt(sys.argv[1:], 'hqva')
for opt, val in opts:
  if opt == '-a':
    ascii = True
  elif opt == '-q':
    relax = True
  elif opt == '-v':
    verbose += 1
  elif opt == '-h':
    usage(0)

if len(args) < 3:
  usage(1)

start_group = int(args[0])
file_no = int(args[1])
out_file = args[2]

class BlockTableEntry(object):
  def __init__(self, bytes):
    self.Item = ord(bytes[0]) & 0xf7
    self.Count = (ord(bytes[1]) << 16) + \
                 (ord(bytes[2]) << 8) + \
                 ord(bytes[3])

def AllValid(v):
  return reduce(lambda x,y: x and y != '\x00', v, True)

class BasicGroup(object):
  kSize = 126632
  kHeaderSize = 32
  
  def __init__(self, data, vdata=None):
    assert(len(data) == self.kSize)
    self._data = data
    self._vdata = vdata
    self._user_data_size = None
    self._group_number = None
    self._ParseHeader()
  
  def _ParseHeader(self):
    if self._vdata is not None:
      #
      # We have validity data. Make certain that
      # the block access table is valid.
      #
      v = self._vdata[126600:126632]
      assert(AllValid(v))
    hdr_bytes = self._data[126600:126632]
    (
      GroupNumber,
      BlockAccessTableCount,
      RecordCount,
      Separator1Count,
      dummy_1,
      Separator2Count,
      GroupRecordCount,
      PreviousRecordGroupNumber,
      GroupSeparator1Count,
      PreviousSeparator1GroupNumber,
      GroupSeparator2Count,
      PreviousSeparator2GroupNumber,
      dummy_2
    ) = struct.unpack('>HHIIHHHHHHHHI', hdr_bytes)
    self._GroupNumber = GroupNumber
    self._BlockAccessTableCount = BlockAccessTableCount
    self._RecordCount = RecordCount
    self._Separator1Count = Separator1Count
    self._Separator2Count = Separator2Count
    self._GroupRecordCount = GroupRecordCount
    self._PreviousRecordGroupNumber = PreviousRecordGroupNumber
    self._GroupSeparator1Count = GroupSeparator1Count
    self._PreviousSeparator1GroupNumber = PreviousSeparator1GroupNumber
    self._GroupSeparator2Count = GroupSeparator2Count
    self._PreviousSeparator2GroupNumber = PreviousSeparator2GroupNumber
    self._BlockAccessTable = []
    #
    # Parse the block access table.
    #
    for i in range(BlockAccessTableCount):
      item_pos = self.kSize - self.kHeaderSize - (i+1) * 4
      item = self._data[item_pos:item_pos+4]
      if self._vdata is not None:
        #
        # We have validity data. Make certain that
        # the item is valid.
        #
        v = self._vdata[item_pos:item_pos+4]
        assert(AllValid(v))
      self._BlockAccessTable.append(BlockTableEntry(item))
    
  def Data(self):
    return self._data

  def Valid(self):
    return self._vdata

  @classmethod
  def FromFile(cls, path, valid_path):
    global relax
    f = open(path, 'rb')
    d = f.read(cls.kSize)
    f.close()
    try:
      f = open(valid_path, 'rb')
      v = f.read(cls.kSize)
      f.close()
    except exceptions.IOError, e:
      if e.errno == 2:
        # File wasn't there. Assume all valid
        v = [1] * cls.kSize
      else:
        v = [0] * cls.kSize
    if not relax:
      # Make sure every byte of the group is valid.
      assert(AllValid(v))
    return cls(d, v)

class Entity(object):
  '''DDS2 compression entity. ECMA 198.'''
  def __init__(self, data, vdata=None):
    self._data = data
    self._vdata = vdata
    self._ParseHeader()

  def _ParseHeader(self):
    if self._vdata is not None:
      assert(AllValid(self._vdata[0:8]))
    d = self._data

    EntityHeaderSize = ord(d[0])
    Zero1 = ord(d[1])
    AccessPointId = ord(d[2])
    UnprocessedRecordLength = (ord(d[3]) << 16) + \
                              (ord(d[4]) <<  8) + \
                              (ord(d[5])      )
    ProcessedRecordCount = (ord(d[6]) << 8) + ord(d[7])

    assert(EntityHeaderSize == 8)
    assert(Zero1 == 0)
    assert(AccessPointId == 0x20) # DLCZ
    self._EntityHeaderSize = EntityHeaderSize
    self._AccessPointId = AccessPointId
    self._UnprocessedRecordLength = UnprocessedRecordLength
    self._ProcessedRecordCount = ProcessedRecordCount

  def Dump(self, f):
    if verbose > 1:
      print " < Header %d, AccessPoint 0x%02x, RecSize 0x%04x, Count %d >" % \
       (self._EntityHeaderSize, self._AccessPointId,
        self._UnprocessedRecordLength, self._ProcessedRecordCount)
    dclz = DCLZ(self._data[8:])
    written = dclz.Expand(f)
    expected = self._UnprocessedRecordLength * self._ProcessedRecordCount
    assert(written == expected)

  @classmethod
  def FromBytes(cls, b, v=None):
    return cls(b, v)

class DCLZ(object):
  '''Data Compression due to Lempel and Ziv.'''

  def __init__(self, data):
    self._data = data
    self._datalen = len(data)
    self.Reset()

  def Reset(self):
    self._total_written = 0
    self._pos = 0
    self._bits = 0
    self._accum = 0
    self._pad_warning = False
    self._end_record_pending = False
    self.ResetDictionary()

  def ResetDictionary(self):
    self._last_string = ''
    self._next_entry = 264
    self._frozen = False
    self._dictionary = {}
    self.SetCodeWordSize(9)

  def SetCodeWordSize(self, n):
    self._cwsize = n
    self._mask = ((1 << n) - 1)

  def AddToDictionary(self, item):
    assert(not self._frozen)
    if len(item) < 129:
      if verbose > 2:
        print "  Dictionary[%d] = %s" % (self._next_entry, item.encode('hex'))
      self._dictionary[self._next_entry] = item
      self._next_entry += 1
      if self._next_entry == 4096:
        self._frozen = True

  def IncrementCodeWordSize(self):
    assert(self._cwsize < 12)
    self.SetCodeWordSize(self._cwsize + 1)

  def Emit(self, item, f):
    '''Emit some bytes from the expansion process,
    possibly updating the local dictionary in the process.'''

    #
    # Write the string to the stream.
    #
    f.write(item)
    self._total_written += len(item)

    #
    # Update dictionary
    #
    if not self._frozen and self._last_string != '':
      self.AddToDictionary(self._last_string + item[0])

    #
    # Remember last string emitted.
    #
    if self._end_record_pending:
      self._end_record_pending = False
      self._last_string = ''
    else:
      self._last_string = item

  def Flush(self):
    self._bits = 0
    self._accum = 0

  def GetNextCode(self):
    '''Get the next codeword from the stream. Takes the current
    codeword size into account.'''

    while self._bits < self._cwsize:
      #
      # We need more bits from the stream. Read in another byte.
      #

      #
      # Make sure there's more data available.
      #
      if self._pos == self._datalen:
        return None

      #
      # Get a byte value.
      #
      c = ord(self._data[self._pos])
      if verbose > 3:
        print "Read character %02x" % c

      #
      # Place it in the higher order bits of the accumulator.
      #
      v = c << self._bits
      self._accum |= v

      #
      # Update the current byte pointer and number of bits
      # in the accumulator.
      #
      self._pos += 1
      self._bits += 8

    #
    # Take a code word from the bottom bits of the accumulator.
    #
    x = self._accum & self._mask

    #
    # Shift the accumulator to account for the taken bits.
    #
    if self._pad_warning:
      #
      # The decoder has been told that this codeword will be padded to
      # the next byte boundary. Obey this note and dump any bits that
      # are still in the accumulator; they are all padding bits.
      #
      self.Flush()
      self._pad_warning = False
    else:
      #
      # Update the accumulator.
      #
      self._accum = self._accum >> self._cwsize
      self._bits -= self._cwsize

    return x

  def Expand(self, f):
    while True:
      cw = self.GetNextCode()

      if cw == 0:
        #
        # Dictionary freeze
        #
        if verbose > 2:
          print 'Dictionary freeze ---'
        self._frozen = True

      elif cw == 1:
        #
        # Dictionary reset
        #
        if verbose > 2:
          print 'Dictionary reset ---'

        #
        # Reset codewords and flush to next
        # byte boundary.
        #
        self.ResetDictionary()
        self.Flush()

      elif cw == 2:
        #
        # Increment word size
        #
        if verbose > 2:
          print 'Increment word size ---'
        self.IncrementCodeWordSize()

      elif cw == 3:
        #
        # End of record.
        #
        if verbose > 2:
          print 'End of record ---'

        #
        # Restart stream
        #
        self.Flush()

        #
        # Next code word will also be padded.
        #
        self._pad_warning = True
        self._end_record_pending = True

      elif cw >= 4 and cw <= 7:
        print 'Illegal code', cw
        assert(0)

      elif cw >= 8 and cw <= 263:
        #
        # Single byte.
        #
        if verbose > 2:
          print "Encoded byte %02x" % (cw - 8)
        item = chr(cw - 8)
        self.Emit(item, f)

      elif cw >= 264 and cw <= 4095:
        #
        # Dictionary entry.
        #
        if verbose > 2:
          print 'Dictionary entry', cw

        #
        # Sometimes the encoder emits references to
        # dictionary entries that we've never seen. But
        # when it does so it must refer to the next available
        # dictionary slot and no further.
        #
        if cw == self._next_entry:
         #
         # Encoder has asked us to expand a dictionary entry that
         # was just created. The only dictionary entry that it could
         # possibly create at this point would be the last string plus
         # its first character.
         #
         item = self._last_string + self._last_string[0]
        else:
         #
         # Encoder wants us to emit the bytes in an existing dictionary
         # entry.
         #
         item = self._dictionary[cw]

        self.Emit(item, f)

      elif cw is None:
        break

    #
    # Done decoding.
    #
    if verbose > 1:
      print '  DCLZ wrote %d bytes' % self._total_written

    return self._total_written

def GroupFileName(num):
  return 'g%06d.bin' % num

def GroupValidFileName(num):
  return 'g%06d.val' % num

group_number = start_group
going = True

f = open(out_file, 'wb')

entity_bytes = ''
entity_valid = ''

while going:
  try:
    g_path = GroupFileName(group_number)
    v_path = GroupValidFileName(group_number)
    group = BasicGroup.FromFile(g_path, v_path)
  except exceptions.IOError, e:
    going = False
    break

  #
  # Figure out which file the first byte of this group belongs
  # to.
  #
  g_file = group._Separator1Count - group._GroupSeparator1Count

  if verbose:
    print "Group", group._GroupNumber, "file", g_file

  #
  # Does this group have data for the file we're dumping?
  #
  if g_file <= file_no and file_no <= group._Separator1Count:
    #
    # Yes.
    #
    data = group.Data()
    valid = group.Valid()
    
    #
    # Walk the block access table, writing bytes that are a part
    # of the stream.
    #
    pos = 0
    for entry in group._BlockAccessTable:
      if entry.Item == 0x07:
        #
        # Separator mark. Could be a file mark or a set mark.
        #
        if entry.Count == 0:
          #
          # File mark. Increment the current file count.
          #
          g_file += 1
          if verbose >= 2:
            print " File mark."
      elif \
         entry.Item == 0x40 or \
         entry.Item == 0x42 or \
         entry.Item == 0x60 or \
         entry.Item == 0x63:
        #
        # Full or partial record. Incorporate it so long as we're
        # in the file being dumped.
        #
        size = entry.Count
        if g_file == file_no:
          #
          # Incorporate these bytes.
          #
          if verbose >= 2:
            print "  Writing %d bytes at offset %d" % (size, pos)
          f.write(data[pos:pos+size])
          if ascii and (entry.Item == 0x60 or entry.Item == 0x63):
            f.write('\n')

        #
        # Regardless of which file this record belongs to,
        # update the position pointer within the group.
        #
        pos += size
      elif entry.Item == 0x80:
        #
        # Skip entry. Skip these bytes.
        #
        pos += entry.Count
        if verbose >= 2:
          print " Skip entry. %d bytes" % (entry.Count,)
      elif entry.Item == 0x73:
        #
        # DDS DCLZ entity: complete entity.
        #
        size = entry.Count
        if verbose >= 2:
          print " Entire entity, %d bytes" % (size,)

        entity_bytes = data[pos:pos+size]
        entity_valid = valid[pos:pos+size]
        e = Entity.FromBytes(entity_bytes, entity_valid)
        e.Dump(f)
        entity_bytes = ''
        entity_valid = ''
        pos += size
      elif entry.Item == 0x52:
        #
        # DDS DCLZ entity: entity start.
        #
        size = entry.Count
        if verbose >= 2:
          print " Entity start, %d bytes" % (size,)

        assert(entity_bytes == '')
        entity_bytes = data[pos:pos+size]
        entity_valid = valid[pos:pos+size]
        pos += size
      elif entry.Item == 0x50:
        #
        # DDS DCLZ entity: entity continuation.
        #
        size = entry.Count
        if verbose >= 2:
          print " Entity continue, %d bytes" % (size,)

        assert(entity_bytes != '')
        entity_bytes += data[pos:pos+size]
        entity_valid += valid[pos:pos+size]
        pos += size
      elif entry.Item == 0x70:
        #
        # DDS DCLZ entity: entity, last part.
        #
        size = entry.Count
        if verbose >= 2:
          print " Entity last part, %d bytes" % (size,)

        assert(entity_bytes != '')
        entity_bytes += data[pos:pos+size]
        entity_valid += valid[pos:pos+size]
        if verbose >= 2:
          print "  (total %d bytes)" % (len(entity_bytes),)
        e = Entity.FromBytes(entity_bytes, entity_valid)
        e.Dump(f)
        entity_bytes = ''
        entity_valid = ''
        pos += size
      elif entry.Item == 0x11:
        #
        # Entity completed. Size check.
        #
        size = entry.Count
        if verbose >= 2:
          print " Entity total %d bytes" % (size,)
      else:
        if verbose >= 2:
          print " Unknown entry type: %02x" % (entry.Item,)

  group_number += 1

f.close()
