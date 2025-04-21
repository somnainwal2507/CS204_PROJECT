#Name: Nachiket Avachat
#Roll Number: 2023CSB1106
#Question Number: 3


#I have assume that each element is a word and not a byte
#For byte, we will store as .byte and in code we will give the offset of 1 each time instead of 4
.data
op_or_unop: .word 1
n: .word 5
arr: .word 5, 4,3,2,1

.text 
lui x5 0x10000
lw x7 0(x5)  #op or unop at x7
addi x5 x5 4
lw x8 0(x5)   #n at x8
addi x5 x5 4   #base pointerof array at x5
lui x6 0x10000
addi x6 x6 0x500#base pointer of output array is x6
add x9 x5 x0  #temp pointer
addi x10 x8 -1  #value of n-1
add x11 x0 x0 #iterator

#I have implemented in place bubble sort.
#So we will first copy the original array to the desired output location
store:
    bge x11 x8 l1
    lw x14 0(x9)
    sw x14 0(x6)
    addi x11 x11 1
    addi x9 x9 4
    addi x6 x6 4
    beq x0 x0 store 
l1:
lui x6 0x10000
addi x6 x6 0x500 #base pointer of output array is x6
add x9 x6 x0 
beq x7 x0 unoptimized
addi x20 x0 1
beq x7 x20 optimized
beq x0 x0 exit  #if x7 is neither 0 nor 1 then nothing will happen



swap1:
    sw x14 4(x9)
    sw x15 0(x9)
    beq x0 x0 for2_cont
    

unoptimized:
add x11 x0 x0  #iterator for loop1
for1:
    bge x11 x8 exit
    add x12 x0 x0 #iterator for loop2
    add x9 x6 x0
    for2:
        bge x12 x10 loop2_end
        lw x14 0(x9)
        lw x15 4(x9)
        blt x15 x14 swap1
    for2_cont:
        addi x12 x12 1
        addi x9 x9 4
        beq x0 x0 for2
loop2_end:
    addi x11 x11 1
    beq x0 x0 for1
        
optimized:
    add x11 x0 x0  #iterator for loop1
for_1:
    bge x11 x8 exit
    add x12 x0 x0 #iterator for loop2
    add x9 x6 x0
    add x22 x0 x0 #flag
    sub x23 x10 x11 #n-i-1
    for_2:
        bge x12 x23 lp2_end
        beq x12 x10 lp2_end
        lw x14 0(x9)
        lw x15 4(x9)
        blt x15 x14 swap2
    for_2_cont:
        addi x12 x12 1
        addi x9 x9 4
        beq x0 x0 for_2
lp2_end:
    addi x11 x11 1 
    beq x22 x0 exit
    beq x0 x0 for_1
    
swap2:
    addi x22 x0 1  #set flag to 1
    sw x14 4(x9)
    sw x15 0(x9)
    beq x0 x0 for_2_cont


exit:

