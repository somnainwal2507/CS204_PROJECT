.text
addi  x1, x0, 2 
loop:
    addi  x1, x1, -1   
    bne   x1, x0, loop   
    addi  x2, x0, 1