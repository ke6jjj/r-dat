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

'''This module helps build quick lookup tables for the data whitening
algorithm used by DDS when it transforms a Group 1 frame into a Group 2
frame.

Data is whitened by combining it with the output of a linear feedback
shift register 15 bits long.
'''

def lfsr_crank(v):
  '''Crank the LFSR by one bit. The feedback tap is fed by the GF(2)
  addition (XOR) of bits 0 and 1 currently in the register.'''
  feedback = (v & 1) ^ ((v >> 1) & 1)
  v = v >> 1
  if feedback:
    v |= 0x4000
  return v

def lfsr_crank_8(v):
  '''Crank the LFSR eight times. This is enough to get the LFSR into
  the state where its bottom bits can be XORed with the next byte.'''
  for i in range(8):
    v = lfsr_crank(v)
  return v

def generate_lfsr_table():
  '''Generate an 8-bit LFSR state modification table. For an LFSR in a
  particular state x[n] this table can be used to compute the LFSR's state
  after eight more cranks (x[n+8]).
  
  x[n+8] = (table(x[n] & 0x1ff) << 7) | (x[n] >> 8)
  '''
  r = []
  for v in range(512):
    next_v = lfsr_crank_8(v)
    r.append((next_v >> 7) & 0xff)
  return r

def do():
  import pretty
  
  t = generate_lfsr_table()
  print pretty.pretty_c_print(t, 8, 80)

def do_binary():
  v = 1
  for i in range(8*32):
    if (i % 8) == 0:
      print hex(i/8),
    s = ''
    for j in range(15):
      if v & (1 << (14-j)):
        s += '1'
      else:
        s += '0'
      if j == 6:
        s += ' '
    print s
    v = lfsr_crank(v)

do()
  
