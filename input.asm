.data
myVar: .word 10, 20
myStr: .asciiz "Hello"

.text
main:
    add x1, x2, x3
    addi x4, x1, 5
    beq x1, x2, main
    jal x0, main
