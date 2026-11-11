# RV32IMA math study listing; Zig 0.14.1, -Oz -fno-inline, soft-float ILP32.
# Generated from math.c, math_kernel.inc and exp_double.inc.
# Z80 excerpts are documented algorithm/call-site excerpts, not full ROM disassembly.
# -fno-inline preserves teaching boundaries; this is NOT the linked firmware layout.
# a0-a7: integer/pointer arguments; a0/a1 also carry binary64 words.
# s0-s11: callee-saved; ra: return address; sp: 16-byte-aligned stack.
# No F/D instructions: __*sf* / __*df* calls perform software IEEE arithmetic.
# SINGLE rounds at binary32 operations; DOUBLE uses binary64.

	.text
	.attribute	4, 16
	.attribute	5, "rv32i2p1_m2p0_a2p1_zmmul1p0"
	.globl	n32_math                        # -- Begin function n32_math
	.p2align	2

# ======================================================================
# FUNCTION n32_math
# Public checked math entry: type/domain/finite checks precede result publication.
# Z80 LOG excerpt: CALL 0955h; OR A; JP PE,1E4Ah ; argument rejection
# Z80 entries: LOG 0809h, EXP 1439h, SQR 13E7h, SIN 1547h, COS 1541h, TAN 15A8h, ATN 15BDh.
# No identical Z80 dispatcher: native tagged values and status codes replace ROM workspace/errors.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	n32_math,@function
n32_math:                               # @n32_math
.Lfunc_begin0:
# C src/native32/math.c:52: int n32_math(uint8_t token,const N32Value *input,N32Value *result){
# %bb.0:
	addi	sp, sp, -64 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 60(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 56(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 52(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 48(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s3, 44(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s4, 40(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s5, 36(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s3, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.Ltmp0:
# C src/native32/math.c:53: if(input->type==N32_STRING)return N32_TYPE;
	lbu	a1, 8(a1) # RV32: load a byte (unsigned/signed respectively); tagged type/token fields are byte-sized.
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	s2, 3 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a0, 2 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	beq	a1, s2, .LBB0_16 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	mv	s0, a2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
# C src/native32/math.c:54: double real=n32_real(input);N32Value bits=n32_double(real);
	mv	a0, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_real # RV32: call n32_real; arguments in a-registers; preserve any live caller-saved values.
.Ltmp1:
	mv	s5, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s4, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	addi	a0, sp, 24 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a1, s5 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a2, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_double # RV32: call n32_double; arguments in a-registers; preserve any live caller-saved values.
.Ltmp2:
# C src/native32/math.c:55: if((bits.as.wide.high&0x7ff00000)==0x7ff00000)return N32_OVERFLOW;
	lw	a0, 28(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lui	a1, 524032 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	and	a0, a0, a1 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	beq	a0, a1, .LBB0_15 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.2:
	li	a0, 182 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
# C src/native32/math.c:56: if(token==T_EXP&&real>1024)return N32_OVERFLOW;
	bne	s1, a0, .LBB0_4 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.3:
	lui	a3, 264448 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s5 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__gtdf2 # RV32: call __gtdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp3:
	bgtz	a0, .LBB0_15 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
.LBB0_4:
	li	a0, 183 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	s2, 5 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
# C src/native32/math.c:57: if((token==T_LOG&&real<=0)||(token==T_SQR&&real<0))return N32_DOMAIN;
	bne	s1, a0, .LBB0_6 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.5:
	mv	a0, s5 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ledf2 # RV32: call __ledf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp4:
	blez	a0, .LBB0_15 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
.LBB0_6:
	li	a0, 177 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	bne	s1, a0, .LBB0_8 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.7:
	mv	a0, s5 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltdf2 # RV32: call __ltdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp5:
	bltz	a0, .LBB0_15 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
.LBB0_8:
# C src/native32/math.c:59: if(input->type==N32_DOUBLE){double out;int status=double_math(token,real,&out);if(status)return status;answer=n32_double(out);}
	lbu	a0, 8(s3) # RV32: load a byte (unsigned/signed respectively); tagged type/token fields are byte-sized.
	li	a1, 2 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	bne	a0, a1, .LBB0_11 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.9:
	mv	a3, sp # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s5 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a2, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	double_math # RV32: call double_math; arguments in a-registers; preserve any live caller-saved values.
.Ltmp6:
	bnez	a0, .LBB0_14 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.10:
	lw	a2, 4(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	a1, 0(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	a0, sp, 12 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	call	n32_double # RV32: call n32_double; arguments in a-registers; preserve any live caller-saved values.
.Ltmp7:
	j	.LBB0_13 # RV32: unconditional control transfer to a basic block or switch target.
.LBB0_11:
# C src/native32/math.c:60: else{float out;int status=single_math(token,(float)real,&out);if(status)return status;answer=n32_single(out);}
	mv	a0, s5 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__truncdfsf2 # RV32: call __truncdfsf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp8:
	mv	a1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a2, sp # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	single_math # RV32: call single_math; arguments in a-registers; preserve any live caller-saved values.
.Ltmp9:
	bnez	a0, .LBB0_14 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.12:
	lw	a1, 0(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	a0, sp, 12 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	call	n32_single # RV32: call n32_single; arguments in a-registers; preserve any live caller-saved values.
.Ltmp10:
.LBB0_13:
# C src/native32/math.c:63: return n32_convert(&answer,answer.type,result);
	lbu	a1, 20(sp) # RV32: load a byte (unsigned/signed respectively); tagged type/token fields are byte-sized.
	addi	a0, sp, 12 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a2, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_convert # RV32: call n32_convert; arguments in a-registers; preserve any live caller-saved values.
.Ltmp11:
.LBB0_14:
	mv	s2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB0_15:
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB0_16:
	lw	ra, 60(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 56(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 52(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 48(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s3, 44(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s4, 40(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s5, 36(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
# C src/native32/math.c:64: }
	addi	sp, sp, 64 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp12:
.Lfunc_end0:
	.size	n32_math, .Lfunc_end0-n32_math
                                        # -- End function
	.p2align	2                               # -- Begin function double_math

# ======================================================================
# FUNCTION double_math
# Token dispatch and result calculation, binary64.
# Z80 SQR 13E7h: CALL 09A4h; LD HL,1380h; CALL 09B1h; JR 13F5h
# Z80 COS 1541h: LD HL,158Bh; CALL 070Bh ; add pi/2, fall into SIN
# Z80 TAN 15A8h: calls SIN/COS; JP 08A0h ; divide sine by cosine
# RV32 uses a token switch, explicit error returns and native output pointers.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	double_math,@function
double_math:                            # @double_math
.Lfunc_begin1:
# %bb.0:
# C src/native32/math_kernel.inc:93: switch(token){
	addi	a0, a0, -177 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a4, 6 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	bltu	a4, a0, .LBB1_8 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	addi	sp, sp, -32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 28(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 24(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 20(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 16(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s3, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s4, 8(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, a2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s2, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	slli	a0, a0, 2 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	lui	a1, %hi(.LJTI1_0) # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a1, %lo(.LJTI1_0) # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	add	a0, a0, a1 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	lw	a0, 0(a0) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	jr	a0 # RV32: unconditional control transfer to a basic block or switch target.
.LBB1_2:
# C src/native32/math_kernel.inc:98: case T_SQR:*result=x==0?(REAL)0:NAME(exponential)(NAME(logarithm)(x)*(REAL)0.5);break;
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__eqdf2 # RV32: call __eqdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp13:
	bnez	a0, .LBB1_13 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.3:
	li	a1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	j	.LBB1_15 # RV32: unconditional control transfer to a basic block or switch target.
.LBB1_4:
# C src/native32/math_kernel.inc:104: case T_ATN:*result=NAME(arctangent)(x);break;
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	double_arctangent # RV32: call double_arctangent; arguments in a-registers; preserve any live caller-saved values.
.Ltmp14:
	j	.LBB1_15 # RV32: unconditional control transfer to a basic block or switch target.
.LBB1_5:
	lui	a0, 262034 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a3, a0, 507 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/math_kernel.inc:101: case T_COS:*result=NAME(sine)(x+(REAL)0x1.921fb6p+0);break;
	lui	a2, 393216 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp15:
	j	.LBB1_10 # RV32: unconditional control transfer to a basic block or switch target.
.LBB1_6:
# C src/native32/math_kernel.inc:103: case T_TAN:{REAL numerator=NAME(sine)(x),denominator=NAME(sine)(x+(REAL)0x1.921fb6p+0);if(denominator==0)return N32_DIVZERO;*result=numerator/denominator;break;}
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	double_sine # RV32: call double_sine; arguments in a-registers; preserve any live caller-saved values.
.Ltmp16:
	mv	s3, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s4, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	lui	a0, 262034 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a3, a0, 507 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	lui	a2, 393216 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp17:
	call	double_sine # RV32: call double_sine; arguments in a-registers; preserve any live caller-saved values.
.Ltmp18:
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s2, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__eqdf2 # RV32: call __eqdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp19:
	li	a2, 4 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	beqz	a0, .LBB1_16 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.7:
	mv	a0, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a2, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__divdf3 # RV32: call __divdf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp20:
	j	.LBB1_15 # RV32: unconditional control transfer to a basic block or switch target.
.LBB1_8:
	li	a2, 9 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	j	.LBB1_17 # RV32: unconditional control transfer to a basic block or switch target.
.LBB1_9:
# C src/native32/math_kernel.inc:99: case T_SIN:*result=NAME(sine)(x);break;
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB1_10:
	call	double_sine # RV32: call double_sine; arguments in a-registers; preserve any live caller-saved values.
.Ltmp21:
	j	.LBB1_15 # RV32: unconditional control transfer to a basic block or switch target.
.LBB1_11:
# C src/native32/math_kernel.inc:95: case T_EXP:*result=NAME(exponential)(x);break;
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	j	.LBB1_14 # RV32: unconditional control transfer to a basic block or switch target.
.LBB1_12:
# C src/native32/math_kernel.inc:94: case T_LOG:*result=NAME(logarithm)(x);break;
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	double_logarithm # RV32: call double_logarithm; arguments in a-registers; preserve any live caller-saved values.
.Ltmp22:
	j	.LBB1_15 # RV32: unconditional control transfer to a basic block or switch target.
.LBB1_13:
# C src/native32/math_kernel.inc:98: case T_SQR:*result=x==0?(REAL)0:NAME(exponential)(NAME(logarithm)(x)*(REAL)0.5);break;
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	double_logarithm # RV32: call double_logarithm; arguments in a-registers; preserve any live caller-saved values.
.Ltmp23:
	lui	a3, 261632 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp24:
.LBB1_14:
	call	double_exponential # RV32: call double_exponential; arguments in a-registers; preserve any live caller-saved values.
.Ltmp25:
.LBB1_15:
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	sw	a0, 0(s0) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	a1, 4(s0) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
.LBB1_16:
	lw	ra, 28(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 24(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 20(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 16(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s3, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s4, 8(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.LBB1_17:
# C src/native32/math_kernel.inc:107: }
	mv	a0, a2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp26:
.Lfunc_end1:
	.size	double_math, .Lfunc_end1-double_math
	.section	.rodata,"a",@progbits
	.p2align	2, 0x0
.LJTI1_0:
	.word	.LBB1_2
	.word	.LBB1_9
	.word	.LBB1_5
	.word	.LBB1_6
	.word	.LBB1_4
	.word	.LBB1_11
	.word	.LBB1_12
                                        # -- End function
	.text
	.p2align	2                               # -- Begin function single_math

# ======================================================================
# FUNCTION single_math
# Token dispatch and result calculation, binary32.
# Z80 SQR 13E7h: CALL 09A4h; LD HL,1380h; CALL 09B1h; JR 13F5h
# Z80 COS 1541h: LD HL,158Bh; CALL 070Bh ; add pi/2, fall into SIN
# Z80 TAN 15A8h: calls SIN/COS; JP 08A0h ; divide sine by cosine
# RV32 uses a token switch, explicit error returns and native output pointers.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	single_math,@function
single_math:                            # @single_math
.Lfunc_begin2:
# %bb.0:
# C src/native32/math_kernel.inc:93: switch(token){
	addi	a0, a0, -177 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a3, 6 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	bltu	a3, a0, .LBB2_8 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	addi	sp, sp, -16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 8(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 4(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 0(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	slli	a0, a0, 2 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	lui	a1, %hi(.LJTI2_0) # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a1, %lo(.LJTI2_0) # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	add	a0, a0, a1 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	lw	a0, 0(a0) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	jr	a0 # RV32: unconditional control transfer to a basic block or switch target.
.LBB2_2:
# C src/native32/math_kernel.inc:98: case T_SQR:*result=x==0?(REAL)0:NAME(exponential)(NAME(logarithm)(x)*(REAL)0.5);break;
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__eqsf2 # RV32: call __eqsf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp27:
	mv	a1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a0, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	beqz	a1, .LBB2_14 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.3:
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	single_logarithm # RV32: call single_logarithm; arguments in a-registers; preserve any live caller-saved values.
.Ltmp28:
	lui	a1, 258048 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	call	__mulsf3 # RV32: call __mulsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp29:
	j	.LBB2_12 # RV32: unconditional control transfer to a basic block or switch target.
.LBB2_4:
# C src/native32/math_kernel.inc:104: case T_ATN:*result=NAME(arctangent)(x);break;
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	single_arctangent # RV32: call single_arctangent; arguments in a-registers; preserve any live caller-saved values.
.Ltmp30:
	j	.LBB2_14 # RV32: unconditional control transfer to a basic block or switch target.
.LBB2_5:
	lui	a0, 261265 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a0, -37 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/math_kernel.inc:101: case T_COS:*result=NAME(sine)(x+(REAL)0x1.921fb6p+0);break;
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__addsf3 # RV32: call __addsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp31:
	j	.LBB2_10 # RV32: unconditional control transfer to a basic block or switch target.
.LBB2_6:
# C src/native32/math_kernel.inc:103: case T_TAN:{REAL numerator=NAME(sine)(x),denominator=NAME(sine)(x+(REAL)0x1.921fb6p+0);if(denominator==0)return N32_DIVZERO;*result=numerator/denominator;break;}
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	single_sine # RV32: call single_sine; arguments in a-registers; preserve any live caller-saved values.
.Ltmp32:
	mv	s2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	lui	a0, 261265 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a0, -37 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__addsf3 # RV32: call __addsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp33:
	call	single_sine # RV32: call single_sine; arguments in a-registers; preserve any live caller-saved values.
.Ltmp34:
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__eqsf2 # RV32: call __eqsf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp35:
	li	a1, 4 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	beqz	a0, .LBB2_15 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.7:
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__divsf3 # RV32: call __divsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp36:
	j	.LBB2_14 # RV32: unconditional control transfer to a basic block or switch target.
.LBB2_8:
	li	a1, 9 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	j	.LBB2_16 # RV32: unconditional control transfer to a basic block or switch target.
.LBB2_9:
# C src/native32/math_kernel.inc:99: case T_SIN:*result=NAME(sine)(x);break;
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB2_10:
	call	single_sine # RV32: call single_sine; arguments in a-registers; preserve any live caller-saved values.
.Ltmp37:
	j	.LBB2_14 # RV32: unconditional control transfer to a basic block or switch target.
.LBB2_11:
# C src/native32/math_kernel.inc:95: case T_EXP:*result=NAME(exponential)(x);break;
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB2_12:
	call	single_exponential # RV32: call single_exponential; arguments in a-registers; preserve any live caller-saved values.
.Ltmp38:
	j	.LBB2_14 # RV32: unconditional control transfer to a basic block or switch target.
.LBB2_13:
# C src/native32/math_kernel.inc:94: case T_LOG:*result=NAME(logarithm)(x);break;
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	single_logarithm # RV32: call single_logarithm; arguments in a-registers; preserve any live caller-saved values.
.Ltmp39:
.LBB2_14:
	li	a1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	sw	a0, 0(s0) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
.LBB2_15:
	lw	ra, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 8(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 4(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 0(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.LBB2_16:
# C src/native32/math_kernel.inc:107: }
	mv	a0, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp40:
.Lfunc_end2:
	.size	single_math, .Lfunc_end2-single_math
	.section	.rodata,"a",@progbits
	.p2align	2, 0x0
.LJTI2_0:
	.word	.LBB2_2
	.word	.LBB2_9
	.word	.LBB2_5
	.word	.LBB2_6
	.word	.LBB2_4
	.word	.LBB2_11
	.word	.LBB2_13
                                        # -- End function
	.text
	.globl	n32_math_power                  # -- Begin function n32_math_power
	.p2align	2

# ======================================================================
# FUNCTION n32_math_power
# Checked fractional power using EXP(y*LOG(|x|)); preserve sign for odd integral y.
# Z80 13E7h: CALL 09A4h ; save argument
# Z80 13EAh: LD HL,1380h; CALL 09B1h; JR 13F5h ; load one half and enter power
# Z80 algorithm call sites: LOG 0809h and EXP 1439h.
# Native zero/domain checks and wide parity avoid narrowing exponents to Z80 byte values.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	n32_math_power,@function
n32_math_power:                         # @n32_math_power
.Lfunc_begin3:
# C src/native32/math.c:65: int n32_math_power(const N32Value *base,const N32Value *exponent,N32Value *result){
# %bb.0:
	addi	sp, sp, -96 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 92(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 88(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 84(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 80(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s3, 76(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s4, 72(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s5, 68(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s6, 64(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s6, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.Ltmp41:
# C src/native32/math.c:66: if(base->type==N32_STRING||exponent->type==N32_STRING)return N32_TYPE;
	lbu	a3, 8(a0) # RV32: load a byte (unsigned/signed respectively); tagged type/token fields are byte-sized.
	mv	s5, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a1, 3 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a0, 2 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	beq	a3, a1, .LBB3_17 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	mv	s0, a2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	lbu	a2, 8(s5) # RV32: load a byte (unsigned/signed respectively); tagged type/token fields are byte-sized.
	beq	a2, a1, .LBB3_17 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.2:
# C src/native32/math.c:67: double x=n32_real(base),y=n32_real(exponent);
	mv	a0, s6 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_real # RV32: call n32_real; arguments in a-registers; preserve any live caller-saved values.
.Ltmp42:
	mv	s3, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s4, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a0, s5 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_real # RV32: call n32_real; arguments in a-registers; preserve any live caller-saved values.
.Ltmp43:
	mv	s2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
# C src/native32/math.c:68: N32Value checked;int validation=n32_convert(base,N32_DOUBLE,&checked);if(validation)return validation;validation=n32_convert(exponent,N32_DOUBLE,&checked);if(validation)return validation;
	li	a1, 2 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	addi	a2, sp, 52 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a0, s6 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_convert # RV32: call n32_convert; arguments in a-registers; preserve any live caller-saved values.
.Ltmp44:
	bnez	a0, .LBB3_17 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.3:
	li	a1, 2 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	addi	a2, sp, 52 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a0, s5 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_convert # RV32: call n32_convert; arguments in a-registers; preserve any live caller-saved values.
.Ltmp45:
	bnez	a0, .LBB3_17 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.4:
# C src/native32/math.c:69: if(y==0){*result=base->type==N32_DOUBLE||exponent->type==N32_DOUBLE?n32_double(1):n32_single(1);return N32_OK;}
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__nedf2 # RV32: call __nedf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp46:
	bnez	a0, .LBB3_9 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.5:
	lbu	a1, 8(s6) # RV32: load a byte (unsigned/signed respectively); tagged type/token fields are byte-sized.
	li	a0, 2 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	beq	a1, a0, .LBB3_7 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.6:
	lbu	a1, 8(s5) # RV32: load a byte (unsigned/signed respectively); tagged type/token fields are byte-sized.
	bne	a1, a0, .LBB3_14 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
.LBB3_7:
	addi	a0, sp, 40 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	lui	a2, 261888 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	li	a1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
.LBB3_8:
	call	n32_double # RV32: call n32_double; arguments in a-registers; preserve any live caller-saved values.
.Ltmp47:
	j	.LBB3_16 # RV32: unconditional control transfer to a basic block or switch target.
.LBB3_9:
# C src/native32/math.c:70: if(x==0){if(y<0)return N32_DIVZERO;*result=base->type==N32_DOUBLE||exponent->type==N32_DOUBLE?n32_double(0):n32_single(0);return N32_OK;}
	mv	a0, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__nedf2 # RV32: call __nedf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp48:
	bnez	a0, .LBB3_18 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.10:
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltdf2 # RV32: call __ltdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp49:
	mv	a1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a0, 4 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	bltz	a1, .LBB3_17 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.11:
	lbu	a1, 8(s6) # RV32: load a byte (unsigned/signed respectively); tagged type/token fields are byte-sized.
	li	a0, 2 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	beq	a1, a0, .LBB3_13 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.12:
	lbu	a1, 8(s5) # RV32: load a byte (unsigned/signed respectively); tagged type/token fields are byte-sized.
	bne	a1, a0, .LBB3_24 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
.LBB3_13:
	addi	a0, sp, 40 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	j	.LBB3_8 # RV32: unconditional control transfer to a basic block or switch target.
.LBB3_14:
# C src/native32/math.c:69: if(y==0){*result=base->type==N32_DOUBLE||exponent->type==N32_DOUBLE?n32_double(1):n32_single(1);return N32_OK;}
	addi	a0, sp, 40 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	lui	a1, 260096 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
.LBB3_15:
	call	n32_single # RV32: call n32_single; arguments in a-registers; preserve any live caller-saved values.
.Ltmp50:
.LBB3_16:
	lw	a1, 48(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	a2, 44(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	a3, 40(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	li	a0, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	sw	a1, 8(s0) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	a2, 4(s0) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	a3, 0(s0) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
.LBB3_17:
	lw	ra, 92(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 88(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 84(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 80(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s3, 76(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s4, 72(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s5, 68(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s6, 64(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
# C src/native32/math.c:82: }
	addi	sp, sp, 96 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.LBB3_18:
# C src/native32/math.c:71: bool negative=x<0;if(negative&&floor_word(y)!=y)return N32_DOMAIN;
	mv	a0, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltdf2 # RV32: call __ltdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp51:
	bgez	a0, .LBB3_20 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.19:
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	floor_word # RV32: call floor_word; arguments in a-registers; preserve any live caller-saved values.
.Ltmp52:
	mv	a2, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__nedf2 # RV32: call __nedf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp53:
	mv	a1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a0, 5 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	bnez	a1, .LBB3_17 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
.LBB3_20:
# C src/native32/math.c:72: N32Type type=base->type==N32_DOUBLE||exponent->type==N32_DOUBLE?N32_DOUBLE:N32_SINGLE;
	lbu	a1, 8(s6) # RV32: load a byte (unsigned/signed respectively); tagged type/token fields are byte-sized.
	li	a0, 2 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	beq	a1, a0, .LBB3_22 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.21:
	lbu	a1, 8(s5) # RV32: load a byte (unsigned/signed respectively); tagged type/token fields are byte-sized.
# C src/native32/math.c:73: N32Value positive=type==N32_DOUBLE?n32_double(negative?-x:x):n32_single((float)(negative?-x:x)),logarithm,product;
	bne	a1, a0, .LBB3_25 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
.LBB3_22:
# C src/native32/math.c:71: bool negative=x<0;if(negative&&floor_word(y)!=y)return N32_DOMAIN;
	mv	a0, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltdf2 # RV32: call __ltdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp54:
# C src/native32/math.c:73: N32Value positive=type==N32_DOUBLE?n32_double(negative?-x:x):n32_single((float)(negative?-x:x)),logarithm,product;
	slti	a0, a0, 0 # RV32: produce a 0/1 predicate for native control flow.
	slli	a0, a0, 31 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	xor	a2, s4, a0 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	addi	a0, sp, 40 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a1, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_double # RV32: call n32_double; arguments in a-registers; preserve any live caller-saved values.
.Ltmp55:
# C src/native32/math.c:74: int status=n32_math(T_LOG,&positive,&logarithm);if(status)return status;
	li	a0, 183 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	addi	a1, sp, 40 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	addi	a2, sp, 28 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	call	n32_math # RV32: call n32_math; arguments in a-registers; preserve any live caller-saved values.
.Ltmp56:
	bnez	a0, .LBB3_17 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.23:
# C src/native32/math.c:75: N32Value power=type==N32_DOUBLE?n32_double(y):n32_single((float)y);
	addi	a0, sp, 4 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a1, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a2, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_double # RV32: call n32_double; arguments in a-registers; preserve any live caller-saved values.
.Ltmp57:
	j	.LBB3_27 # RV32: unconditional control transfer to a basic block or switch target.
.LBB3_24:
# C src/native32/math.c:70: if(x==0){if(y<0)return N32_DIVZERO;*result=base->type==N32_DOUBLE||exponent->type==N32_DOUBLE?n32_double(0):n32_single(0);return N32_OK;}
	addi	a0, sp, 40 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	j	.LBB3_15 # RV32: unconditional control transfer to a basic block or switch target.
.LBB3_25:
# C src/native32/math.c:71: bool negative=x<0;if(negative&&floor_word(y)!=y)return N32_DOMAIN;
	mv	a0, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltdf2 # RV32: call __ltdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp58:
# C src/native32/math.c:73: N32Value positive=type==N32_DOUBLE?n32_double(negative?-x:x):n32_single((float)(negative?-x:x)),logarithm,product;
	slti	a0, a0, 0 # RV32: produce a 0/1 predicate for native control flow.
	slli	a0, a0, 31 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	xor	a1, s4, a0 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	mv	a0, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__truncdfsf2 # RV32: call __truncdfsf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp59:
	mv	a1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	addi	a0, sp, 40 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	call	n32_single # RV32: call n32_single; arguments in a-registers; preserve any live caller-saved values.
.Ltmp60:
# C src/native32/math.c:74: int status=n32_math(T_LOG,&positive,&logarithm);if(status)return status;
	li	a0, 183 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	addi	a1, sp, 40 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	addi	a2, sp, 28 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	call	n32_math # RV32: call n32_math; arguments in a-registers; preserve any live caller-saved values.
.Ltmp61:
	bnez	a0, .LBB3_17 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.26:
# C src/native32/math.c:75: N32Value power=type==N32_DOUBLE?n32_double(y):n32_single((float)y);
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__truncdfsf2 # RV32: call __truncdfsf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp62:
	mv	a1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	addi	a0, sp, 4 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	call	n32_single # RV32: call n32_single; arguments in a-registers; preserve any live caller-saved values.
.Ltmp63:
.LBB3_27:
# C src/native32/math.c:76: status=n32_binary('*',&logarithm,&power,&product);if(status)return status;
	li	a0, 42 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	addi	a1, sp, 28 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	addi	a2, sp, 4 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	addi	a3, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	call	n32_binary # RV32: call n32_binary; arguments in a-registers; preserve any live caller-saved values.
.Ltmp64:
	bnez	a0, .LBB3_17 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.28:
# C src/native32/math.c:71: bool negative=x<0;if(negative&&floor_word(y)!=y)return N32_DOMAIN;
	mv	a0, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltdf2 # RV32: call __ltdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp65:
	mv	s3, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
# C src/native32/math.c:77: status=n32_math(T_EXP,&product,result);
	li	a0, 182 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	addi	a1, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a2, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_math # RV32: call n32_math; arguments in a-registers; preserve any live caller-saved values.
.Ltmp66:
# C src/native32/math.c:80: if(!status&&negative&&y-floor_word(y*0.5)*2!=0)status=n32_unary('-',result,result);
	bgez	s3, .LBB3_17 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.29:
	bnez	a0, .LBB3_17 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.30:
	lui	a3, 261632 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp67:
	call	floor_word # RV32: call floor_word; arguments in a-registers; preserve any live caller-saved values.
.Ltmp68:
	mv	a2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp69:
	mv	a2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__subdf3 # RV32: call __subdf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp70:
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__eqdf2 # RV32: call __eqdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp71:
	beqz	a0, .LBB3_17 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.31:
	li	a0, 45 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	mv	a1, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a2, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_unary # RV32: call n32_unary; arguments in a-registers; preserve any live caller-saved values.
.Ltmp72:
	j	.LBB3_17 # RV32: unconditional control transfer to a basic block or switch target.
.Ltmp73:
.Lfunc_end3:
	.size	n32_math_power, .Lfunc_end3-n32_math_power
                                        # -- End function
	.p2align	2                               # -- Begin function floor_word

# ======================================================================
# FUNCTION floor_word
# IEEE binary64 floor helper; native replacement for the ROM INT path.
# Z80 caller excerpt: CALL 0B40h ; INT/floor during EXP reduction
# Z80 0AFBh family shifts MBF bytes and corrects negative fractions (summary).
# No identical Z80 implementation: RV32 masks IEEE fraction bits and handles large integers.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	floor_word,@function
floor_word:                             # @floor_word
.Lfunc_begin4:
# C src/native32/math.c:13: static double floor_word(double value){
# %bb.0:
	addi	sp, sp, -48 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 44(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 40(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 36(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 32(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s3, 28(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s4, 24(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.Ltmp74:
# C src/native32/math.c:14: N32Value words=n32_double(value);uint64_t bits=((uint64_t)words.as.wide.high<<32)|words.as.wide.low;
	addi	a0, sp, 12 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a2, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_double # RV32: call n32_double; arguments in a-registers; preserve any live caller-saved values.
.Ltmp75:
	lw	a0, 16(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
# C src/native32/math.c:15: unsigned exponent=(unsigned)((bits>>52)&2047);
	slli	a2, a0, 1 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	srli	a2, a2, 21 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	li	a1, 1022 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
# C src/native32/math.c:16: if(exponent<1023)return value<0?-1:0;
	bltu	a1, a2, .LBB4_2 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltdf2 # RV32: call __ltdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp76:
	li	s1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	srai	a0, a0, 31 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	lui	s0, 786176 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	and	s0, a0, s0 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	j	.LBB4_8 # RV32: unconditional control transfer to a basic block or switch target.
.LBB4_2:
	li	a1, 1074 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
# C src/native32/math.c:17: if(exponent>=1075)return value;
	bltu	a1, a2, .LBB4_8 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.3:
	lw	a1, 12(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	li	a3, 1075 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
# C src/native32/math.c:18: uint64_t mask=(UINT64_C(1)<<(1075-exponent))-1;
	sub	a3, a3, a2 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a5, 1043 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	sub	a5, a5, a2 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	srai	a2, a5, 31 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	li	a6, -1 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	sll	a4, a6, a3 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	and	a2, a2, a4 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	bltz	a5, .LBB4_5 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.4:
	sll	a3, a6, a5 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	j	.LBB4_6 # RV32: unconditional control transfer to a basic block or switch target.
.LBB4_5:
	not	a3, a3 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	lui	a5, 524288 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a5, a5, -1 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	srl	a3, a5, a3 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	or	a3, a4, a3 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
.LBB4_6:
	not	a4, a3 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	not	a5, a2 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
# C src/native32/math.c:19: bool fraction=(bits&mask)!=0;bits&=~mask;
	and	a5, a1, a5 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	and	a4, a0, a4 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	or	a4, a5, a4 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	snez	s4, a4 # RV32: produce a 0/1 predicate for native control flow.
	and	a0, a3, a0 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	and	a1, a2, a1 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
# C src/native32/math.c:20: words.as.wide.low=(uint32_t)bits;words.as.wide.high=(uint32_t)(bits>>32);
	sw	a1, 12(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	a0, 16(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
# C src/native32/math.c:21: double whole=n32_real(&words);return fraction&&value<0?whole-1:whole;
	addi	a0, sp, 12 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	call	n32_real # RV32: call n32_real; arguments in a-registers; preserve any live caller-saved values.
.Ltmp77:
	mv	s2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s3, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltdf2 # RV32: call __ltdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp78:
	slti	a0, a0, 0 # RV32: produce a 0/1 predicate for native control flow.
	and	s4, a0, s4 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	lui	a3, 786176 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp79:
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s0, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	bnez	s4, .LBB4_8 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.7:
	mv	s0, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB4_8:
# C src/native32/math.c:22: }
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	lw	ra, 44(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 40(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 36(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 32(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s3, 28(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s4, 24(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 48 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp80:
.Lfunc_end4:
	.size	floor_word, .Lfunc_end4-floor_word
                                        # -- End function
	.p2align	2                               # -- Begin function double_logarithm

# ======================================================================
# FUNCTION double_logarithm
# LOG, binary64: normalize x=m*2^k; rational transform; odd polynomial.
# Z80 0809h: CALL 0955h; OR A; JP PE,1E4Ah ; reject invalid argument
# Z80 0810h: LD HL,4124h; LD A,(HL) ; accumulator exponent
# Z80 0814h: LD BC,8035h; LD DE,04F3h
# Z80 081Ah: SUB B; PUSH AF; LD (HL),B ; save exponent, normalize
# Z80: LD HL,07F8h; CALL 0710h ; table value minus accumulator
# Z80: LD HL,07FCh; CALL 149Ah ; z*P(z*z)
# RV32 extracts IEEE exponent fields and uses word-float add/divide/multiply helpers.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	double_logarithm,@function
double_logarithm:                       # @double_logarithm
.Lfunc_begin5:
# C src/native32/math_kernel.inc:29: static REAL NAME(logarithm)(REAL x){
# %bb.0:
	addi	sp, sp, -16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 8(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 4(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
.Ltmp81:
# C src/native32/math_kernel.inc:31: int exponent;REAL mantissa=FRACTION(x,&exponent);
	mv	a2, sp # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	fraction_double # RV32: call fraction_double; arguments in a-registers; preserve any live caller-saved values.
.Ltmp82:
	lui	a2, 261738 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a3, a2, 158 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/math_kernel.inc:32: REAL denominator=mantissa+(REAL)0x1.6a09e6p-1;
	lui	a2, 393216 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp83:
	mv	a2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	lui	a0, 786282 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a0, 158 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/math_kernel.inc:33: REAL z=(REAL)1-(REAL)0x1.6a09e6p+0/denominator;
	lui	a0, 393216 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	call	__divdf3 # RV32: call __divdf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp84:
	lui	a3, 261888 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp85:
# C src/native32/math_kernel.inc:34: REAL answer=NAME(odd)(z,coefficients,3);answer=answer-(REAL)0.5;answer=answer+(REAL)exponent;
	lui	a2, %hi(double_logarithm.coefficients) # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a2, a2, %lo(double_logarithm.coefficients) # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a3, 3 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	double_odd # RV32: call double_odd; arguments in a-registers; preserve any live caller-saved values.
.Ltmp86:
	lui	a3, 785920 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp87:
	lw	a2, 0(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a0, a2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__floatsidf # RV32: call __floatsidf; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp88:
	mv	a2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a0, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp89:
	lui	a2, 261731 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a3, a2, -445 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/math_kernel.inc:35: return answer*(REAL)0x1.62e43p-1;
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp90:
	lw	ra, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 8(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 4(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp91:
.Lfunc_end5:
	.size	double_logarithm, .Lfunc_end5-double_logarithm
                                        # -- End function
	.p2align	2                               # -- Begin function double_exponential

# ======================================================================
# FUNCTION double_exponential
# EXP, binary64: exponent reduction, polynomial, then power-of-two scaling.
# Z80 144Dh: CALL 0B40h ; floor after multiplication by log2(e)
# Z80: CALL 070Bh; CALL 0841h; CALL 0713h; CALL 0982h ; reduction
# Z80: LD HL,1479h; CALL 14A9h ; exp(-r) polynomial
# RV32 DOUBLE refinement: split ln(2), 18 coefficients, final subnormal rounding.
# The ROM uses eight coefficients; these are NOT the same constant table.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	double_exponential,@function
double_exponential:                     # @double_exponential
.Lfunc_begin6:
# C src/native32/exp_double.inc:13: static double double_exponential(double x){
# %bb.0:
	addi	sp, sp, -48 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 44(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 40(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 36(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 32(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s3, 28(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s4, 24(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s5, 20(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s6, 16(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s2, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s3, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.Ltmp92:
# C src/native32/exp_double.inc:14: if(x< -746)return 0;
	lui	a3, 788597 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltdf2 # RV32: call __ltdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp93:
	bgez	a0, .LBB6_2 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	li	s1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	s2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	j	.LBB6_10 # RV32: unconditional control transfer to a basic block or switch target.
.LBB6_2:
	lui	a0, 414392 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a2, a0, 766 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	lui	a0, 262001 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a3, a0, 1351 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/exp_double.inc:15: int scale=(int)floor_word(x*0x1.71547652b82fep+0)+1;
	mv	a0, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp94:
	call	floor_word # RV32: call floor_word; arguments in a-registers; preserve any live caller-saved values.
.Ltmp95:
	call	__fixdfsi # RV32: call __fixdfsi; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp96:
	mv	s0, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	addi	s1, a0, 1 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/exp_double.inc:16: double reduced=(double)scale*0x1.62e42fee00000p-1-x;
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__floatsidf # RV32: call __floatsidf; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp97:
	mv	s4, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s5, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	lui	a0, 261731 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a3, a0, -446 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	lui	a2, 1043968 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp98:
	mv	a2, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__subdf3 # RV32: call __subdf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp99:
	mv	s2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s3, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	lui	a0, 219028 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a2, a0, -906 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	lui	a0, 253604 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a3, a0, -1553 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/exp_double.inc:17: reduced=reduced+(double)scale*0x1.a39ef35793c76p-33;
	mv	a0, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s5 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp100:
	mv	a2, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp101:
# C src/native32/exp_double.inc:18: double answer=double_polynomial(reduced,n32_exp_coefficients,18);
	lui	a2, %hi(n32_exp_coefficients) # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a2, a2, %lo(n32_exp_coefficients) # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a3, 18 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	double_polynomial # RV32: call double_polynomial; arguments in a-registers; preserve any live caller-saved values.
.Ltmp102:
	li	s5, -1023 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	mv	s2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s3, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
# C src/native32/exp_double.inc:23: if(power>1023)power-=1023;
	bge	s0, s5, .LBB6_4 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.3:
	addi	s1, s0, 1023 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.LBB6_4:
	li	s6, 1022 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	bge	s6, s0, .LBB6_6 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.5:
	addi	a0, s0, 2047 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	addi	s1, a0, 1027 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.LBB6_6:
# C src/native32/exp_double.inc:25: N32Value factor=n32_double(0);factor.as.wide.high=(uint32_t)(power+1023)<<20;
	addi	a0, sp, 4 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	n32_double # RV32: call n32_double; arguments in a-registers; preserve any live caller-saved values.
.Ltmp103:
	slli	s1, s1, 20 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	lui	a0, 261888 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	add	a0, s1, a0 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	a0, 8(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
# C src/native32/exp_double.inc:26: answer=answer*n32_real(&factor);
	addi	a0, sp, 4 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	call	n32_real # RV32: call n32_real; arguments in a-registers; preserve any live caller-saved values.
.Ltmp104:
	mv	a2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp105:
	mv	s3, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s4, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
# C src/native32/exp_double.inc:27: if(scale>1023)answer=answer*0x1p1023;
	lui	a3, 523776 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp106:
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s2, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	lui	a3, 256 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp107:
	blt	s0, s5, .LBB6_8 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.7:
	mv	a0, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB6_8:
	blt	s6, s0, .LBB6_10 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.9:
	mv	s2, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB6_10:
# C src/native32/exp_double.inc:30: }
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	lw	ra, 44(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 40(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 36(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 32(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s3, 28(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s4, 24(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s5, 20(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s6, 16(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 48 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp108:
.Lfunc_end6:
	.size	double_exponential, .Lfunc_end6-double_exponential
                                        # -- End function
	.p2align	2                               # -- Begin function double_sine

# ======================================================================
# FUNCTION double_sine
# SIN, binary64: reduce to turns, fold quarter/half turns, odd polynomial.
# Z80 1547h: divide by ROM 2*pi; floor and subtract (algorithm summary)
# Z80: CALL 0710h ; table value MINUS accumulator, not the reverse
# Z80: LD HL,1593h; JP 149Ah ; five-coefficient odd polynomial
# RV32 branches preserve folding order; large-angle precision remains limited.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	double_sine,@function
double_sine:                            # @double_sine
.Lfunc_begin7:
# C src/native32/math_kernel.inc:71: static REAL NAME(sine)(REAL x){
# %bb.0:
	addi	sp, sp, -32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 28(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 24(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 20(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 16(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s3, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s4, 8(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lui	a2, 262546 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a3, a2, 507 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.Ltmp109:
# C src/native32/math_kernel.inc:74: REAL turns=x/(REAL)0x1.921fb6p+2;REAL fraction=turns-(REAL)floor_word(turns);
	lui	a2, 393216 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	call	__divdf3 # RV32: call __divdf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp110:
	mv	s0, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	floor_word # RV32: call floor_word; arguments in a-registers; preserve any live caller-saved values.
.Ltmp111:
# C src/native32/math_kernel.inc:75: REAL folded=(REAL)0.25-fraction;bool negative=folded<0;
	mv	a2, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__subdf3 # RV32: call __subdf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp112:
	lui	a3, 261376 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp113:
	mv	s2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s3, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltdf2 # RV32: call __ltdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp114:
	mv	s0, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
# C src/native32/math_kernel.inc:76: if(negative)folded=folded+(REAL)0.5;
	lui	a3, 261632 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp115:
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s4, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	bltz	s0, .LBB7_2 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	mv	s1, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s4, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB7_2:
# C src/native32/math_kernel.inc:77: if(folded>=0)folded=-folded;folded=folded+(REAL)0.25;if(negative)folded=-folded;
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__gedf2 # RV32: call __gedf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp116:
	slti	a0, a0, 0 # RV32: produce a 0/1 predicate for native control flow.
	not	a0, a0 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	slli	a0, a0, 31 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	xor	a1, s4, a0 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	lui	a3, 261376 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp117:
	lui	a2, 524288 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	and	a2, s0, a2 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	xor	a1, a1, a2 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
# C src/native32/math_kernel.inc:78: return NAME(odd)(folded,coefficients,5);
	lui	a2, %hi(double_sine.coefficients) # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a2, a2, %lo(double_sine.coefficients) # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a3, 5 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	lw	ra, 28(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 24(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 20(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 16(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s3, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s4, 8(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.Ltmp118:
	tail	double_odd # RV32: tail-transfer to double_odd; arguments in a-registers; preserve any live caller-saved values.
.Ltmp119:
.Lfunc_end7:
	.size	double_sine, .Lfunc_end7-double_sine
                                        # -- End function
	.p2align	2                               # -- Begin function double_arctangent

# ======================================================================
# FUNCTION double_arctangent
# ATN, binary64: save sign; reciprocal reduction when |x|>=1.
# Z80 15BDh: sign/exponent tests and reciprocal (algorithm summary)
# Z80: LD HL,15E3h; CALL 149Ah ; nine-coefficient odd polynomial
# Z80 return through 0710h ; pi/2 minus result for reciprocal branch
# RV32 restores the original sign after evaluation and optional pi/2 subtraction.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	double_arctangent,@function
double_arctangent:                      # @double_arctangent
.Lfunc_begin8:
# C src/native32/math_kernel.inc:84: static REAL NAME(arctangent)(REAL x){
# %bb.0:
	addi	sp, sp, -32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 28(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 24(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 20(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 16(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s3, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.Ltmp120:
# C src/native32/math_kernel.inc:88: bool negative=x<0;if(negative)x=-x;bool reciprocal=x>=(REAL)1;
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	li	a3, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltdf2 # RV32: call __ltdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp121:
	lui	a1, 524288 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	and	s3, a0, a1 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	xor	s2, s0, s3 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	lui	a3, 261888 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__gedf2 # RV32: call __gedf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp122:
	mv	s0, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
# C src/native32/math_kernel.inc:89: if(reciprocal)x=(REAL)1/x;REAL answer=NAME(odd)(x,coefficients,9);
	lui	a1, 261888 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	li	a0, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	mv	a2, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__divdf3 # RV32: call __divdf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp123:
	bgez	s0, .LBB8_2 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	mv	a1, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB8_2:
	lui	a2, %hi(double_arctangent.coefficients) # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a2, a2, %lo(double_arctangent.coefficients) # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a3, 9 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	double_odd # RV32: call double_odd; arguments in a-registers; preserve any live caller-saved values.
.Ltmp124:
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s2, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	lui	a0, 262034 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a0, 507 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/math_kernel.inc:90: if(reciprocal)answer=(REAL)0x1.921fb6p+0-answer;return negative?-answer:answer;
	lui	a0, 393216 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a2, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__subdf3 # RV32: call __subdf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp125:
	bgez	s0, .LBB8_4 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.3:
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB8_4:
	xor	a1, a1, s3 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	lw	ra, 28(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 24(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 20(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 16(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s3, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp126:
.Lfunc_end8:
	.size	double_arctangent, .Lfunc_end8-double_arctangent
                                        # -- End function
	.p2align	2                               # -- Begin function fraction_double

# ======================================================================
# FUNCTION fraction_double
# IEEE mantissa/exponent extraction for LOG; positive input precondition.
# Z80 LOG 0810h: LD HL,4124h; LD A,(HL)
# Z80 0814h: LD BC,8035h; LD DE,04F3h
# Z80 081Ah: SUB B; PUSH AF; LD (HL),B ; normalize with B=80h
# No identical Z80 helper: RV32 masks IEEE words and scales subnormals first.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	fraction_double,@function
fraction_double:                        # @fraction_double
.Lfunc_begin9:
# C src/native32/math.c:29: static double fraction_double(double value,int *exponent){
# %bb.0:
	addi	sp, sp, -48 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 44(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 40(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 36(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 32(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.Ltmp127:
# C src/native32/math.c:30: N32Value words=n32_double(value);int adjust=0;
	addi	a0, sp, 20 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a1, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a2, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_double # RV32: call n32_double; arguments in a-registers; preserve any live caller-saved values.
.Ltmp128:
# C src/native32/math.c:31: if(value<0x1p-1022){value*=0x1p54;adjust=-54;words=n32_double(value);}
	lui	a3, 256 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltdf2 # RV32: call __ltdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp129:
	li	a1, -1022 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	bgez	a0, .LBB9_2 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	lui	a3, 275712 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a2, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp130:
	mv	a2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	addi	a0, sp, 8 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a1, a2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a2, a3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	n32_double # RV32: call n32_double; arguments in a-registers; preserve any live caller-saved values.
.Ltmp131:
	lw	a0, 16(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	a1, 12(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	a2, 8(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	a0, 28(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	a1, 24(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	a2, 20(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	li	a1, -1076 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
.LBB9_2:
# C src/native32/math.c:32: *exponent=(int)((words.as.wide.high>>20)&2047)-1022+adjust;
	lw	a0, 24(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	slli	a2, a0, 1 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	srli	a2, a2, 21 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	add	a1, a1, a2 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	a1, 0(s0) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
# C src/native32/math.c:33: words.as.wide.high=(words.as.wide.high&0xfffff)|UINT32_C(0x3fe00000);return n32_real(&words);
	slli	a0, a0, 12 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	srli	a0, a0, 12 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	lui	a1, 261632 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	or	a0, a0, a1 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	sw	a0, 24(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	a0, sp, 20 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	call	n32_real # RV32: call n32_real; arguments in a-registers; preserve any live caller-saved values.
.Ltmp132:
	lw	ra, 44(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 40(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 36(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 32(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 48 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp133:
.Lfunc_end9:
	.size	fraction_double, .Lfunc_end9-fraction_double
                                        # -- End function
	.p2align	2                               # -- Begin function double_odd

# ======================================================================
# FUNCTION double_odd
# Odd polynomial x*P(x*x), binary64.
# Z80 149Ah: CALL 09A4h ; save x
# Z80 14A5h: CALL 0847h ; square saved x
# Z80 14A8h: POP HL ; fall through into Horner at 14A9h; stacked return later multiplies by saved x
# RV32 retains x across the polynomial call using stack/callee-saved registers.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	double_odd,@function
double_odd:                             # @double_odd
.Lfunc_begin10:
# C src/native32/math_kernel.inc:16: static REAL NAME(odd)(REAL x,const REAL *coefficients,unsigned count){
# %bb.0:
	addi	sp, sp, -32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 28(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 24(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 20(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 16(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s3, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, a2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s2, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s3, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.Ltmp134:
# C src/native32/math_kernel.inc:17: REAL square=x*x,answer=NAME(polynomial)(square,coefficients,count);return x*answer;
	mv	a2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp135:
	mv	a2, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	double_polynomial # RV32: call double_polynomial; arguments in a-registers; preserve any live caller-saved values.
.Ltmp136:
	mv	a2, s3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp137:
	lw	ra, 28(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 24(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 20(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 16(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s3, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp138:
.Lfunc_end10:
	.size	double_odd, .Lfunc_end10-double_odd
                                        # -- End function
	.p2align	2                               # -- Begin function double_polynomial

# ======================================================================
# FUNCTION double_polynomial
# Horner polynomial, binary64; coefficient count includes the leading term.
# Z80 14A9h: CALL 09A4h ; save argument
# Z80 14ACh: LD A,(HL); INC HL; CALL 09B1h ; load coefficient count/value
# Z80 14BBh: CALL 0847h ; accumulator *= x
# Z80 14C3h: CALL 0716h ; accumulator += coefficient
# RV32 loop advances native coefficient words; no MBF byte decoder or Z80 flags.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	double_polynomial,@function
double_polynomial:                      # @double_polynomial
.Lfunc_begin11:
# %bb.0:
# C src/native32/math_kernel.inc:12: REAL value=coefficients[0];for(unsigned i=1;i<count;i++){value=value*x;value=value+coefficients[i];}return value;
	lw	a5, 4(a2) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	a4, 0(a2) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	li	a6, 2 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	bltu	a3, a6, .LBB11_4 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	addi	sp, sp, -32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 28(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 24(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 20(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 16(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s3, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	addi	s2, a2, 8 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	slli	a3, a3, 3 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	add	s3, a2, a3 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.LBB11_2:                               # =>This Inner Loop Header: Depth=1
	mv	a0, a4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, a5 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a2, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a3, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__muldf3 # RV32: call __muldf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp139:
	lw	a2, 0(s2) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	a3, 4(s2) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	call	__adddf3 # RV32: call __adddf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp140:
	addi	s2, s2, 8 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a4, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a5, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	bne	s2, s3, .LBB11_2 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.3:
	lw	ra, 28(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 24(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 20(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 16(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s3, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.LBB11_4:
	mv	a0, a4 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, a5 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp141:
.Lfunc_end11:
	.size	double_polynomial, .Lfunc_end11-double_polynomial
                                        # -- End function
	.p2align	2                               # -- Begin function single_logarithm

# ======================================================================
# FUNCTION single_logarithm
# LOG, binary32: normalize x=m*2^k; rational transform; odd polynomial.
# Z80 0809h: CALL 0955h; OR A; JP PE,1E4Ah ; reject invalid argument
# Z80 0810h: LD HL,4124h; LD A,(HL) ; accumulator exponent
# Z80 0814h: LD BC,8035h; LD DE,04F3h
# Z80 081Ah: SUB B; PUSH AF; LD (HL),B ; save exponent, normalize
# Z80: LD HL,07F8h; CALL 0710h ; table value minus accumulator
# Z80: LD HL,07FCh; CALL 149Ah ; z*P(z*z)
# RV32 extracts IEEE exponent fields and uses word-float add/divide/multiply helpers.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	single_logarithm,@function
single_logarithm:                       # @single_logarithm
.Lfunc_begin12:
# C src/native32/math_kernel.inc:29: static REAL NAME(logarithm)(REAL x){
# %bb.0:
	addi	sp, sp, -16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 8(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
.Ltmp142:
# C src/native32/math_kernel.inc:31: int exponent;REAL mantissa=FRACTION(x,&exponent);
	addi	a1, sp, 4 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	call	fraction_single # RV32: call fraction_single; arguments in a-registers; preserve any live caller-saved values.
.Ltmp143:
	lui	a1, 258896 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a1, 1267 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/math_kernel.inc:32: REAL denominator=mantissa+(REAL)0x1.6a09e6p-1;
	call	__addsf3 # RV32: call __addsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp144:
	mv	a1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	lui	a0, 785232 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a0, a0, 1267 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/math_kernel.inc:33: REAL z=(REAL)1-(REAL)0x1.6a09e6p+0/denominator;
	call	__divsf3 # RV32: call __divsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp145:
	lui	a1, 260096 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	call	__addsf3 # RV32: call __addsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp146:
# C src/native32/math_kernel.inc:34: REAL answer=NAME(odd)(z,coefficients,3);answer=answer-(REAL)0.5;answer=answer+(REAL)exponent;
	lui	a1, %hi(single_logarithm.coefficients) # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a1, %lo(single_logarithm.coefficients) # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a2, 3 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	single_odd # RV32: call single_odd; arguments in a-registers; preserve any live caller-saved values.
.Ltmp147:
	lui	a1, 782336 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	call	__addsf3 # RV32: call __addsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp148:
	lw	a1, 4(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a0, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__floatsisf # RV32: call __floatsisf; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp149:
	mv	a1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a0, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__addsf3 # RV32: call __addsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp150:
	lui	a1, 258839 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a1, 536 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/math_kernel.inc:35: return answer*(REAL)0x1.62e43p-1;
	call	__mulsf3 # RV32: call __mulsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp151:
	lw	ra, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 8(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp152:
.Lfunc_end12:
	.size	single_logarithm, .Lfunc_end12-single_logarithm
                                        # -- End function
	.p2align	2                               # -- Begin function single_exponential

# ======================================================================
# FUNCTION single_exponential
# EXP, binary32: exponent reduction, polynomial, then power-of-two scaling.
# Z80 144Dh: CALL 0B40h ; floor after multiplication by log2(e)
# Z80: CALL 070Bh; CALL 0841h; CALL 0713h; CALL 0982h ; reduction
# Z80: LD HL,1479h; CALL 14A9h ; exp(-r) polynomial
# RV32 SINGLE retains eight decoded ROM coefficients and binary32 rounding.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	single_exponential,@function
single_exponential:                     # @single_exponential
.Lfunc_begin13:
# C src/native32/math_kernel.inc:48: static REAL NAME(exponential)(REAL x){
# %bb.0:
	addi	sp, sp, -16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 8(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 4(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 0(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.Ltmp153:
# C src/native32/math_kernel.inc:54: if(x<(REAL)-2048)return (REAL)0;
	lui	a1, 806912 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	call	__ltsf2 # RV32: call __ltsf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp154:
	bgez	a0, .LBB13_2 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	li	a0, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	j	.LBB13_14 # RV32: unconditional control transfer to a basic block or switch target.
.LBB13_2:
	lui	a0, 261003 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a0, -1477 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/math_kernel.inc:55: REAL binary=x*(REAL)0x1.715476p+0;int scale=(int)floor_word(binary)+1;
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__mulsf3 # RV32: call __mulsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp155:
	call	__extendsfdf2 # RV32: call __extendsfdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp156:
	call	floor_word # RV32: call floor_word; arguments in a-registers; preserve any live caller-saved values.
.Ltmp157:
	call	__fixdfsi # RV32: call __fixdfsi; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp158:
	mv	s2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	addi	s0, a0, 1 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/math_kernel.inc:56: REAL reduced=(REAL)scale*(REAL)0x1.62e43p-1;reduced=reduced-x;
	mv	a0, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__floatsisf # RV32: call __floatsisf; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp159:
	lui	a1, 258839 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a1, 536 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	call	__mulsf3 # RV32: call __mulsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp160:
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__subsf3 # RV32: call __subsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp161:
# C src/native32/math_kernel.inc:57: REAL answer=NAME(polynomial)(reduced,coefficients,8);
	lui	a1, %hi(single_exponential.coefficients) # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a1, %lo(single_exponential.coefficients) # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a2, 8 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	single_polynomial # RV32: call single_polynomial; arguments in a-registers; preserve any live caller-saved values.
.Ltmp162:
	li	a1, 32 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
# C src/native32/math_kernel.inc:58: while(scale>32){answer=answer*(REAL)0x1p32;scale-=32;}
	blt	s2, a1, .LBB13_6 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.3:
	addi	s0, s2, 33 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	s1, 64 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
.LBB13_4:                               # =>This Inner Loop Header: Depth=1
	lui	a1, 325632 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	call	__mulsf3 # RV32: call __mulsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp163:
	addi	s0, s0, -32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	bltu	s1, s0, .LBB13_4 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.5:
# C src/native32/math_kernel.inc:60: while(scale>0){answer=answer*(REAL)2;scale--;}
	addi	s0, s0, -32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	j	.LBB13_12 # RV32: unconditional control transfer to a basic block or switch target.
.LBB13_6:
	li	a1, -34 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
# C src/native32/math_kernel.inc:59: while(scale< -32){answer=answer*(REAL)0x1p-32;scale+=32;}
	blt	a1, s2, .LBB13_11 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.7:
	addi	s2, s2, -31 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	s0, -64 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
.LBB13_8:                               # =>This Inner Loop Header: Depth=1
	lui	a1, 194560 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	call	__mulsf3 # RV32: call __mulsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp164:
	addi	s2, s2, 32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	bltu	s2, s0, .LBB13_8 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.9:
# C src/native32/math_kernel.inc:61: while(scale<0){answer=answer*(REAL)0.5;scale++;}return answer;
	addi	s0, s2, 32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.LBB13_10:                              # =>This Inner Loop Header: Depth=1
	lui	a1, 258048 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	call	__mulsf3 # RV32: call __mulsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp165:
	addi	s0, s0, 1 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	bnez	s0, .LBB13_10 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
	j	.LBB13_14 # RV32: unconditional control transfer to a basic block or switch target.
.LBB13_11:
# C src/native32/math_kernel.inc:60: while(scale>0){answer=answer*(REAL)2;scale--;}
	bltz	s2, .LBB13_15 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
.LBB13_12:
	addi	s0, s0, 1 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	s1, 1 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
.LBB13_13:                              # =>This Inner Loop Header: Depth=1
	mv	a1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__addsf3 # RV32: call __addsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp166:
	addi	s0, s0, -1 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	bltu	s1, s0, .LBB13_13 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
.LBB13_14:
	lw	ra, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 8(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 4(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 0(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
# C src/native32/math_kernel.inc:62: }
	addi	sp, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.LBB13_15:
	li	a1, -1 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
# C src/native32/math_kernel.inc:61: while(scale<0){answer=answer*(REAL)0.5;scale++;}return answer;
	bne	s2, a1, .LBB13_10 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
	j	.LBB13_14 # RV32: unconditional control transfer to a basic block or switch target.
.Ltmp167:
.Lfunc_end13:
	.size	single_exponential, .Lfunc_end13-single_exponential
                                        # -- End function
	.p2align	2                               # -- Begin function single_sine

# ======================================================================
# FUNCTION single_sine
# SIN, binary32: reduce to turns, fold quarter/half turns, odd polynomial.
# Z80 1547h: divide by ROM 2*pi; floor and subtract (algorithm summary)
# Z80: CALL 0710h ; table value MINUS accumulator, not the reverse
# Z80: LD HL,1593h; JP 149Ah ; five-coefficient odd polynomial
# RV32 branches preserve folding order; large-angle precision remains limited.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	single_sine,@function
single_sine:                            # @single_sine
.Lfunc_begin14:
# C src/native32/math_kernel.inc:71: static REAL NAME(sine)(REAL x){
# %bb.0:
	addi	sp, sp, -16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 8(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 4(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 0(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lui	a1, 265361 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a1, -37 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.Ltmp168:
# C src/native32/math_kernel.inc:74: REAL turns=x/(REAL)0x1.921fb6p+2;REAL fraction=turns-(REAL)floor_word(turns);
	call	__divsf3 # RV32: call __divsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp169:
	mv	s0, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__extendsfdf2 # RV32: call __extendsfdf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp170:
	call	floor_word # RV32: call floor_word; arguments in a-registers; preserve any live caller-saved values.
.Ltmp171:
	call	__truncdfsf2 # RV32: call __truncdfsf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp172:
# C src/native32/math_kernel.inc:75: REAL folded=(REAL)0.25-fraction;bool negative=folded<0;
	mv	a1, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__subsf3 # RV32: call __subsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp173:
	lui	a1, 256000 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	call	__addsf3 # RV32: call __addsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp174:
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltsf2 # RV32: call __ltsf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp175:
	mv	s0, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
# C src/native32/math_kernel.inc:76: if(negative)folded=folded+(REAL)0.5;
	lui	a1, 258048 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__addsf3 # RV32: call __addsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp176:
	mv	s2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	bltz	s0, .LBB14_2 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	mv	s2, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB14_2:
# C src/native32/math_kernel.inc:77: if(folded>=0)folded=-folded;folded=folded+(REAL)0.25;if(negative)folded=-folded;
	mv	a0, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	li	a1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__gesf2 # RV32: call __gesf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp177:
	slti	a0, a0, 0 # RV32: produce a 0/1 predicate for native control flow.
	not	a0, a0 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	slli	a0, a0, 31 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	xor	a0, s2, a0 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	lui	a1, 256000 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	call	__addsf3 # RV32: call __addsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp178:
	lui	a1, 524288 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	and	a1, s0, a1 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	xor	a0, a0, a1 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
# C src/native32/math_kernel.inc:78: return NAME(odd)(folded,coefficients,5);
	lui	a1, %hi(single_sine.coefficients) # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a1, %lo(single_sine.coefficients) # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a2, 5 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	lw	ra, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 8(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 4(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 0(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.Ltmp179:
	tail	single_odd # RV32: tail-transfer to single_odd; arguments in a-registers; preserve any live caller-saved values.
.Ltmp180:
.Lfunc_end14:
	.size	single_sine, .Lfunc_end14-single_sine
                                        # -- End function
	.p2align	2                               # -- Begin function single_arctangent

# ======================================================================
# FUNCTION single_arctangent
# ATN, binary32: save sign; reciprocal reduction when |x|>=1.
# Z80 15BDh: sign/exponent tests and reciprocal (algorithm summary)
# Z80: LD HL,15E3h; CALL 149Ah ; nine-coefficient odd polynomial
# Z80 return through 0710h ; pi/2 minus result for reciprocal branch
# RV32 restores the original sign after evaluation and optional pi/2 subtraction.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	single_arctangent,@function
single_arctangent:                      # @single_arctangent
.Lfunc_begin15:
# C src/native32/math_kernel.inc:84: static REAL NAME(arctangent)(REAL x){
# %bb.0:
	addi	sp, sp, -16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 8(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 4(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 0(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.Ltmp181:
# C src/native32/math_kernel.inc:88: bool negative=x<0;if(negative)x=-x;bool reciprocal=x>=(REAL)1;
	li	a1, 0 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	__ltsf2 # RV32: call __ltsf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp182:
	lui	a1, 524288 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	and	s2, a0, a1 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	xor	s1, s0, s2 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	lui	a1, 260096 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__gesf2 # RV32: call __gesf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp183:
	mv	s0, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
# C src/native32/math_kernel.inc:89: if(reciprocal)x=(REAL)1/x;REAL answer=NAME(odd)(x,coefficients,9);
	lui	a0, 260096 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__divsf3 # RV32: call __divsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp184:
	bgez	s0, .LBB15_2 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB15_2:
	lui	a1, %hi(single_arctangent.coefficients) # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a1, a1, %lo(single_arctangent.coefficients) # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a2, 9 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	single_odd # RV32: call single_odd; arguments in a-registers; preserve any live caller-saved values.
.Ltmp185:
	mv	s1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	lui	a0, 261265 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	addi	a0, a0, -37 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
# C src/native32/math_kernel.inc:90: if(reciprocal)answer=(REAL)0x1.921fb6p+0-answer;return negative?-answer:answer;
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__subsf3 # RV32: call __subsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp186:
	bgez	s0, .LBB15_4 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.3:
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.LBB15_4:
	xor	a0, a0, s2 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	lw	ra, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 8(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 4(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 0(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp187:
.Lfunc_end15:
	.size	single_arctangent, .Lfunc_end15-single_arctangent
                                        # -- End function
	.p2align	2                               # -- Begin function fraction_single

# ======================================================================
# FUNCTION fraction_single
# IEEE mantissa/exponent extraction for LOG; positive input precondition.
# Z80 LOG 0810h: LD HL,4124h; LD A,(HL)
# Z80 0814h: LD BC,8035h; LD DE,04F3h
# Z80 081Ah: SUB B; PUSH AF; LD (HL),B ; normalize with B=80h
# No identical Z80 helper: RV32 masks IEEE words and scales subnormals first.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	fraction_single,@function
fraction_single:                        # @fraction_single
.Lfunc_begin16:
# C src/native32/math.c:23: static float fraction_single(float value,int *exponent){
# %bb.0:
	addi	sp, sp, -32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 28(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 24(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 20(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	sw	a0, 16(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
.Ltmp188:
# C src/native32/math.c:24: uint32_t bits;memcpy(&bits,&value,4);int adjust=0;
	addi	a0, sp, 12 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	addi	a1, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a2, 4 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	memcpy # RV32: call memcpy; arguments in a-registers; preserve any live caller-saved values.
.Ltmp189:
# C src/native32/math.c:25: if(value<0x1p-126f){value*=0x1p24f;adjust=-24;memcpy(&bits,&value,4);}
	lw	s1, 16(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lui	a1, 2048 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__ltsf2 # RV32: call __ltsf2; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp190:
	li	a1, -126 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	bgez	a0, .LBB16_2 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	lui	a1, 309248 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	mv	a0, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__mulsf3 # RV32: call __mulsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp191:
	sw	a0, 16(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	a0, sp, 12 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	addi	a1, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a2, 4 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	memcpy # RV32: call memcpy; arguments in a-registers; preserve any live caller-saved values.
.Ltmp192:
	li	a1, -150 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
.LBB16_2:
# C src/native32/math.c:26: *exponent=(int)((bits>>23)&255)-126+adjust;
	lw	a0, 12(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	slli	a2, a0, 1 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	srli	a2, a2, 24 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	add	a1, a1, a2 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	a1, 0(s0) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
# C src/native32/math.c:27: bits=(bits&0x7fffff)|UINT32_C(0x3f000000);memcpy(&value,&bits,4);return value;
	slli	a0, a0, 9 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	srli	a0, a0, 9 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	lui	a1, 258048 # RV32: materialize upper 20 bits of an address or constant; paired lower bits complete the word.
	or	a0, a0, a1 # RV32: bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.
	sw	a0, 12(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	a0, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	addi	a1, sp, 12 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	li	a2, 4 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	call	memcpy # RV32: call memcpy; arguments in a-registers; preserve any live caller-saved values.
.Ltmp193:
	lw	a0, 16(sp) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	ra, 28(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 24(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 20(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 32 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp194:
.Lfunc_end16:
	.size	fraction_single, .Lfunc_end16-fraction_single
                                        # -- End function
	.p2align	2                               # -- Begin function single_odd

# ======================================================================
# FUNCTION single_odd
# Odd polynomial x*P(x*x), binary32.
# Z80 149Ah: CALL 09A4h ; save x
# Z80 14A5h: CALL 0847h ; square saved x
# Z80 14A8h: POP HL ; fall through into Horner at 14A9h; stacked return later multiplies by saved x
# RV32 retains x across the polynomial call using stack/callee-saved registers.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	single_odd,@function
single_odd:                             # @single_odd
.Lfunc_begin17:
# C src/native32/math_kernel.inc:16: static REAL NAME(odd)(REAL x,const REAL *coefficients,unsigned count){
# %bb.0:
	addi	sp, sp, -16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 8(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 4(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 0(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s1, a1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	s2, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
.Ltmp195:
# C src/native32/math_kernel.inc:17: REAL square=x*x,answer=NAME(polynomial)(square,coefficients,count);return x*answer;
	mv	a1, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__mulsf3 # RV32: call __mulsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp196:
	mv	a1, s1 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a2, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	single_polynomial # RV32: call single_polynomial; arguments in a-registers; preserve any live caller-saved values.
.Ltmp197:
	mv	a1, s2 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__mulsf3 # RV32: call __mulsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp198:
	lw	ra, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 8(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 4(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 0(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp199:
.Lfunc_end17:
	.size	single_odd, .Lfunc_end17-single_odd
                                        # -- End function
	.p2align	2                               # -- Begin function single_polynomial

# ======================================================================
# FUNCTION single_polynomial
# Horner polynomial, binary32; coefficient count includes the leading term.
# Z80 14A9h: CALL 09A4h ; save argument
# Z80 14ACh: LD A,(HL); INC HL; CALL 09B1h ; load coefficient count/value
# Z80 14BBh: CALL 0847h ; accumulator *= x
# Z80 14C3h: CALL 0716h ; accumulator += coefficient
# RV32 loop advances native coefficient words; no MBF byte decoder or Z80 flags.
# RV32: saved-register spills preserve caller state; branches implement C control flow.
# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).
# ======================================================================
	.type	single_polynomial,@function
single_polynomial:                      # @single_polynomial
.Lfunc_begin18:
# %bb.0:
# C src/native32/math_kernel.inc:12: REAL value=coefficients[0];for(unsigned i=1;i<count;i++){value=value*x;value=value+coefficients[i];}return value;
	lw	a3, 0(a1) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	li	a4, 2 # RV32: materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.
	bltu	a2, a4, .LBB18_4 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.1:
	addi	sp, sp, -16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	sw	ra, 12(sp)                      # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s0, 8(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s1, 4(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	sw	s2, 0(sp)                       # 4-byte Folded Spill # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	mv	s0, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	addi	s1, a1, 4 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	slli	a2, a2, 2 # RV32: shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.
	add	s2, a1, a2 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.LBB18_2:                               # =>This Inner Loop Header: Depth=1
	mv	a0, a3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	mv	a1, s0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	call	__mulsf3 # RV32: call __mulsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp200:
	lw	a1, 0(s1) # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	call	__addsf3 # RV32: call __addsf3; soft-float/compiler ABI helper; IEEE bits travel in integer registers.
.Ltmp201:
	addi	s1, s1, 4 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
	mv	a3, a0 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	bne	s1, s2, .LBB18_2 # RV32: conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.
# %bb.3:
	lw	ra, 12(sp)                      # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s0, 8(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s1, 4(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	lw	s2, 0(sp)                       # 4-byte Folded Reload # RV32: transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.
	addi	sp, sp, 16 # RV32: integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.
.LBB18_4:
	mv	a0, a3 # RV32: copy an integer register or floating-point bit-pattern word without numeric conversion.
	ret # RV32: return to ra; a0 (and a1 for binary64) carries the result.
.Ltmp202:
.Lfunc_end18:
	.size	single_polynomial, .Lfunc_end18-single_polynomial
                                        # -- End function
	.type	double_logarithm.coefficients,@object # @double_logarithm.coefficients
	.section	.rodata,"a",@progbits
	.p2align	3, 0x0
double_logarithm.coefficients:
	.quad	0x3fe32ad540000000              # double 0.59897863864898682
	.quad	0x3feec45e20000000              # double 0.96147066354751587
	.quad	0x40071548a0000000              # double 2.885392427444458
	.size	double_logarithm.coefficients, 24

	.type	n32_exp_coefficients,@object    # @n32_exp_coefficients
	.p2align	3, 0x0
n32_exp_coefficients:
	.quad	0xbce952c77030ad4a              # double -2.8114572543455206E-15
	.quad	0x3d2ae7f3e733b81f              # double 4.7794773323873853E-14
	.quad	0xbd6ae7f3e733b81f              # double -7.6471637318198164E-13
	.quad	0x3da93974a8c07c9d              # double 1.1470745597729725E-11
	.quad	0xbde6124613a86d09              # double -1.6059043836821613E-10
	.quad	0x3e21eed8eff8d898              # double 2.08767569878681E-9
	.quad	0xbe5ae64567f544e4              # double -2.505210838544172E-8
	.quad	0x3e927e4fb7789f5c              # double 2.7557319223985888E-7
	.quad	0xbec71de3a556c734              # double -2.7557319223985893E-6
	.quad	0x3efa01a01a01a01a              # double 2.4801587301587302E-5
	.quad	0xbf2a01a01a01a01a              # double -1.9841269841269841E-4
	.quad	0x3f56c16c16c16c17              # double 0.0013888888888888889
	.quad	0xbf81111111111111              # double -0.0083333333333333332
	.quad	0x3fa5555555555555              # double 0.041666666666666664
	.quad	0xbfc5555555555555              # double -0.16666666666666666
	.quad	0x3fe0000000000000              # double 0.5
	.quad	0xbff0000000000000              # double -1
	.quad	0x3ff0000000000000              # double 1
	.size	n32_exp_coefficients, 144

	.type	double_sine.coefficients,@object # @double_sine.coefficients
	.p2align	3, 0x0
double_sine.coefficients:
	.quad	0x4043daf740000000              # double 39.710670471191406
	.quad	0xc05324cc80000000              # double -76.574981689453125
	.quad	0x4054668b00000000              # double 81.60223388671875
	.quad	0xc044abbc00000000              # double -41.3416748046875
	.quad	0x401921fb40000000              # double 6.2831850051879883
	.size	double_sine.coefficients, 40

	.type	double_arctangent.coefficients,@object # @double_arctangent.coefficients
	.p2align	3, 0x0
double_arctangent.coefficients:
	.quad	0x3f677ae940000000              # double 0.0028662257827818394
	.quad	0xbf908dc040000000              # double -0.016165737062692642
	.quad	0x3fa5f83fc0000000              # double 0.042909614741802216
	.quad	0xbfb3462e80000000              # double -0.075289636850357056
	.quad	0x3fbb47b080000000              # double 0.10656264424324036
	.quad	0xbfc22ff900000000              # double -0.14208900928497314
	.quad	0x3fc9977c80000000              # double 0.19993549585342407
	.quad	0xbfd5554d80000000              # double -0.33333146572113037
	.quad	0x3ff0000000000000              # double 1
	.size	double_arctangent.coefficients, 72

	.type	single_logarithm.coefficients,@object # @single_logarithm.coefficients
	.p2align	2, 0x0
single_logarithm.coefficients:
	.word	0x3f1956aa                      # float 0.598978639
	.word	0x3f7622f1                      # float 0.961470663
	.word	0x4038aa45                      # float 2.88539243
	.size	single_logarithm.coefficients, 12

	.type	single_exponential.coefficients,@object # @single_exponential.coefficients
	.p2align	2, 0x0
single_exponential.coefficients:
	.word	0xb9142e40                      # float -1.41316094E-4
	.word	0x3aae4f70                      # float 0.00132988207
	.word	0xbc08026e                      # float -0.00830136053
	.word	0x3d2aa0e6                      # float 0.041657351
	.word	0xbe2aaa50                      # float -0.166665316
	.word	0x3effffff                      # float 0.49999997
	.word	0xbf800000                      # float -1
	.word	0x3f800000                      # float 1
	.size	single_exponential.coefficients, 32

	.type	single_sine.coefficients,@object # @single_sine.coefficients
	.p2align	2, 0x0
single_sine.coefficients:
	.word	0x421ed7ba                      # float 39.7106705
	.word	0xc2992664                      # float -76.5749816
	.word	0x42a33458                      # float 81.6022339
	.word	0xc2255de0                      # float -41.3416748
	.word	0x40c90fda                      # float 6.283185
	.size	single_sine.coefficients, 20

	.type	single_arctangent.coefficients,@object # @single_arctangent.coefficients
	.p2align	2, 0x0
single_arctangent.coefficients:
	.word	0x3b3bd74a                      # float 0.00286622578
	.word	0xbc846e02                      # float -0.0161657371
	.word	0x3d2fc1fe                      # float 0.0429096147
	.word	0xbd9a3174                      # float -0.0752896368
	.word	0x3dda3d84                      # float 0.106562644
	.word	0xbe117fc8                      # float -0.142089009
	.word	0x3e4cbbe4                      # float 0.199935496
	.word	0xbeaaaa6c                      # float -0.333331466
	.word	0x3f800000                      # float 1
	.size	single_arctangent.coefficients, 36


.section .note.GNU-stack,"",@progbits
