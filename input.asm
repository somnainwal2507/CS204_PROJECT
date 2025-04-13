# GCD algorithm

.data

A: .word 5
B: .word 15
C: .word 25

.text

lui x5 0x10000 # Base address
lw x31 0(x5) # Store A
lw x30 4(x5) # Store B
lw x20 8(x5) # Store C
GCD:
# If B = 0
beq x0 x30 EXIT

# Store B in tmp register
add x29 x0 x30
# B = A mod B
rem x30 x31 x30
# A = B
add x31 x0 x29
# GCD(B, A mod B)
beq x0 x0 GCD


EXIT: