@ ******** ASMZoomBlit(unsigned char *src, unsigned char *dst4, int nbx, int nby, int height2, int zoom) ********

	.ALIGN
	.GLOBAL ASMZoomBlit
	.TYPE   ASMZoomBlit, function
	.CODE 32

@r0 = src
@r1 = dst4
@r2 = nbx
@r3 = nby

@r6 = height2
@r7 = zoom
@r8 = tmp
@r9 = tmp
@r10 = tmpnby
@r11 = src4

ASMZoomBlit:

	sub		sp,sp,#8
        stmfd		r13!,{r6-r11}
        ldr		r6,[r13,#32]
        ldr		r7,[r13,#36]

_bx:
	MUL		r8,r7,r2		@Calcul de src4
	MOV		r9,r8,LSR #10
	MLA		r11,r9,r6,r0
	MOV		r10,r3
	MUL		r8,r7,r10

_by:
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]
	BMI		_fincol
	LDRB		r9,[r11,+r8,LSR #10]
	SUBS		r10,r10,#1
	MULPL	r8,r7,r10
	STRB		r9,[r1,+r10]

	BPL		_by

_fincol:
	SUB		r1,r1,#240
	SUBS		r2,r2,#1
	BPL		_bx

        ldmfd		r13!,{r6-r11}
	add		sp,sp,#8
	bx		lr



@ ******** ASMZoomTransBlit(unsigned char *src, unsigned char *dst4, int nbx, int nby, int height2, int zoom, int trans) ********

	.ALIGN
	.GLOBAL ASMZoomTransBlit
	.TYPE   ASMZoomTransBlit, function
	.CODE 32

@r0 = src
@r1 = dst4
@r2 = nbx
@r3 = nby

@r5 = trans
@r6 = height2
@r7 = zoom
@r8 = tmp
@r9 = tmp
@r10 = tmpnby
@r11 = src4

ASMZoomTransBlit:

	sub		sp,sp,#12
        stmfd		r13!,{r5-r11}
        LDR		r6,[r13,#40]
        LDR		r7,[r13,#44]
        LDR		r5,[r13,#48]

_bx2:
	MUL		r8,r7,r2
	MOV		r9,r8,LSR #10
	MLA		r11,r9,r6,r0
	MUL		r8,r7,r3
	SUBS		r10,r3,#1
	BMI		_fincol2

_by2:
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol2
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1

	BPL		_by2

_fincol2:					@Dernier pixel
	LDRB		r9,[r11,+r8,LSR #10]
	SUB		r1,r1,#240
	TEQ		r5,r9
	STRNEB	r9,[r1,#239]

	SUBS		r2,r2,#1
	BPL		_bx2

        ldmfd		r13!,{r5-r11}
	add		sp,sp,#12
	bx		lr



@ ******** ASMZoomTransInvBlit(unsigned char *src, unsigned char *dst4, int nbx, int nby, int height2, int zoom, int trans) ********

	.ALIGN
	.GLOBAL ASMZoomTransInvBlit
	.TYPE   ASMZoomTransInvBlit, function
	.CODE 32

@r0 = src
@r1 = dst4
@r2 = nbx
@r3 = nby

@r5 = trans
@r6 = height2
@r7 = zoom
@r8 = tmp
@r9 = tmp
@r10 = tmpnby
@r11 = src4

ASMZoomTransInvBlit:

	sub		sp,sp,#12
        stmfd		r13!,{r5-r11}
        LDR		r6,[r13,#40]
        LDR		r7,[r13,#44]
        LDR		r5,[r13,#48]

_bx3:
	MUL		r8,r7,r2
	MOV		r9,r8,LSR #10
	MLA		r11,r9,r6,r0
	MUL		r8,r7,r3
	SUBS		r10,r3,#1
	BMI		_fincol3

_by3:
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol3
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r9,[r1,+r10]
	SUBS		r10,r10,#1

	BPL		_by3

_fincol3:					@Dernier pixel
	LDRB		r9,[r11,+r8,LSR #10]
	ADD		r1,r1,#240
	TEQ		r5,r9
	STRNEB	r9,[r1,#-239]

	SUBS		r2,r2,#1
	BPL		_bx3

        ldmfd		r13!,{r5-r11}
	add		sp,sp,#12
	bx		lr



@ ******** ASMZoomSolidBlit(unsigned char *src, unsigned char *dst4, int nbx, int nby, int height2, int zoom, int trans, int coul) ********

	.ALIGN
	.GLOBAL ASMZoomSolidBlit
	.TYPE   ASMZoomSolidBlit, function
	.CODE 32

@r0 = src
@r1 = dst4
@r2 = nbx
@r3 = nby

@r4 = coul
@r5 = trans
@r6 = height2
@r7 = zoom
@r8 = tmp
@r9 = tmp
@r10	= tmpnby
@r11 = src4

ASMZoomSolidBlit:

	sub		sp,sp,#16
        stmfd		r13!,{r4-r11}
        LDR		r6,[r13,#48]
        LDR		r7,[r13,#52]
        LDR		r5,[r13,#56]
	LDR		r4,[r13,#60]

_bx4:
	MUL		r8,r7,r2
	MOV		r9,r8,LSR #10
	MLA		r11,r9,r6,r0
	MUL		r8,r7,r3
	SUBS		r10,r3,#1
	BMI		_fincol4

_by4:
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol4
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1

	BPL		_by4

_fincol4:					@Dernier pixel
	LDRB		r9,[r11,+r8,LSR #10]
	SUB		r1,r1,#240
	TEQ		r5,r9
	STRNEB	r4,[r1,#239]

	SUBS		r2,r2,#1
	BPL		_bx4

        ldmfd		r13!,{r4-r11}
	add		sp,sp,#16
	bx		lr



@ ******** ASMZoomSolidInvBlit(unsigned char *src, unsigned char *dst4, int nbx, int nby, int height2, int zoom, int trans, int coul) ********

	.ALIGN
	.GLOBAL ASMZoomSolidInvBlit
	.TYPE   ASMZoomSolidInvBlit, function
	.CODE 32

@r0 = src
@r1 = dst4
@r2 = nbx
@r3 = nby

@r4 = coul
@r5 = trans
@r6 = height2
@r7 = zoom
@r8 = tmp
@r9 = tmp
@r10	= tmpnby
@r11 = src4

ASMZoomSolidInvBlit:

	sub		sp,sp,#16
        stmfd		r13!,{r4-r11}
        LDR		r6,[r13,#48]
        LDR		r7,[r13,#52]
        LDR		r5,[r13,#56]
	LDR		r4,[r13,#60]

_bx5:
	MUL		r8,r7,r2
	MOV		r9,r8,LSR #10
	MLA		r11,r9,r6,r0
	MUL		r8,r7,r3
	SUBS		r10,r3,#1
	BMI		_fincol5

_by5:
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1
	BMI		_fincol5
	LDRB		r9,[r11,+r8,LSR #10]
	MULPL	r8,r7,r10
	TEQ		r5,r9
	STRNEB	r4,[r1,+r10]
	SUBS		r10,r10,#1

	BPL		_by5

_fincol5:					@Dernier pixel
	LDRB		r9,[r11,+r8,LSR #10]
	ADD		r1,r1,#240
	TEQ		r5,r9
	STRNEB	r4,[r1,#-239]

	SUBS		r2,r2,#1
	BPL		_bx5

        ldmfd		r13!,{r4-r11}
	add		sp,sp,#16
	bx		lr



@ ******** ASMFastTransBlit(unsigned char *src4, unsigned char *dst4, int nbx, int nby, int height2, int trans) ********

	.ALIGN
	.GLOBAL ASMFastTransBlit
	.TYPE   ASMFastTransBlit, function
	.CODE 32

@r0 = src4
@r1 = dst4
@r2 = nbx
@r3 = nby

@r4 = height2
@r5 = trans
@r6 = tmp
@r7 = tmpnby
@r8 = tmp2

ASMFastTransBlit:

	sub		sp,sp,#8
        stmfd		r13!,{r4-r8}
        ldr		r4,[r13,#28]
        ldr		r5,[r13,#32]

_bx7:
	ldrb		r8,[r0,+r3]		@lecture 1er pixel
	subs		r7,r3,#1
	bmi		_sauty2

_by7:
	LDRB		r6,[r0,+r7]
	TEQ		r8,r5
	STRNEB	r8,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2b
	LDRB		r8,[r0,+r7]
	TEQ		r6,r5
	STRNEB	r6,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2
	LDRB		r6,[r0,+r7]
	TEQ		r8,r5
	STRNEB	r8,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2b
	LDRB		r8,[r0,+r7]
	TEQ		r6,r5
	STRNEB	r6,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2
	LDRB		r6,[r0,+r7]
	TEQ		r8,r5
	STRNEB	r8,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2b
	LDRB		r8,[r0,+r7]
	TEQ		r6,r5
	STRNEB	r6,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2
	LDRB		r6,[r0,+r7]
	TEQ		r8,r5
	STRNEB	r8,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2b
	LDRB		r8,[r0,+r7]
	TEQ		r6,r5
	STRNEB	r6,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2
	LDRB		r6,[r0,+r7]
	TEQ		r8,r5
	STRNEB	r8,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2b
	LDRB		r8,[r0,+r7]
	TEQ		r6,r5
	STRNEB	r6,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2
	LDRB		r6,[r0,+r7]
	TEQ		r8,r5
	STRNEB	r8,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2b
	LDRB		r8,[r0,+r7]
	TEQ		r6,r5
	STRNEB	r6,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2
	LDRB		r6,[r0,+r7]
	TEQ		r8,r5
	STRNEB	r8,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2b
	LDRB		r8,[r0,+r7]
	TEQ		r6,r5
	STRNEB	r6,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2
	LDRB		r6,[r0,+r7]
	TEQ		r8,r5
	STRNEB	r8,[r1,+r7]
	SUBS		r7,r7,#1
	BMI		_sauty2b
	LDRB		r8,[r0,+r7]
	TEQ		r6,r5
	STRNEB	r6,[r1,+r7]
	SUBS		r7,r7,#1

	BPL		_by7

_sauty2:					@ecriture r8
	TEQ		r8,r5
	STRNEB	r8,[r1,+r7]
	SUB		r0,r0,r4
	SUB		r1,r1,#240
	SUBS		r2,r2,#1
	BPL		_bx7

        ldmfd		r13!,{r4-r8}
	add		sp,sp,#8
	bx		lr

_sauty2b:					@ecriture r6
	TEQ		r6,r5
	STRNEB	r6,[r1,+r7]
	SUB		r0,r0,r4
	SUB		r1,r1,#240
	SUBS		r2,r2,#1
	BPL		_bx7

        ldmfd		r13!,{r4-r8}
	add		sp,sp,#8
	bx		lr



@ ******** ASMFastSolidBlit(unsigned char *src4, unsigned char *dst4, int nbx, int nby, int height2, int trans, int coul) ********

	.ALIGN
	.GLOBAL ASMFastSolidBlit
	.TYPE   ASMFastSolidBlit, function
	.CODE 32

@r0 = src4
@r1 = dst4
@r2 = nbx
@r3 = nby

@r4 = height2
@r5 = trans
@r6 = coul
@r7 = tmp
@r8 = tmpnby
@r9 = tmp2

ASMFastSolidBlit:

	sub		sp,sp,#12
        stmfd		r13!,{r4-r9}
        ldr		r4,[r13,#36]
        ldr		r5,[r13,#40]
        ldr		r6,[r13,#44]

_bx8:
	ldrb		r9,[r0,+r3]		@lecture 1er pixel
	subs		r8,r3,#1
	bmi		_sauty2

_by8:
	LDRB		r7,[r0,+r8]
	TEQ		r9,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3b
	LDRB		r9,[r0,+r8]
	TEQ		r7,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3
	LDRB		r7,[r0,+r8]
	TEQ		r9,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3b
	LDRB		r9,[r0,+r8]
	TEQ		r7,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3
	LDRB		r7,[r0,+r8]
	TEQ		r9,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3b
	LDRB		r9,[r0,+r8]
	TEQ		r7,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3
	LDRB		r7,[r0,+r8]
	TEQ		r9,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3b
	LDRB		r9,[r0,+r8]
	TEQ		r7,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3
	LDRB		r7,[r0,+r8]
	TEQ		r9,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3b
	LDRB		r9,[r0,+r8]
	TEQ		r7,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3
	LDRB		r7,[r0,+r8]
	TEQ		r9,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3b
	LDRB		r9,[r0,+r8]
	TEQ		r7,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3
	LDRB		r7,[r0,+r8]
	TEQ		r9,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3b
	LDRB		r9,[r0,+r8]
	TEQ		r7,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3
	LDRB		r7,[r0,+r8]
	TEQ		r9,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1
	BMI		_sauty3b
	LDRB		r9,[r0,+r8]
	TEQ		r7,r5
	STRNEB	r6,[r1,+r8]
	SUBS		r8,r8,#1

	BPL		_by8

_sauty3:					@ecriture pour r9
	TEQ		r9,r5
	STRNEB	r6,[r1,+r8]
	SUB		r0,r0,r4
	SUB		r1,r1,#240
	SUBS		r2,r2,#1
	BPL		_bx8

        ldmfd		r13!,{r4-r9}
	add		sp,sp,#12
	bx		lr

_sauty3b:					@ecriture pour r7
	TEQ		r7,r5
	STRNEB	r6,[r1,+r8]
	SUB		r0,r0,r4
	SUB		r1,r1,#240
	SUBS		r2,r2,#1
	BPL		_bx8

        ldmfd		r13!,{r4-r9}
	add		sp,sp,#12
	bx		lr



@ ******** ASMSaveBitmap(unsigned char *src4, unsigned char *dst, int nbx, int nby, int height2) ********

	.ALIGN
	.GLOBAL ASMSaveBitmap
	.TYPE   ASMSaveBitmap, function
	.CODE 32

@r0 = src4
@r1 = dst + 1
@r2 = nbx
@r3 = nby

@r7 = height2
@r8 = tmp
@r9 = tmpnby
@r10 = dst4

ASMSaveBitmap:

	sub		sp,sp,#4
        stmfd		r13!,{r7-r10}
        LDR		r7,[r13,#20]

_bx6:
	MLA		r10,r2,r7,r1
	MOV		r9,r3

_by6:
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]
	BMI		_fincol6
	LDRB		r8,[r0,+r9]
	SUBS		r9,r9,#1
	STRB		r8,[r10,+r9]

	BPL		_by6

_fincol6:
	SUB		r0,r0,#240
	SUBS		r2,r2,#1
	BPL		_bx6

        ldmfd		r13!,{r7-r10}
	add		sp,sp,#4
	bx		lr



@ ******** ASMMix(unsigned short *buf, s_mix *pmix, int len) ********

	.ALIGN
	.GLOBAL ASMMix
	.TYPE   ASMMix, function
	.CODE 32

@r0 = buf
@r1 = pmix
@r2 = len

@r3 = tmp
@r6 = tmpi
@r7 = #32768
@r8 = tmp
@r9 = tmp
@r10 = tmp
@r11 = sample

@LDMDB - pre-decrement
@LDMIA - post-increment

ASMMix:

        stmfd		sp!,{r6-r11}
	MOV		r7,#32768

bouclemix:
	LDMIA	r1,{r8-r9}		@canal 0
	MOV		r6,#14
	ADD		r3,r1,#16
	CMP		r8,r9
	BEQ		sautboucle0	@Le son boucle peut-être
	LDRMIH	r9,[r8],#2
	STRMI	r8,[r1]
	SUBMI	r11,r9,#32768
	MOVPL	r11,#0

bouclevoix:
	LDMIA	r3,{r8-r9}		@autres canaux
	ADD		r3,r3,#16
	CMP		r8,r9
	BEQ		sautbouclen	@Le son boucle peut-être
	LDRMIH	r9,[r8],#2
	STRMI	r8,[r3,#-16]
	SUBMI	r9,r9,#32768
	ADDMI	r11,r11,r9

	SUBS		r6,r6,#1
	BPL		bouclevoix

sautbouclevoix:
	ADD		r8,r11,#262144
	RSB		r11,r7,r8,LSR #2
	CMP		r11,#65536		@Saturation
	MVNPL	r11,#0
	CMPMI	r11,#0
	MOVMI	r11,#0
	STRH		r11,[r0],#2
	SUBS		r2,r2,#1
	BNE		bouclemix

        ldmfd		sp!,{r6-r11}
	bx		lr

sautboucle0:
	LDMDB	r3,{r9-r10}		@On regarde s'il faut répéter
	SUBS		r9,r9,#1
	STRPL	r10,[r1]		@On boucle
	STRPL	r9,[r1,#8]
	BPL		bouclemix
	ADD		r8,r8,#2		@On marque le son comme fini
	STR		r8,[r1]
	MOV		r11,#0
	B		bouclevoix

sautbouclen:
	LDMDB	r3,{r9-r10}		@On regarde s'il faut répéter
	SUBS		r9,r9,#1
	STRPL	r10,[r3,#-16]	@On boucle
	STRPL	r9,[r3,#-8]
	SUBPL	r3,r3,#16
	BPL		bouclevoix
	ADD		r8,r8,#2		@On marque le son comme fini
	STR		r8,[r3,#-16]

	SUBS		r6,r6,#1
	BPL		bouclevoix
	B		sautbouclevoix



@ ******** ASMFastClear(unsigned char *dst4, int nbx, int nby) ********

	.ALIGN
	.GLOBAL ASMFastClear
	.TYPE   ASMFastClear, function
	.CODE 32

@r0 = dst4
@r1 = nbx
@r2 = nby

@r3 = #0
@r4 = tmpnby

@optimisé pour h=20

ASMFastClear:

	str		r4,[sp,#-4]!
	MOV		r3,#0

_bx9:
	MOV		r4,r2

_by9:
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1
	BMI		_sauty4
	STRB		r3,[r0,+r4]
	SUBS		r4,r4,#1

	BPL		_by9

_sauty4:
	SUB		r0,r0,#240
	SUBS		r1,r1,#1
	BPL		_bx9

	ldr		r4,[sp],#4
	bx		lr
