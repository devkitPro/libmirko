        .GLOBAL         _cpu_speed
        .GLOBAL         gp_setMMU
        .GLOBAL         gp_clearFramebuffer16

_cpu_speed:
        stmdb   r13!, {r0-r12,lr}
        stmdb   sp!, {r0, r1, r2}
        mov             r0, sp
        swi             #0x0d
        add             sp, sp, #12
        ldmia   r13!, {r0-r12,lr}
        bx              lr
        nop

gp_setMMU:
        stmdb   r13!, {r0-r12,lr}
        swi             #0x02
        // Invalidate Cache
        mcr 15, 0, r0, cr7, cr7,0
        ldmia   r13!, {r0-r12,lr}
        bx              lr
        nop

gp_clearFramebuffer16:
        // r0 = framebuffer
        // r1 = color
        orr r1,r1,r1,lsl #16   //hi halfword = lo halfword
        mov r2,#38400
clear:  str  r1,[r0],#4
        subs r2,r2,#1
        bne clear
        bx lr

