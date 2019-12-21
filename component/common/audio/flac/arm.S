;/*   The following is an assembler optimised version of the LPC filtering
; *   routines needed for FLAC decoding. It is optimised for use with ARM 
; *   processors.
; *   All LPC filtering up to order 9 is done in specially optimised unrolled
; *   loops, while every order above this is handled by a slower default routine.*/
#if   defined ( __CC_ARM )     /* Keil */
    #define CODESECT AREA     ||.text||, CODE, READONLY, ALIGN=2
    #define LABEL
#elif defined ( __IASMARM__ )  /* IAR */
    #define CODESECT SECTION `.text`:CODE
    #define PROC
    #define LABEL
    #define ENDP
    #define EXPORT PUBLIC
#elif defined ( __CSMC__ )	   /* Cosmic */
	#define	CODESECT	switch .text
	#define THUMB
	#define EXPORT	xdef
	#define PROC	:
	#define LABEL	:
	#define ENDP
	#define arm_bitreversal_32 _arm_bitreversal_32
#elif defined ( __TI_ARM__ )   /* TI ARM */
    #define THUMB    .thumb
    #define CODESECT .text
    #define EXPORT   .global
    #define PROC     : .asmfunc
    #define LABEL    :
    #define ENDP     .endasmfunc
    #define END
#elif defined ( __GNUC__ )     /* GCC */
    #define THUMB .thumb
    #define CODESECT .section .text
    #define EXPORT .global
    #define PROC :
    #define LABEL :
    #define ENDP
    #define END

    .syntax unified
#endif

	.text
	CODESECT
	.align 3
	THUMB
	.thumb_func
	EXPORT  lpc_decode_arm
	.type    lpc_decode_arm, function
lpc_decode_arm PROC
    stmdb sp!, { r4-r11, lr }
    ldr r4, [sp, #36]
    ;/* r0 = blocksize, r1 = qlevel, r2 = pred_order */
    ;/*   r3 = data, r4 = coeffs */
     
    ;/* the data pointer always lags behind history pointer by 'pred_order' */
    ;/*   samples. since we have one loop for each order, we can hard code this */
    ;/*  and free a register by not saving data pointer.  */

    sub r3, r3, r2, lsl #2    ;/* r3 = history */
    cmp r0, #0                ;/* no samples to process */
    beq.w exit
    cmp r2, #9                ;/* check if order is too high for unrolled loops */
	bgt.w default
	ldr r5, =jumptable1   ;/* jump to our unrolled decode loop if it exists */
	add r5, r5, r2, lsl #2
	ldr r2, [r5]
	bx  r2

jumptable1 LABEL
    .word exit                   
    .word order1
    .word order2
    .word order3
    .word order4
    .word order5
    .word order6
    .word order7
    .word order8
	.word order9

;/* last jump table entry coincides with target, so leave it out */
order9 LABEL
    ldmia r4, { r5-r12, r14 } ;/* fetch coefs */
loop9 LABEL
    ldr r4, [r3], #4          ;/* load first history sample */
    mul r2, r4, r14           ;/* multiply with last coef */
    ldr r4, [r3], #4          ;/* rinse and repeat while accumulating sum in r2 */
    mla r2, r4, r12, r2
    ldr r4, [r3], #4
    mla r2, r4, r11, r2
    ldr r4, [r3], #4
    mla r2, r4, r10, r2
    ldr r4, [r3], #4
    mla r2, r4, r9, r2
    ldr r4, [r3], #4
    mla r2, r4, r8, r2
    ldr r4, [r3], #4
    mla r2, r4, r7, r2
    ldr r4, [r3], #4
    mla r2, r4, r6, r2
    ldr r4, [r3], #4
    mla r2, r4, r5, r2
    ldr r4, [r3]              ;/* r4 = residual */
	asr r2, r1
	add r2, r4, r2 ;/* shift sum by qlevel bits and add residual */ 
    str r2, [r3], #-8*4       ;/* save result and wrap history pointer back */
    subs r0, r0, #1           ;/* check if we're done */
    bne loop9                ;/* nope, jump back */
    b exit
    
order8 LABEL
    ldmia r4, { r5-r12 }
loop8 LABEL
    ;/* we have more registers to spare here, so start block reading */
    ldmia r3!, { r4, r14 }
    mul r2, r4, r12
    mla r2, r14, r11, r2
    ldmia r3!, { r4, r14 }
    mla r2, r4, r10, r2
    mla r2, r14, r9, r2
    ldmia r3!, { r4, r14 }
    mla r2, r4, r8, r2
    mla r2, r14, r7, r2
    ldmia r3!, { r4, r14 }
    mla r2, r4, r6, r2
    mla r2, r14, r5, r2
    ldr r4, [r3]
	asr r2, r1
	add r2, r4, r2
    str r2, [r3], #-7*4
    subs r0, r0, #1
    bne loop8
    b exit

order7 LABEL
    ldmia r4, { r5-r11 }
loop7 LABEL
    ldmia r3!, { r4, r12, r14 }
    mul r2, r4, r11
    mla r2, r12, r10, r2
    mla r2, r14, r9, r2
    ldmia r3!, { r4, r12, r14 }
    mla r2, r4, r8, r2
    mla r2, r12, r7, r2
    mla r2, r14, r6, r2
    ldr r4, [r3], #4
    mla r2, r4, r5, r2
    ldr r4, [r3]
	asr r2, r1
	add r2, r4, r2	
    str r2, [r3], #-6*4
    subs r0, r0, #1
    bne loop7
    b exit

order6 LABEL
    ldmia r4, { r5-r10 }
loop6 LABEL
    ldmia r3!, { r4, r11-r12, r14 }
    mul r2, r4, r10
    mla r2, r11, r9, r2
    mla r2, r12, r8, r2
    mla r2, r14, r7, r2
    ldmia r3!, { r4, r11 }
    mla r2, r4, r6, r2
    mla r2, r11, r5, r2
    ldr r4, [r3]
	asr r2, r1
	add r2, r4, r2	
    str r2, [r3], #-5*4
    subs r0, r0, #1
    bne loop6
    b exit

order5 LABEL
    ldmia r4, { r5-r9 }
loop5 LABEL
    ldmia r3!, { r4, r10-r12, r14 }
    mul r2, r4, r9
    mla r2, r10, r8, r2
    mla r2, r11, r7, r2
    mla r2, r12, r6, r2
    mla r2, r14, r5, r2
    ldr r4, [r3]
	asr r2, r1
	add r2, r4, r2	
    str r2, [r3], #-4*4
    subs r0, r0, #1
    bne loop5
    b exit

order4 LABEL
    ldmia r4, { r5-r8 }
loop4 LABEL
    ldmia r3!, { r4, r11-r12, r14 }
    mul r2, r4, r8
    mla r2, r11, r7, r2
    mla r2, r12, r6, r2
    mla r2, r14, r5, r2
    ldr r4, [r3]
	asr r2, r1
	add r2, r4, r2	
    str r2, [r3], #-3*4
    subs r0, r0, #1
    bne loop4
    b exit

order3 LABEL
    ldmia r4, { r5-r7 }
loop3 LABEL
    ldmia r3!, { r4, r12, r14 }
    mul r2, r4, r7
    mla r2, r12, r6, r2
    mla r2, r14, r5, r2
    ldr r4, [r3]
	asr r2, r1
	add r2, r4, r2	
    str r2, [r3], #-2*4
    subs r0, r0, #1
    bne loop3
    b exit

order2 LABEL
    ldmia r4, { r5-r6 }
loop2 LABEL
    ldmia r3!, { r4, r14 }
    mul r2, r4, r6
    mla r2, r14, r5, r2
    ldr r4, [r3]
	asr r2, r1
	add r2, r4, r2	
    str r2, [r3], #-1*4
    subs r0, r0, #1
    bne loop2
    b exit

order1 LABEL
    ldr r5, [r4]            ;/* load the one coef we need */
    ldr r4, [r3], #4        ;/* load one history sample, r3 now points to residual */
loop1 LABEL
    mul r2, r4, r5          ;/* multiply coef by history sample */
    ldr r4, [r3]            ;/* load residual */
	asr r2, r1
	add r4, r4, r2	;/* add result to residual */
    str r4, [r3], #4        ;/* place r3 at next residual, we already have  */
    subs r0, r0, #1         ;/* the current sample in r4 for the next iteration */
    bne loop1
    b exit

default LABEL
    ;/* we do the filtering in an unrolled by 4 loop as far as we can, and then */
    ;/*   do the rest by jump table.  */
    add r5, r4, r2, lsl #2   ;/* need to start in the other end of coefs */
    mov r7, r2, lsr #2       ;/* r7 = coefs/4 */
    mov r14, #0              ;/* init accumulator */
dloop1 LABEL
    ldmdb r5!, { r8-r11 }
    ldmia r3!, { r6, r12 }
    mla r14, r6, r11, r14
    mla r14, r12, r10, r14
    ldmia r3!, { r6, r12 }
    mla r14, r6, r9, r14
    mla r14, r12, r8, r14
    subs r7, r7, #1
    bne dloop1

    and r7, r2, #3            ;/* get remaining samples to be filtered */
	
	ldr r2, =jumptable2
	add r2, r2, r7, lsl #2
	ldr r2, [r2]
	bx  r2 ;/* jump into accumulator chain */
jumptable2 LABEL
    .word dsave
    .word oneleft
    .word twoleft
	.word threeleft
threeleft LABEL
    ldr r12, [r5, #-4]!
    ldr r8, [r3], #4
    mla r14, r12, r8, r14  
twoleft LABEL
    ldr r12, [r5, #-4]!
    ldr r8, [r3], #4
    mla r14, r12, r8, r14  
oneleft LABEL
    ldr r12, [r5, #-4]!
    ldr r8, [r3], #4
    mla r14, r12, r8, r14  

dsave LABEL
    ldr r12, [r3]             ;/* load residual */
	asr r14, r1
	add r14, r12, r14	;/* shift sum by qlevel bits and add residual */
    str r14, [r3], #4         ;/* store result */
    sub r3, r3, r2, lsl #2    ;/* and wrap history pointer back to next first pos */
    subs r0, r0, #1           ;/* are we done? */
    bne default              ;/* no, prepare for next sample */

exit LABEL
    ldmia sp!, { r4-r11, pc }

	END
