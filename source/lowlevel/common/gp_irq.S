                .global gp_enableIRQ
                .global gp_disableIRQ
                .global gp_installSWIIRQ
                .global gp_removeSWIIRQ
         

gp_enableIRQ:
                STMDB    r13!,{r0,lr}
                MRS      r0,CPSR
                BIC      r0,r0,#0x80
                MSR      CPSR,r0
                LDMIA    r13!,{r0,pc}
                @MOV      pc,lr

gp_disableIRQ:
                STMDB    r13!,{r0,lr}
                MRS      r0,CPSR
                ORR      r0,r0,#0xc0
                MSR      CPSR,r0
                LDMIA    r13!,{r0,pc}
                @MOV      pc,lr

gp_installSWIIRQ:
                STMDB    r13!,{r14}
                SWI      0x9
                LDMIA    r13!,{pc}

gp_removeSWIIRQ:
                STMDB    r13!,{r14}
                SWI      0xa
                LDMIA    r13!,{pc}

