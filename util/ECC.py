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

'''The galois field for this code is modulo this polynomial.

  G(x) = x^8 + x^4 + x^3 + x^2 + 1

Section 9.3.4 G4 Sub-group
'''

mod_poly = 0x11d
mod_poly_top = 0x100

'''The primitive element, alpha, of the code.'''
primitive = 0x2

def multiply(a,b):
  '''Multiply polynomial a by polynomial b and reduce the result
  mod mod_poly.
  
  Optimization requires that a and b must both be reduced modulo
  mod_poly before begining.'''
  assert(a < 0x100 and b < 0x100)
  acc = 0
  bits = 8
  for bit in range(bits):
    #
    # Multiply the current accumulator polynomial by x
    #
    acc <<= 1
    
    #
    # Reduce modulo mod_poly
    #
    if acc & mod_poly_top:
      acc ^= mod_poly
    
    #
    # Add in a copy of b if the current top bit of a says so.
    #
    if a & 0x80:
      acc ^= b
    
    #
    # Get a ready for its next bit.
    #
    a = (a << 1) & 0xff
  
  return acc

def invert(a):
  '''Compute the multiplicative inverse of a in the field.'''
  return pow(a, 254)

def pow(a, b):
  '''Raise polynomial a to the integer power b and reduce the result
  modulo mod_poly.'''
  #
  # Get b in binary representation, highest bit first.
  #
  bits = splat(b)
  
  #
  # Perform the power operation using the binary exponentiation algorithm.
  #
  acc = 1
  for bit in bits:
    #
    # First step, always square the current accumulator.
    #
    acc = multiply(acc,acc)

    #
    # Second step: if the current high bit of b is set, multiply the
    # accumulator by a.
    #
    if bit:
      acc = multiply(acc, a)

  return acc

def scalar_multiply(p, s):
  '''Multiply the polynomial p by the scalar value s.'''
  r = [0] * len(p)
  for i in range(len(p)):
    r[i] = multiply(p[i], s)
  return r

def poly_multiply(p, q):
  '''Multiply the polynomial in p by the polynomial in q.'''
  new_l = len(p) + len(q) - 1
  r = [0] * new_l
  for i in range(len(p)):
    for j in range(len(q)):
      r[i+j] ^= multiply(p[i],q[j])
  return r

def poly_add(p, q):
  '''Add the polynomial p to the polynomial q.'''
  lp = len(p)
  lq = len(q)
  new_l = max(lp, lq)
  m = min(lp, lq)
  r = [0] * new_l
  for i in range(m):
    r[i] = p[i] ^ q[i]
  if lp > lq:
    b = p
  else:
    b = q
  for i in range(m, new_l):
    r[i] = b[i]
  
  return r

def poly_evaluate(p, x):
  '''Evaluate the polynomial p(x) with a specific value for x.'''
  res = 0
  for i in range(len(p)):
    res ^= multiply(p[i], pow(x, i))
  return res

def poly_divide_by_x_plus_y(a, y):
  '''Divide polynomial a(x) by polynomial (x + y)'.

 Returns (dividend, remainder, is_zero). is_zero is true iff
 dividend is zero and remainder is zero.'''
  l = len(a)
  if l == 0:
    return ([0], [0], True)

  #
  # We're going to modify a in place. Make a copy so that we
  # don't destroy caller's list.
  #
  a = a[:]

  #
  # Start out with an empty dividend.
  #
  d = []
  is_zero = True
  for i in range(l-1,0,-1):
    #
    # Long division. Multiply (x + y) by Ax^(n-1) to get:
    #
    #  Ax^n + AYx^(n-1)
    #
    # Then subtract.
    #
    #          A   x^(n-1)
    #  =============================
    #  Ax^n +  B   x^(n-1) + ... + Z
    #  Ax^n + AY   x^(n-1)
    #  --------------------------
    #        (AY+B)x^(n-1)
    #
    d.insert(0,a[i]) # Dividend
    is_zero = is_zero and a[i] == 0
    a[i-1] ^= multiply(y, a[i])

  r = a[0]
  is_zero = is_zero and r == 0

  return (d, r, is_zero)
      
def splat(x):
  '''Get a binary vector representation of the integer x. The returned
  vector will be in most-significant-bit first order. Always returns at least
  one bit. (I.e. splat(0) == [0])
  
  E.g: splat(6) == [1, 1, 0]'''
  acc = []
  while True:
    acc.insert(0, x & 1)
    x = x >> 1
    if x == 0:
      break
  
  return acc

def generate_alpha_powers():
  '''Generate all of the powers of alpha (from 0 to 255), the primitive element
  of this code.'''
  a = []
  for i in range(256):
    a.append(pow(primitive, i))
  return a

def generate_log_alpha_table():
  '''Generate a lookup table that takes a field element (a byte) and returns
  the discrete log of that element terms of the base primitive, alpha.'''
  t = [0] * 256
  for i, x in enumerate(generate_alpha_powers()):
    t[x] = i%255
  return t

def generate_inverses():
  r = [0]
  for i in range(1, 256): 
    r.append(invert(i))
  return r

def generate_Hp():
  '''Generate the P check matrix Hp:
  
  1     1     1     1    ... 1    1    1
  a^31  a^30  a^29  a^28 ... a^2  a^1  1
  a^62  a^60  a^58  a^56 ... a^4  a^2  1
  a^93  a^90  a^87  a^84 ... a^6  a^3  1
  
  I have discovered that Reed-Solomon codes work for other check matricies
  as well. Another matrix you will often see is
  
  a^31  a^30  a^29  a^28  ... a^2  a^1  1
  a^62  a^60  a^58  a^56  ... a^4  a^2  1
  a^93  a^90  a^87  a^84  ... a^6  a^3  1
  a^124 a^120 a^116 a^112 ... a^8  a^4  1
  
  It just so happens that DAT and DDS use the first matrix. In general,
  any integer b0 can be used as the starting power for alpha, the consecutive
  rows beyond that must just be sequential multiples of that power. DAT
  uses 0 as a starting power, others use 1. [Modified Euclidean Algorithms
  for Decoding Reed-Solomon Codes. Sarwate, Yan.]
  
  The starting power affects the decoding routine I am using in so much as
  that Forney's formula requires the starting power. It labels this quantity
  as "b0". DAT uses a b0 = 0, and hence, the first row of the matrix is

  a^(0*31) a^(0*30) a^(0*29) ... a^(0*0) = 1 1 1 ... 1

  '''
  Hp = [[0]*32,[0]*32,[0]*32,[0]*32]
  for i in range(4):
    for j in range(32):
      Hp[i][31-j] = pow(primitive, j*i)
  
  return Hp

def generate_Hq():
  '''Generate the Q check matrix Hq:

  1     1     1     1     ... 1    1    1
  a^31  a^30  a^29  a^28  ... a^2  a^1  1
  a^62  a^60  a^58  a^56  ... a^4  a^2  1
  a^93  a^90  a^87  a^84  ... a^6  a^3  1
  a^124 a^120 a^116 a^112 ... a^8  a^4  1
  a^155 a^150 a^145 a^140 ... a^10 a^5  1
  '''
  Hq = [[0]*32,[0]*32,[0]*32,[0]*32,[0]*32,[0]*32]
  for i in range(6):
    for j in range(32):
      Hq[i][31-j] = pow(primitive, j*i)
  
  return Hq

def generate_Hi():
  '''Generate the I check matrix (for DDS ECC3) Hi:

  1     1     1     1    ... 1    1    1
  a^45  a^44  a^43  a^42 ... a^2  a^1  1
  '''
  Hi = [[0]*46,[0]*46]
  for i in range(2):
    for j in range(46):
      Hi[i][45-j] = pow(primitive, j*i)

  return Hi

def generate_multiplication_table():
  '''Generate the multiplication table for a x b.'''
  res = []
  for a in range(256):
    for b in range(256):
      res.append(multiply(a,b))
  return res

def Vps_for_block_pair(a,b):
  '''Deinterleave the bytes from two 32-byte blocks into two 32-byte Vp
  Reed-Solomon arrays suitable for Reed-Solomon checking.'''
  v1 = [0] * 32
  v2 = [0] * 32
  
  for i in range(16):
    v1[i   ] = a[i*2]
    v1[i+16] = b[i*2]
    v2[i   ] = a[i*2+1]
    v2[i+16] = b[i*2+1]
  
  return (v1, v2)

def block_pair_from_Vps(v1, v2):
  '''Interleave the bytes from two 32-byte Vp Reed-Solomon arrays into
  two 32-byte DAT blocks.'''
  a = [0] * 32
  b = [0] * 32

  for i in range(16):
    a[i*2  ] = v1[i]
    b[i*2  ] = v1[i+16]
    a[i*2+1] = v2[i]
    b[i*2+1] = v2[i+16]

  return (a, b)

def check_Vp_Hp(Vp, Hp):
  '''Multiply 32-byte vector Vp by a Reed-Solomon check matrix and return the
  result.'''
  assert(len(Hp[0]) == len(Vp))
  rows = len(Hp)
  r = [0] * rows
  for i in range(rows):
    for j in range(len(Vp)):
      r[i] ^= multiply(Vp[j], Hp[i][j])
  return r

def is_zero(p):
  '''Returns true if p is zero or represents a zero-valued polynomial.'''
  if p == 0:
    return True
  return reduce(lambda x,y: x and (y == 0), p, True)
  
def sy_compute_sigma(S, E):
  '''Compute the error locator polynomial from a syndrome vector and a
  known erasures vector using the Extended Euclidean algorithm for
  Errors-and-Erasures decoding as described by a paper by Sarwate and Yan.'''
  #
  # First, acknowledge that the number of syndrome elements is the same
  # as twice the error correction capability of the code, "t".
  #
  two_t = len(S) 
  
  #
  # Construct a polynomial in z of degree 2t.
  #
  #         2t
  # U(z) = z   = [0, 0, ..., 1]
  #
  U = [0] * (two_t + 1)
  U[two_t] = 1
  
  l = -1
  V = S[:]
  X = [1]
  W = [0]
  
  #
  # This is a mostly hardware algorithm that avoids if statements. The
  # stilted nature is part of the original paper. I have added some if
  # statements for clarity.
  #
  for i in range(two_t):
    #
    # Determine if we're still processing the known erasures polynomial.
    #
    first = len(E) != 0
    
    swap = (not first) and (V[two_t-1] != 0) and (l < 0)
    
    if not first:
      g = U[two_t]
      z = V[two_t - 1]
    else:
      g = E[0]
      z = 1
    
    if swap:
      l = -(l + 1)
    elif not first:
      l = l - 1
    
    #
    # Reduce the erasures polynomial by one degree, dropping off
    # the units term.
    #
    if len(E):
      E = E[1:]
    
    if not first:
      v_adjust = scalar_multiply(U, z)
      x_adjust = scalar_multiply(W, z)
    else:
      v_adjust = scalar_multiply(V, z)
      x_adjust = scalar_multiply(X, z)          
  
    #
    # Raise V and X's powers.
    #
    pow_V = [0] + V[0:two_t]
    pow_X = [0] + X[0:two_t]
    
    #
    # Multiply by scalar g.
    #
    new_V = scalar_multiply(pow_V, g)
    new_X = scalar_multiply(pow_X, g)
    
    #
    # Adjust respectively.
    #
    new_V = poly_add(new_V, v_adjust)
    new_X = poly_add(new_X, x_adjust)
    
    if swap:
      U = pow_V
      W = pow_X
    
    V = new_V
    X = new_X

  #
  # The error locator polynomial is now in X and the error magnitude
  # polynomial is in V.
  #
  # If l is less than zero, then the errors are correctable. If l equals
  # zero then the errors are correctable only if the erasures polynomial
  # is also zero.
  #
  return (X[0:two_t+1], V[0:two_t], l < 0 and (len(E) == 0))
  
def bk_compute_sigma(S):
  '''Compute the error locator polynomial from a syndrome vector using the
  Berlekamp-Massey algorithm.
  
  Not sure if this works really well.'''
  #
  # First step, figure out the maximum number of steps we're going to make.
  # The are are 2t syndrome symbols in a code. We use Berlekamps' iterative
  # algorithm here, and it takes 2t steps to complete. Therefore, the
  # number of steps is len(S).
  #
  steps = len(S)
  t = steps / 2
  
  #
  # The algorithm works by filling out rows in a table. Here we store
  # the table by its columns. E.g. the "d" entry for row 0 is d[0].
  #
  # Entries in sig (sigma) are polynomials. As such, each is a list of
  # coefficients. It begins with two entries, both x^0 (which is, of course,
  # 1).
  #
  # sig: Sigma, the error locator polynomial.
  #
  # d: Discrepency, the difference between Sigma and the closest Newton's
  #    identity.
  #
  # l: Degree, the degree (highest power) of Sigma.
  #
  sig = [ (1,), (1,) ]
  d   = [ 1, S[0] ]
  l   = [ 0, 0 ]
  
  
  #
  # Keep track of the row with the highest degree polynomial with
  # non-zero discrepency.
  #
  # (This always starts as the first row).
  #
  rho = 0

  #
  # Also, a speedup due to Chen allows us to stop the iteration of this
  # algorithm if some number of consecutive iterations yield zero discrepency.
  # Keep track of the number of consecutive zero-discrepency rows we've made.
  #
  if S[0] == 0:
    consecutive_zeros = 1
  else:
    consecutive_zeros = 0
  
  #
  # The table begins with two rows already filled in, so we need to compute
  # the remaining rows.
  #
  for row in range(1,steps):
    #
    # Check the Discrepency column of this row.
    #
    if d[row] == 0:
      #
      # It is zero. There's no change to the error locator polynomial.
      #
      new_sig = sig[row]
      new_l = l[row]
    else:
      #
      # The discrepency is non-zero. We must find the best previous
      # polynomial with non-zero discrepency. If we've been running the
      # algorithm correctly, then this row number will be in the variable
      # "rho".
      #
      # So the next step is to update the error locator polynomial by
      # combining it with the polynomial from row rho (row your boat), but,
      # before we do so, we are now in a position to check if the updated
      # polynomial's degree is going to cross the uncorrectable
      # threshold. That check is easy to do, so let's do it now.
      #
      # Compute the degree that the updated polynomial is going to have.
      #
      new_l = max(l[row], l[rho] + row - rho)
      
      #
      # Is the new polynomial going to be too big?
      #
      if new_l > t:
        #
        # Too big. This syndrome represents and uncorrectable error
        # pattern.
        #
        raise Exception('Uncorrectable')
      
      #
      # Compute the new polynomial.
      #
      # One of the subexpressions in the new polynomial equation requires
      # us to multiply an earlier polynomial in X by a power of X. This
      # is equivalent to shifting the earlier polynomial's coefficients
      # up by that power of X. Compute that power now.
      #
      # (You'll notice that row - rho is also in the computation of
      # new_l).
      #
      offset = row - rho
      
      #
      # Intialize a list to hold the new polynomial. Also, fill it
      # with the polynomial from the last row.
      #
      new_sig = [0] * (new_l + 1)
      for i in range(l[row]+1):
        new_sig[i] = sig[row][i]
      
      #
      # Calculate the value Du * Dp^1. It will be a coefficient we
      # apply to all of the coefficients of the old polynomial.
      #
      dudp = multiply(d[row], invert(d[rho]))
      
      #
      # Calculate the new polynomial.
      #
      for i in range(l[rho]+1):
        new_sig[i+offset] ^= multiply(sig[rho][i], dudp)
        
    #
    # The polynomial has been updated.  
    # Compute the new Discrepency.
    #
    new_d = S[row]
    for coeff in range(1,new_l+1):
      new_d ^= multiply(S[row-coeff], new_sig[coeff])
    
    #
    # Store the results.
    #
    sig.append(new_sig)
    d.append(new_d)
    l.append(new_l)
    
    #
    # We can stop the algorithm if we've computed some number of
    # consecutive zero discrepencies.
    #
    if new_d == 0:
      #
      # We have another zero discrepency row.
      #
      consecutive_zeros += 1
      if consecutive_zeros > t - new_l:
        #
        # Yep, we've hit the zero stop. We're done!
        #
        break
    else:
      #
      # This new row has non-zero discrepency. Reset the
      # consecutive count.
      #
      consecutive_zero = 0
      
    #
    # If this row has a non-zero discrepency and the difference
    # between its degree and row number is higher than that of the previous
    # rho, nominate this row as the new rho.
    #
    if d[row] != 0 and ((row - l[row]) > (rho - l[rho])):  
      rho = row
    
  #
  # Return the last error locator polynomial.
  #
  return sig[-1]

def find_primitive_roots(p, max_power):
  '''Find those powers of the field primitive (alpha) whose inverses are roots
  to the given polynomial, p. Useful for interpreting error-locator polynomials.
  
  p - Coefficients of the polynomial. The lowest power (units power)
  coefficient comes first.
  
  Returns the log(base alpha) of the results.'''
  roots = []
  for i in range(max_power):
    beta = invert(pow(primitive, i))
    res = poly_evaluate(p, beta)
    if res == 0:
      #
      # Found a root. Append it to the list
      #
      roots.append(i)

  return roots

def compute_error_at_location(sigma, omega, i):
  '''Compute the error value at codeword location i (zero-based) given
  the 'Z' function (my term) of the syndromes and the other locations
  in which there are errors.'''
  inv_beta_l = invert(pow(primitive, i))
  
  #
  # Evaluate the top of Forney's formula: send the error position
  # indicator through omega.
  #
  top = poly_evaluate(omega, inv_beta_l)
  
  #
  # To complete the top of Forney's formula we must multiply the previous
  # result by inv_beta_l^(b0). b0 is the power of the lowest power of
  # alpha in the code. It's a variable that is often zero or one, but
  # is up to the particular implementation of the code.
  #
  # DAT happens to use a b0 of zero. You can see this because the C1
  # and C2 check matricies that DAT uses in the code all begin with rows
  # of "1"'s. In other words, they all begin with rows of alpha^0.
  # Some codes begin with rows of incrementing powers of alpha. Those
  # codes use value of '1' for b0.
  #
  # Since our code has a b0 value of zero, inv_beta_l^b0 is one.
  # Multiplying by 1 can be skipped. So we skip it.
  #
  # Otherwise we would write:
  #
  # top = multiply(top, pow(inv_beta_l, b0))
  #
  
  #
  # Now, evaluate the error position indicator through the derivative
  # of the error locator polynomial.
  #
  # Due to some handy math and that we're working in GF(2), the derivative
  # of the error-locator polynomial is just every other of its terms!
  #
  res = 0
  y = 1
  for j in range(1,len(sigma)):
    if (j & 1) == 1:
      res ^= multiply(sigma[j], y)
    y = multiply(y, inv_beta_l)
  
  #
  # Like the top portion, the bottom portion must be multiplied by a value.
  # In this case it is just the error position indicator, inv_beta_l.
  #
  bottom = multiply(res, inv_beta_l)
  
  #
  # The return value is the top value divided by the bottom value!
  #
  return multiply(top, invert(bottom))
  
def test_c1():
  '''Test C1 error correction.'''
  block_pair_tests = [
  #
  # Here is an example subcode block pair, with two C1 ECC sections, which has
  # a single byte error in block 2.
  #                      
  #    20aaaa000024131756940729193914d820aaaa000024131720aaaa0000241317 
  # >> 20aaaa000024131756d40729193914d820aaaa0000241317abbbe79542da976d 
  #                      ^^                            ^^^^^^^^^^^^^^^^
  #                     Error                             C1 bytes (P)
  #
  # That 'd4' should be a '94'.
  #
    [ '20aaaa000024131756940729193914d820aaaa000024131720aaaa0000241317' \
      '20aaaa000024131756d40729193914d820aaaa0000241317abbbe79542da976d',
      # Corrected below
      '20aaaa000024131756940729193914d820aaaa000024131720aaaa0000241317' \
      '20aaaa000024131756940729193914d820aaaa0000241317abbbe79542da976d' ],
    
  #
  # Here is a block pair from the subcode section of an audio tape.
  # This block represents a good block.
  #
    [ '20aaaa000024131756940729193914d820aaaa000024131720aaaa0000241317' \
      '20aaaa000024131756940729193914d820aaaa0000241317abbbe79542da976d',
      None ],

  #
  # This block represents a DAT audio block. (It is good)
  #
    [ '01f956aa02fce46001fde71b00fd923e03fcb4f301fee8b902fc6c5003fe3104' \
      '0200d33503fdb17400ffcf5a0301e634fefdbfa601ff56d892ec1580823a9df2',
      None ],
  
  #
  # Another DAT audio block (good).
  #
    [ 'ff0109b00001cc2dfe023adb01fc3c2202002e2a00ffd6da03fb976efaffce24' \
      '03023d4cfe03bb8efc002732fc01320d0101932c05fec2443d2ac3f771da2db6',
      None ],

  #
  # An erasure test from the block above.
  #
    [ 'ff0109b00001cc2dfe023adb01fc3c2202002e2a00ffd6da03fb976efaffce24' \
      '03023d4cfe03bb8efc002732fc01320d0101932c05fec2443d??c3??71??2d??',
      #
      'ff0109b00001cc2dfe023adb01fc3c2202002e2a00ffd6da03fb976efaffce24' \
      '03023d4cfe03bb8efc002732fc01320d0101932c05fec2443d2ac3f771da2db6' ],

  #
  # Uncorrectable. (Three errors)
  #
    [ 'ff0109b00001cc2dfe023adb01fc3c2202002e2a00ffd6da03fb976efaffce24' \
      '03023d4cfe03bb8efc002732fc01320d0101932c05fec2443d2ac30071002d00',
      #                                                      ^^  ^^  ^^
      'ffXX09XX00XXccXXfeXX3aXX01XX3cXX02XX2eXX00XXd6XX03XX97XXfaXXceXX' \
      '03XX3dXXfeXXbbXXfcXX27XXfcXX32XX01XX93XX05XXc2XX3dXXc3XX71XX2dXX' ],
  #
  # Uncorrectable (six errors)
  #
    #    vv  vv      vv      vv  vv                          vv
    [ 'ff1109c20001cc20fe023aff010c3c2202002e2a00ffd6da03fb976dfaffce24' \
      '03023d4cfe03bb8efc002732fc01320d0101932c05fec2443d2ac30071002d00',
      #
      'ffXX09XX00XXccXXfeXX3aXX01XX3cXX02XX2eXX00XXd6XX03XX97XXfaXXceXX' \
      '03XX3dXXfeXXbbXXfcXX27XXfcXX32XX01XX93XX05XXc2XX3dXXc3XX71XX2dXX' ],
  #
  # This vector used to escape detection in older versions of my
  # algorithm.
  #
    [ #                            
      'b10043005500af00fc00580029009f00d50073004800bd001a002100e5001700'
      '0a00420038009700af00770020000200a8009d00a5006a00670000000e00f100',
      #
      'XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00'
      'XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00XX00' ],
  #
  # This real-world vector supposedly corrects ok in C1 but doesn't
  # check out under C2. Here's its transformation in C1.
  #
    [ #      vv
      '078705efbdb68cfbebb9931479563c31d7e0f25ebfd615c8d21bacb5d7fb68b5'
      'ba70c26bdd084e571082df8d7c593dfe2e4b3b2b4f008f00d505f683944d47a8',
      #      vv
      '078705afbdb68cfbebb9931479563c31d7e0f25ebfd615c8d21bacb5d7fb68b5'
      'ba70c26bdd084e571082df8d7c593dfe2e4b3b2b4f008f00d505f683944d47a8' ],
  #
  # Here's the vector above, but after C2 correction has been applied.
  # Strangely enough, it checks out perfectly when the C2 correction gets
  # back-ported. In all, the even stream of this block pair had seven
  # errors.
  #
    [# vv    C1        vv                                          vv
      '1f8705afbdb68cfbcfb9931479563c31d7e0f25ebfd615c8d21bacb5d7fb6bb5'
      'ba70226bdd084e571082078d7c593dfe2e4b3b2b4f008800d505f683944d47a8',
      #    ^^              ^^                      ^^
      None ],
  ]

  #
  # Real-world deinverleaved vectors.
  #
  vector_tests = [
  
  #
  # Uncorrectable
  #
     [ 'b6af1bbab70041b1b1b1b1b11b1600aeb6ae222ea300ed223d3db1b116afae36',
        -1 ],
  
  #
  # Two errors (12, 22)
  #     31                                                             0 
  #      |                vv                  vv                       |  
     [ 'ff0efdae00d500f9ffd60392fd5600ea0128ff5ffdef0132ff980000e5010387',
       '??0efdae00d500f9ffd60392fd5600ea0128ff5ffdef0132ff980000e50103??' ],
  
  #
  # Two errors (15, 28)
  #           vv                        vv 
     [ 'f186d8705d3c191ff63584d029c9ef27f0f3b913fa1390835f7c025d4d6a298a',
       'f186d8??5d3c191ff63584d029c9ef27??f3b913fa1390835f7c025d4d6a298a' ],
      

  #
  # Four erasures                                               vvvvvvvv
     [ '8720462678e3e31165435f6569cf45ccd6219ab7dcde4c0cddf66b76????????',
       '8720462678e3e31165435f6569cf45ccd6219ab7dcde4c0cddf66b76499b0aeb' ],
  ]

  # (Uncorrectable)
  vp_ex9 = '002e00fa2900ae00fa24222223f766ae223eddc99d22fadd3ddd041d0300d689'
  
  #
  # Generate the C1 parity check matrix.
  #
  Hp = generate_Hp()
  
  #
  # Test block pairs.
  #
  for block_pair_test in block_pair_tests:
    input = block_pair_test[0]
    expect = block_pair_test[1]

    #
    # Convert the hex strings into byte arrays and decipher the erasure
    # locations.
    #
    p = [ input[i:i+2] for i in range(0,128,2) ]
    b = [ int((byte, '00')[int(byte == '??')],16) for byte in p ]
    erasures = [ item[0] 
                 for item in filter(lambda x: x[1] == '??', enumerate(p)) ]

    r = c1_correct_block_pair(b, erasures, Hp)
    
    #
    # Turn the returned array back into a result string. Uncorrectable
    # bytes come back as 257. We want them to appear as XX in the result
    # string.
    #
    c = [ ('%02x' % x, 'XX')[int(x > 255)] for x in r ]
    result = ''.join(c)

    if expect is None:
      assert(result == input)
    else:
      assert(result == expect)
      
def c1_correct_block_pair(p, erasures, Hp):
  '''Detect and correct the errors in the bytes in the given 64-byte array
  under the assumption that it represents two 32-byte blocks that are part
  of a DAT block pair, using the provided check matrix for the code.'''
  #
  # Split the caller's array into its two blocks.
  #
  a = p[0:32]
  b = p[32:64]

  #  
  # Deinterleave the bytes into two C1 arrays.
  #
  vp1, vp2 = Vps_for_block_pair(a, b)

  #
  # Likewise deinterleave the erasures.
  #
  even_erasures = filter(lambda x: (x & 1) == 0, erasures)
  odd_erasures = filter(lambda x: (x & 1) == 1, erasures)
  vp1_erasures = [ x / 2 for x in even_erasures ]
  vp2_erasures = [ x / 2 for x in odd_erasures ]

  #
  # Correct both arrays
  #
  cp1 = c1_correct_codeword(vp1, Hp, vp1_erasures)
  cp2 = c1_correct_codeword(vp2, Hp, vp2_erasures)

  if cp1 is None:
    cp1 = [256] * 32
  if cp2 is None:
    cp2 = [256] * 32

  (c,d) = block_pair_from_Vps(cp1, cp2)
  
  return c + d
  
def c1_correct_codeword(vp, Hp, erasures):
  '''Detect and correct the errors in the given codeword vector (bytes)
  using the provided check matrix.'''
  #
  # Compute the syndrome values.
  #
  syndrome = check_Vp_Hp(vp, Hp)

  #
  # If the syndrome is zero then there's no error!
  #
  if is_zero(syndrome):
    return vp

  #
  # Determine how many bytes are in this code word.
  #
  length = len(vp)

  #
  # Incorporate any erasure information.
  #
  chi = [ pow(primitive, length - 1 - c) for c in erasures ]

  #
  # Use the Extended Euclidean algorithm to get the error locator
  # and error magnitude polynomials from the syndrome.
  #
  (sigma, omega, ok) = sy_compute_sigma(syndrome, chi)
  
  #
  # If the algorithm terminated with a residual then the error
  # is not correctable.
  #
  if not ok:
    return None

  #
  # Find the roots of the error locator polynomial; these
  # roots determine where the errors lie.
  #
  roots = find_primitive_roots(sigma, 32)

  if not roots:
    #
    # No roots found within the range given.
    # This means there's an uncorrectable error.
    #
    return None

  for i in roots:
    error = compute_error_at_location(sigma, omega, i)
    location = length - 1 - i
    vp[location] ^= error

    #
    # Update the syndrome with the correction.
    #
    for j in range(len(syndrome)):
      syndrome[j] ^= multiply(Hp[j][location], error)

  #
  # If everything is ok then the new syndrome should be
  # zero.
  #
  if is_zero(syndrome):
    return vp
  else:
    return None

def do():
  import pretty

  #print pretty.pretty_c_print(generate_log_alpha_table(), 8, 80)
  #print pretty.pretty_c_print(generate_alpha_powers(), 8, 80)
  #print pretty.pretty_c_print(generate_inverses(), 8, 80)
  #print pretty.pretty_c_print(generate_C1_inverse_magnitudes(), 8, 80)
  #print pretty.pretty_c_print(generate_Hq()[0], 8, 80)
  #print pretty.pretty_c_print(generate_Hq()[1], 8, 80)
  #print pretty.pretty_c_print(generate_Hq()[2], 8, 80)
  #print pretty.pretty_c_print(generate_Hq()[3], 8, 80)
  #print pretty.pretty_c_print(generate_Hq()[4], 8, 80)
  #print pretty.pretty_c_print(generate_Hq()[5], 8, 80)
  print pretty.pretty_c_print(generate_Hi()[0], 8, 80)
  print pretty.pretty_c_print(generate_Hi()[1], 8, 80)

def test():
  #assert(splat(1) == [1])
  #assert(splat(0) == [0])
  #assert(splat(6) == [1,1,0])
  #assert(multiply(1,1) == 1)
  #assert(multiply(1,2) == 2)
  #assert(multiply(0x80, 2) == (mod_poly ^ 0x100))
  #assert(pow(1,1) == 1)
  #assert(pow(2,1) == 2)
  #assert(pow(2,2) == 4)
  #assert(pow(2,3) == 8)
  #assert(pow(2,8) == (mod_poly ^ 0x100))
  assert(poly_multiply([0], [0]) == [0])
  assert(poly_multiply([1], [0]) == [0])
  assert(poly_multiply([1], [1]) == [1])
  assert(poly_multiply([1], [0,1]) == [0,1])
  assert(poly_multiply([0,1], [0,1]) == [0,0,1])
  assert(poly_divide_by_x_plus_y([0,1], 0) == ([1],0,False))
  test_c1()
  #do()
  
if __name__ == '__main__':
  print "Testing module"
  test()
  print "OK"

