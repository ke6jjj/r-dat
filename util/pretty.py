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

def pretty_c_print(table, data_width, width, indent=2):
  '''Pretty print a big table in C array format.'''
  if data_width == 8:
    fmt = '0x%02x'
    fmt_len = 4
  elif data_width == 32:
    fmt = '0x%08x'
    fmt_len = 10
  
  out = ''
  indent_s = ' ' * indent
  cur_len = indent
  table_len = len(table)
  last_i = table_len - 1
  first_item = True
  for i in range(table_len):
    while True:
      item = table[i]
      s = fmt % item
      if first_item:
        s = indent_s + s
      if i != last_i:
        s += ','
      item_len = len(s)
      if (cur_len + item_len < width) or first_item:
        out += s
        first_item = False
        cur_len += item_len
        break
      else:
        out += '\n'
        cur_len = 0
        first_item = True
        continue
  return out
