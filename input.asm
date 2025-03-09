.data
myVar: .word 10, 20
myStr: .asciiz "Hello"

.text
main:
lui x3 0x10000
ld x2, 0, x3
