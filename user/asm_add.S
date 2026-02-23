.globl main
.globl printf
.globl gets

.section .rodata
msg_parse_error:    .string "ERROR IN PARSING INTEGER: %d\n"
msg_extra_chars:    .string "ERROR: EXTRA CHARACTERS IN INPUT\n"
msg_result:         .string "%d + %d = %d\n"
fmt_input:          .string "|%s|\n"
newline:            .string "\n"

.section .data
.balign 8
buffer:             .space 130

.section .text


my_atoi:
    addi sp, sp, -32
    sd ra, 24(sp)
    sd s0, 16(sp)
    sd s1, 8(sp)
    sd s2, 0(sp)
    addi s0, sp, 32
    
    mv s1, a0
    mv s2, a1

    li t0, 0
    li t1, 0

    lb t2, 0(s1)
    li t3, '-'
    bne t2, t3, atoi_loop

    addi s1, s1, 1
    li t1, 1
    
atoi_loop:
    mv t4, s1
    
atoi_digits:
    lb t2, 0(s1)
    li t3, '0'
    blt t2, t3, atoi_check_end
    li t3, '9'
    bgt t2, t3, atoi_check_end
    
    li t3, 10
    mul t0, t0, t3
    addi t2, t2, -48
    add t0, t0, t2

    addi s1, s1, 1
    j atoi_digits

atoi_check_end:
    beq s1, t4, atoi_error

    beqz t1, atoi_store
    neg t0, t0
    
atoi_store:
    sw t0, 0(s2)
    li a0, 0
    j atoi_exit

atoi_error:
    li a0, -1

atoi_exit:
    ld ra, 24(sp)
    ld s0, 16(sp)
    ld s1, 8(sp)
    ld s2, 0(sp)
    addi sp, sp, 32
    ret


parse_int:
    addi sp, sp, -64
    sd ra, 56(sp)
    sd s0, 48(sp)
    sd s1, 40(sp)
    sd s2, 32(sp)
    sd s3, 24(sp)
    sd s4, 16(sp)
    addi s0, sp, 64
    
    mv s1, a0
    mv s2, a1

    sw zero, 0(s2)

    ld s3, 0(s1)
skip_spaces:
    lb t0, 0(s3)
    li t1, ' '
    bne t0, t1, check_end
    addi s3, s3, 1
    j skip_spaces

check_end:
    lb t0, 0(s3)
    bnez t0, have_number
    li a0, -1
    j parse_int_exit

have_number:
    mv s4, s3
    
find_end:
    lb t0, 0(s3)
    beqz t0, end_found
    li t1, ' '
    beq t0, t1, end_found
    addi s3, s3, 1
    j find_end

end_found:
    beq s3, s4, parse_error

    lb t0, 0(s3)
    sb t0, 4(sp)
    sb zero, 0(s3)

    mv a0, s4
    addi a1, sp, 0
    call my_atoi

    lb t0, 4(sp)
    sb t0, 0(s3)

    bgez a0, store_result
    j parse_int_exit

store_result:
    lw t0, 0(sp)
    sw t0, 0(s2)

    sd s3, 0(s1)

    li a0, 0
    j parse_int_exit

parse_error:
    li a0, -1

parse_int_exit:
    ld ra, 56(sp)
    ld s0, 48(sp)
    ld s1, 40(sp)
    ld s2, 32(sp)
    ld s3, 24(sp)
    ld s4, 16(sp)
    addi sp, sp, 64
    ret


add:
    add a0, a0, a1
    ret


main:
    addi sp, sp, -96
    sd ra, 88(sp)
    sd s0, 80(sp)
    sd s1, 72(sp)
    sd s2, 64(sp)
    sd s3, 56(sp)
    sd s4, 48(sp)
    sd fp, 40(sp)
    addi s0, sp, 96
    
    la a0, buffer
    li a1, 130
    call gets

    la t0, buffer
strip_nl:
    lb t1, 0(t0)
    beqz t1, print_buffer
    li t2, 10
    beq t1, t2, cut_nl
    li t2, 13
    beq t1, t2, cut_nl
    addi t0, t0, 1
    j strip_nl
cut_nl:
    sb zero, 0(t0)

print_buffer:
    la a0, fmt_input
    la a1, buffer
    call printf

    la s2, buffer

    addi a0, sp, 32
    sd s2, 0(a0)
    addi a1, sp, 24
    call parse_int
    mv s3, a0

    bgez s3, parse_second

    la a0, msg_parse_error
    mv a1, s3
    call printf
    mv a0, s3
    j main_exit

parse_second:
    lw s1, 24(sp)

    ld s2, 32(sp)

    addi a0, sp, 32
    sd s2, 0(a0)
    addi a1, sp, 20
    call parse_int
    mv s3, a0

    bgez s3, check_extra

    la a0, msg_parse_error
    mv a1, s3
    call printf
    mv a0, s3
    j main_exit

check_extra:
    lw s4, 20(sp)

    ld s2, 32(sp)
skip_remaining_spaces:
    lb t0, 0(s2)
    li t1, ' '
    bne t0, t1, check_end_of_string
    addi s2, s2, 1
    j skip_remaining_spaces

check_end_of_string:
    lb t0, 0(s2)
    beqz t0, print_result

    la a0, msg_extra_chars
    call printf
    li a0, -1
    j main_exit

print_result:
    mv a0, s1
    mv a1, s4
    call add
    mv t0, a0

    la a0, msg_result
    mv a1, s1
    mv a2, s4
    mv a3, t0
    call printf

    li a0, 0

main_exit:
    ld ra, 88(sp)
    ld s0, 80(sp)
    ld s1, 72(sp)
    ld s2, 64(sp)
    ld s3, 56(sp)
    ld s4, 48(sp)
    ld fp, 40(sp)
    addi sp, sp, 96
    ret
