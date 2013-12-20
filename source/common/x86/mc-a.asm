;*****************************************************************************
;* mc-a.asm: x86 motion compensation
;*****************************************************************************
;* Copyright (C) 2003-2013 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Jason Garrett-Glaser <darkshikari@gmail.com>
;*          Laurent Aimar <fenrir@via.ecp.fr>
;*          Dylan Yudaken <dyudaken@gmail.com>
;*          Holger Lubitz <holger@lubitz.org>
;*          Min Chen <chenm001@163.com>
;*          Oskar Arvidsson <oskar@irock.se>
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;*
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
;*
;* This program is also available under a commercial proprietary license.
;* For more information, contact us at licensing@x264.com.
;*****************************************************************************

%include "x86inc.asm"
%include "x86util.asm"

SECTION_RODATA 32

ch_shuf: times 2 db 0,2,2,4,4,6,6,8,1,3,3,5,5,7,7,9
ch_shuf_adj: times 8 db 0
             times 8 db 2
             times 8 db 4
             times 8 db 6
sq_1: times 1 dq 1

SECTION .text

cextern pb_0
cextern pw_1
cextern pw_4
cextern pw_8
cextern pw_32
cextern pw_64
cextern pw_512
cextern pw_00ff
cextern pw_pixel_max
cextern sw_64
cextern pd_32
cextern deinterleave_shufd

;=============================================================================
; implicit weighted biprediction
;=============================================================================
; assumes log2_denom = 5, offset = 0, weight1 + weight2 = 64
%if WIN64
    DECLARE_REG_TMP 0,1,2,3,4,5,4,5
    %macro AVG_START 0-1 0
        PROLOGUE 6,7,%1
    %endmacro
%elif UNIX64
    DECLARE_REG_TMP 0,1,2,3,4,5,7,8
    %macro AVG_START 0-1 0
        PROLOGUE 6,9,%1
    %endmacro
%else
    DECLARE_REG_TMP 1,2,3,4,5,6,1,2
    %macro AVG_START 0-1 0
        PROLOGUE 0,7,%1
        mov t0, r0m
        mov t1, r1m
        mov t2, r2m
        mov t3, r3m
        mov t4, r4m
        mov t5, r5m
    %endmacro
%endif

%macro AVG_END 0
    lea  t4, [t4+t5*2*SIZEOF_PIXEL]
    lea  t2, [t2+t3*2*SIZEOF_PIXEL]
    lea  t0, [t0+t1*2*SIZEOF_PIXEL]
    sub eax, 2
    jg .height_loop
 %ifidn movu,movq ; detect MMX
    EMMS
 %endif
    RET
%endmacro

%if HIGH_BIT_DEPTH

%macro BIWEIGHT_MMX 2
    movh      m0, %1
    movh      m1, %2
    punpcklwd m0, m1
    pmaddwd   m0, m3
    paddd     m0, m4
    psrad     m0, 6
%endmacro

%macro BIWEIGHT_START_MMX 0
    movzx  t6d, word r6m
    mov    t7d, 64
    sub    t7d, t6d
    shl    t7d, 16
    add    t6d, t7d
    movd    m3, t6d
    SPLATD  m3, m3
    mova    m4, [pd_32]
    pxor    m5, m5
%endmacro

%else ;!HIGH_BIT_DEPTH
%macro BIWEIGHT_MMX 2
    movh      m0, %1
    movh      m1, %2
    punpcklbw m0, m5
    punpcklbw m1, m5
    pmullw    m0, m2
    pmullw    m1, m3
    paddw     m0, m1
    paddw     m0, m4
    psraw     m0, 6
%endmacro

%macro BIWEIGHT_START_MMX 0
    movd    m2, r6m
    SPLATW  m2, m2   ; weight_dst
    mova    m3, [pw_64]
    psubw   m3, m2   ; weight_src
    mova    m4, [pw_32] ; rounding
    pxor    m5, m5
%endmacro
%endif ;HIGH_BIT_DEPTH

%macro BIWEIGHT_SSSE3 2
    movh      m0, %1
    movh      m1, %2
    punpcklbw m0, m1
    pmaddubsw m0, m3
    pmulhrsw  m0, m4
%endmacro

%macro BIWEIGHT_START_SSSE3 0
    movzx  t6d, byte r6m ; FIXME x86_64
    mov    t7d, 64
    sub    t7d, t6d
    shl    t7d, 8
    add    t6d, t7d
    mova    m4, [pw_512]
    movd   xm3, t6d
%if cpuflag(avx2)
    vpbroadcastw m3, xm3
%else
    SPLATW  m3, m3   ; weight_dst,src
%endif
%endmacro

%if HIGH_BIT_DEPTH
%macro BIWEIGHT_ROW 4
    BIWEIGHT   [%2], [%3]
%if %4==mmsize/4
    packssdw     m0, m0
    CLIPW        m0, m5, m7
    movh       [%1], m0
%else
    SWAP 0, 6
    BIWEIGHT   [%2+mmsize/2], [%3+mmsize/2]
    packssdw     m6, m0
    CLIPW        m6, m5, m7
    mova       [%1], m6
%endif
%endmacro

%else ;!HIGH_BIT_DEPTH
%macro BIWEIGHT_ROW 4
    BIWEIGHT [%2], [%3]
%if %4==mmsize/2
    packuswb   m0, m0
    movh     [%1], m0
%else
    SWAP 0, 6
    BIWEIGHT [%2+mmsize/2], [%3+mmsize/2]
    packuswb   m6, m0
%if %4 != 12
    mova    [%1], m6
%else ; !w12
    movh    [%1], m6
    movhlps m6, m6
    movd    [%1+mmsize/2], m6
%endif ; w12
%endif
%endmacro

%endif ;HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; int pixel_avg_weight_w16( pixel *dst, intptr_t, pixel *src1, intptr_t, pixel *src2, intptr_t, int i_weight )
;-----------------------------------------------------------------------------
%macro AVG_WEIGHT 1-2 0
cglobal pixel_avg_weight_w%1
    BIWEIGHT_START
    AVG_START %2
%if HIGH_BIT_DEPTH
    mova    m7, [pw_pixel_max]
%endif
.height_loop:
%if mmsize==16 && %1==mmsize/(2*SIZEOF_PIXEL)
    BIWEIGHT [t2], [t4]
    SWAP 0, 6
    BIWEIGHT [t2+SIZEOF_PIXEL*t3], [t4+SIZEOF_PIXEL*t5]
%if HIGH_BIT_DEPTH
    packssdw m6, m0
    CLIPW    m6, m5, m7
%else ;!HIGH_BIT_DEPTH
    packuswb m6, m0
%endif ;HIGH_BIT_DEPTH
    movlps   [t0], m6
    movhps   [t0+SIZEOF_PIXEL*t1], m6
%else
%assign x 0
%rep (%1*SIZEOF_PIXEL+mmsize-1)/mmsize
%assign y mmsize
%if (%1 == 12) && (%1*SIZEOF_PIXEL-x < mmsize)
%assign y (%1*SIZEOF_PIXEL-x)
%endif
    BIWEIGHT_ROW   t0+x,                   t2+x,                   t4+x,                 y
    BIWEIGHT_ROW   t0+x+SIZEOF_PIXEL*t1,   t2+x+SIZEOF_PIXEL*t3,   t4+x+SIZEOF_PIXEL*t5, y
%assign x x+mmsize
%endrep
%endif
    AVG_END
%endmacro

%define BIWEIGHT BIWEIGHT_MMX
%define BIWEIGHT_START BIWEIGHT_START_MMX
INIT_MMX mmx2
AVG_WEIGHT 4
AVG_WEIGHT 8
AVG_WEIGHT 12
AVG_WEIGHT 16
AVG_WEIGHT 32
AVG_WEIGHT 64
AVG_WEIGHT 24
AVG_WEIGHT 48
%if HIGH_BIT_DEPTH
INIT_XMM sse2
AVG_WEIGHT 4,  8
AVG_WEIGHT 8,  8
AVG_WEIGHT 16, 8
%else ;!HIGH_BIT_DEPTH
INIT_XMM sse2
AVG_WEIGHT 8,  7
AVG_WEIGHT 12, 7
AVG_WEIGHT 16, 7
AVG_WEIGHT 32, 7
AVG_WEIGHT 64, 7
AVG_WEIGHT 24, 7
AVG_WEIGHT 48, 7
%define BIWEIGHT BIWEIGHT_SSSE3
%define BIWEIGHT_START BIWEIGHT_START_SSSE3
INIT_MMX ssse3
AVG_WEIGHT 4
INIT_XMM ssse3
AVG_WEIGHT 8,  7
AVG_WEIGHT 12, 7
AVG_WEIGHT 16, 7
AVG_WEIGHT 32, 7
AVG_WEIGHT 64, 7
AVG_WEIGHT 24, 7
AVG_WEIGHT 48, 7

INIT_YMM avx2
cglobal pixel_avg_weight_w16
    BIWEIGHT_START
    AVG_START 5
.height_loop:
    movu     xm0, [t2]
    movu     xm1, [t4]
    vinserti128 m0, m0, [t2+t3], 1
    vinserti128 m1, m1, [t4+t5], 1
    SBUTTERFLY bw, 0, 1, 2
    pmaddubsw m0, m3
    pmaddubsw m1, m3
    pmulhrsw  m0, m4
    pmulhrsw  m1, m4
    packuswb  m0, m1
    mova    [t0], xm0
    vextracti128 [t0+t1], m0, 1
    AVG_END
%endif ;HIGH_BIT_DEPTH

;=============================================================================
; P frame explicit weighted prediction
;=============================================================================

%if HIGH_BIT_DEPTH
; width
%macro WEIGHT_START 1
    mova        m0, [r4+ 0]         ; 1<<denom
    mova        m3, [r4+16]
    movd        m2, [r4+32]         ; denom
    mova        m4, [pw_pixel_max]
    paddw       m2, [sq_1]          ; denom+1
%endmacro

; src1, src2
%macro WEIGHT 2
    movh        m5, [%1]
    movh        m6, [%2]
    punpcklwd   m5, m0
    punpcklwd   m6, m0
    pmaddwd     m5, m3
    pmaddwd     m6, m3
    psrad       m5, m2
    psrad       m6, m2
    packssdw    m5, m6
%endmacro

; src, dst, width
%macro WEIGHT_TWO_ROW 4
    %assign x 0
%rep (%3+mmsize/2-1)/(mmsize/2)
%if %3-x/2 <= 4 && mmsize == 16
    WEIGHT      %1+x, %1+r3+x
    CLIPW         m5, [pb_0], m4
    movh      [%2+x], m5
    movhps [%2+r1+x], m5
%else
    WEIGHT      %1+x, %1+x+mmsize/2
    SWAP           5,  7
    WEIGHT   %1+r3+x, %1+r3+x+mmsize/2
    CLIPW         m5, [pb_0], m4
    CLIPW         m7, [pb_0], m4
    mova      [%2+x], m7
    mova   [%2+r1+x], m5
%endif
    %assign x x+mmsize
%endrep
%endmacro

%else ; !HIGH_BIT_DEPTH

%macro WEIGHT_START 1
%if cpuflag(avx2)
    vbroadcasti128 m3, [r4]
    vbroadcasti128 m4, [r4+16]
%else
    mova     m3, [r4]
    mova     m4, [r4+16]
%if notcpuflag(ssse3)
    movd     m5, [r4+32]
%endif
%endif
    pxor     m2, m2
%endmacro

; src1, src2, dst1, dst2, fast
%macro WEIGHT_ROWx2 5
    movh      m0, [%1         ]
    movh      m1, [%1+mmsize/2]
    movh      m6, [%2         ]
    movh      m7, [%2+mmsize/2]
    punpcklbw m0, m2
    punpcklbw m1, m2
    punpcklbw m6, m2
    punpcklbw m7, m2
%if cpuflag(ssse3)
%if %5==0
    psllw     m0, 7
    psllw     m1, 7
    psllw     m6, 7
    psllw     m7, 7
%endif
    pmulhrsw  m0, m3
    pmulhrsw  m1, m3
    pmulhrsw  m6, m3
    pmulhrsw  m7, m3
    paddw     m0, m4
    paddw     m1, m4
    paddw     m6, m4
    paddw     m7, m4
%else
    pmullw    m0, m3
    pmullw    m1, m3
    pmullw    m6, m3
    pmullw    m7, m3
    paddsw    m0, m4        ;1<<(denom-1)+(offset<<denom)
    paddsw    m1, m4
    paddsw    m6, m4
    paddsw    m7, m4
    psraw     m0, m5
    psraw     m1, m5
    psraw     m6, m5
    psraw     m7, m5
%endif
    packuswb  m0, m1
    packuswb  m6, m7
    mova    [%3], m0
    mova    [%4], m6
%endmacro

; src1, src2, dst1, dst2, width, fast
%macro WEIGHT_COL 6
%if cpuflag(avx2)
%if %5==16
    movu     xm0, [%1]
    vinserti128 m0, m0, [%2], 1
    punpckhbw m1, m0, m2
    punpcklbw m0, m0, m2
%if %6==0
    psllw     m0, 7
    psllw     m1, 7
%endif
    pmulhrsw  m0, m3
    pmulhrsw  m1, m3
    paddw     m0, m4
    paddw     m1, m4
    packuswb  m0, m1
    mova    [%3], xm0
    vextracti128 [%4], m0, 1
%else
    movq     xm0, [%1]
    vinserti128 m0, m0, [%2], 1
    punpcklbw m0, m2
%if %6==0
    psllw     m0, 7
%endif
    pmulhrsw  m0, m3
    paddw     m0, m4
    packuswb  m0, m0
    vextracti128 xm1, m0, 1
%if %5 == 8
    movq    [%3], xm0
    movq    [%4], xm1
%else
    movd    [%3], xm0
    movd    [%4], xm1
%endif
%endif
%else
    movh      m0, [%1]
    movh      m1, [%2]
    punpcklbw m0, m2
    punpcklbw m1, m2
%if cpuflag(ssse3)
%if %6==0
    psllw     m0, 7
    psllw     m1, 7
%endif
    pmulhrsw  m0, m3
    pmulhrsw  m1, m3
    paddw     m0, m4
    paddw     m1, m4
%else
    pmullw    m0, m3
    pmullw    m1, m3
    paddsw    m0, m4        ;1<<(denom-1)+(offset<<denom)
    paddsw    m1, m4
    psraw     m0, m5
    psraw     m1, m5
%endif
%if %5 == 8
    packuswb  m0, m1
    movh    [%3], m0
    movhps  [%4], m0
%else
    packuswb  m0, m0
    packuswb  m1, m1
    movd    [%3], m0    ; width 2 can write garbage for the last 2 bytes
    movd    [%4], m1
%endif
%endif
%endmacro
; src, dst, width
%macro WEIGHT_TWO_ROW 4
%assign x 0
%rep %3
%if (%3-x) >= mmsize
    WEIGHT_ROWx2 %1+x, %1+r3+x, %2+x, %2+r1+x, %4
    %assign x (x+mmsize)
%else
    %assign w %3-x
%if w == 20
    %assign w 16
%endif
    WEIGHT_COL %1+x, %1+r3+x, %2+x, %2+r1+x, w, %4
    %assign x (x+w)
%endif
%if x >= %3
    %exitrep
%endif
%endrep
%endmacro

%endif ; HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
;void mc_weight_wX( pixel *dst, intptr_t i_dst_stride, pixel *src, intptr_t i_src_stride, weight_t *weight, int h )
;-----------------------------------------------------------------------------

%macro WEIGHTER 1
cglobal mc_weight_w%1, 6,6,8
    FIX_STRIDES r1, r3
    WEIGHT_START %1
%if cpuflag(ssse3) && HIGH_BIT_DEPTH == 0
    ; we can merge the shift step into the scale factor
    ; if (m3<<7) doesn't overflow an int16_t
    cmp byte [r4+1], 0
    jz .fast
%endif
.loop:
    WEIGHT_TWO_ROW r2, r0, %1, 0
    lea  r0, [r0+r1*2]
    lea  r2, [r2+r3*2]
    sub r5d, 2
    jg .loop
    RET
%if cpuflag(ssse3) && HIGH_BIT_DEPTH == 0
.fast:
    psllw m3, 7
.fastloop:
    WEIGHT_TWO_ROW r2, r0, %1, 1
    lea  r0, [r0+r1*2]
    lea  r2, [r2+r3*2]
    sub r5d, 2
    jg .fastloop
    RET
%endif
%endmacro

INIT_MMX mmx2
WEIGHTER  4
WEIGHTER  8
WEIGHTER 12
WEIGHTER 16
WEIGHTER 20
INIT_XMM sse2
WEIGHTER  8
WEIGHTER 16
WEIGHTER 20
%if HIGH_BIT_DEPTH
WEIGHTER 12
%else
INIT_MMX ssse3
WEIGHTER  4
INIT_XMM ssse3
WEIGHTER  8
WEIGHTER 16
WEIGHTER 20
INIT_YMM avx2
WEIGHTER 8
WEIGHTER 16
WEIGHTER 20
%endif

%macro OFFSET_OP 7
    mov%6        m0, [%1]
    mov%6        m1, [%2]
%if HIGH_BIT_DEPTH
    p%5usw       m0, m2
    p%5usw       m1, m2
%ifidn %5,add
    pminsw       m0, m3
    pminsw       m1, m3
%endif
%else
    p%5usb       m0, m2
    p%5usb       m1, m2
%endif
    mov%7      [%3], m0
    mov%7      [%4], m1
%endmacro

%macro OFFSET_TWO_ROW 4
%assign x 0
%rep %3
%if (%3*SIZEOF_PIXEL-x) >= mmsize
    OFFSET_OP (%1+x), (%1+x+r3), (%2+x), (%2+x+r1), %4, u, a
    %assign x (x+mmsize)
%else
%if HIGH_BIT_DEPTH
    OFFSET_OP (%1+x), (%1+x+r3), (%2+x), (%2+x+r1), %4, h, h
%else
    OFFSET_OP (%1+x), (%1+x+r3), (%2+x), (%2+x+r1), %4, d, d
%endif
    %exitrep
%endif
%if x >= %3*SIZEOF_PIXEL
    %exitrep
%endif
%endrep
%endmacro

;-----------------------------------------------------------------------------
;void mc_offset_wX( pixel *src, intptr_t i_src_stride, pixel *dst, intptr_t i_dst_stride, weight_t *w, int h )
;-----------------------------------------------------------------------------
%macro OFFSET 2
cglobal mc_offset%2_w%1, 6,6
    FIX_STRIDES r1, r3
    mova m2, [r4]
%if HIGH_BIT_DEPTH
%ifidn %2,add
    mova m3, [pw_pixel_max]
%endif
%endif
.loop:
    OFFSET_TWO_ROW r2, r0, %1, %2
    lea  r0, [r0+r1*2]
    lea  r2, [r2+r3*2]
    sub r5d, 2
    jg .loop
    RET
%endmacro

%macro OFFSETPN 1
       OFFSET %1, add
       OFFSET %1, sub
%endmacro
INIT_MMX mmx2
OFFSETPN  4
OFFSETPN  8
OFFSETPN 12
OFFSETPN 16
OFFSETPN 20
INIT_XMM sse2
OFFSETPN 12
OFFSETPN 16
OFFSETPN 20
%if HIGH_BIT_DEPTH
INIT_XMM sse2
OFFSETPN  8
%endif


;=============================================================================
; pixel avg
;=============================================================================

;-----------------------------------------------------------------------------
; void pixel_avg_4x4( pixel *dst, intptr_t dst_stride, pixel *src1, intptr_t src1_stride,
;                     pixel *src2, intptr_t src2_stride, int weight );
;-----------------------------------------------------------------------------
%macro AVGH 2
cglobal pixel_avg_%1x%2
    mov eax, %2
    cmp dword r6m, 32
    jne pixel_avg_weight_w%1 %+ SUFFIX
%if cpuflag(avx2) && %1 == 16 ; all AVX2 machines can do fast 16-byte unaligned loads
    jmp pixel_avg_w%1_avx2
%else
%if mmsize == 16 && (%1 % 16 == 0)
    test dword r4m, 15
    jz pixel_avg_w%1_sse2
%endif
    jmp pixel_avg_w%1_mmx2
%endif
%endmacro

;-----------------------------------------------------------------------------
; void pixel_avg_w4( pixel *dst, intptr_t dst_stride, pixel *src1, intptr_t src1_stride,
;                    pixel *src2, intptr_t src2_stride, int height, int weight );
;-----------------------------------------------------------------------------

%macro AVG_FUNC 3-4
cglobal pixel_avg_w%1
    AVG_START
.height_loop:
%assign x 0
%rep (%1*SIZEOF_PIXEL+mmsize-1)/mmsize
    %2     m0, [t2+x]
    %2     m1, [t2+x+SIZEOF_PIXEL*t3]
%if HIGH_BIT_DEPTH
    pavgw  m0, [t4+x]
    pavgw  m1, [t4+x+SIZEOF_PIXEL*t5]
%else ;!HIGH_BIT_DEPTH
    pavgb  m0, [t4+x]
    pavgb  m1, [t4+x+SIZEOF_PIXEL*t5]
%endif
%if (%1 == 12) && (%1-x/SIZEOF_PIXEL < mmsize)
    %4     [t0+x], m0
    %4     [t0+x+SIZEOF_PIXEL*t1], m1
%else
    %3     [t0+x], m0
    %3     [t0+x+SIZEOF_PIXEL*t1], m1
%endif
%assign x x+mmsize
%endrep
    AVG_END
%endmacro

%if HIGH_BIT_DEPTH

INIT_MMX mmx2
AVG_FUNC 4, movq, movq
AVGH 4, 16
AVGH 4, 8
AVGH 4, 4
AVGH 4, 2

AVG_FUNC 8, movq, movq
AVGH 8, 16
AVGH 8,  8
AVGH 8,  4

AVG_FUNC 16, movq, movq
AVGH 16, 16
AVGH 16,  8

INIT_XMM sse2
AVG_FUNC 4, movq, movq
AVGH  4, 16
AVGH  4, 8
AVGH  4, 4
AVGH  4, 2

AVG_FUNC 8, movdqu, movdqa
AVGH  8, 16
AVGH  8,  8
AVGH  8,  4

AVG_FUNC 16, movdqu, movdqa
AVGH  16, 16
AVGH  16,  8

%else ;!HIGH_BIT_DEPTH

INIT_MMX mmx2
AVG_FUNC 4, movd, movd
AVGH 4, 16
AVGH 4, 8
AVGH 4, 4
AVGH 4, 2

AVG_FUNC 8, movq, movq
AVGH 8, 32
AVGH 8, 16
AVGH 8,  8
AVGH 8,  4

AVG_FUNC 12, movq, movq, movd
AVGH 12, 16

AVG_FUNC 16, movq, movq
AVGH 16, 64
AVGH 16, 32
AVGH 16, 16
AVGH 16, 12
AVGH 16, 8
AVGH 16, 4

AVG_FUNC 32, movq, movq
AVGH 32, 32
AVGH 32, 24
AVGH 32, 16
AVGH 32, 8

AVG_FUNC 64, movq, movq
AVGH 64, 64
AVGH 64, 48
AVGH 64, 16

AVG_FUNC 24, movq, movq
AVGH 24, 32

AVG_FUNC 48, movq, movq
AVGH 48, 64

INIT_XMM sse2
AVG_FUNC 64, movdqu, movdqa
AVGH 64, 64
AVGH 64, 48
AVGH 64, 16

AVG_FUNC 32, movdqu, movdqa
AVGH 32, 64
AVGH 32, 32
AVGH 32, 24
AVGH 32, 16
AVGH 32, 8

AVG_FUNC 24, movdqu, movdqa
AVGH 24, 32

AVG_FUNC 16, movdqu, movdqa
AVGH 16, 64
AVGH 16, 32
AVGH 16, 16
AVGH 16, 12
AVGH 16, 8
AVGH 16, 4

AVG_FUNC 48, movdqu, movdqa
AVGH 48, 64

AVG_FUNC 12, movdqu, movdqa, movq
AVGH 12, 16

AVGH  8, 32
AVGH  8, 16
AVGH  8,  8
AVGH  8,  4
INIT_XMM ssse3
AVGH 24, 32

AVGH 64, 64
AVGH 64, 48
AVGH 64, 16

AVGH 32, 64
AVGH 32, 32
AVGH 32, 24
AVGH 32, 16
AVGH 32, 8

AVGH 16, 64
AVGH 16, 32
AVGH 16, 16
AVGH 16, 12
AVGH 16, 8
AVGH 16, 4

AVGH 48, 64

AVGH 12, 16

AVGH  8, 32
AVGH  8, 16
AVGH  8,  8
AVGH  8,  4
INIT_MMX ssse3
AVGH  4, 16
AVGH  4,  8
AVGH  4,  4
AVGH  4,  2
INIT_XMM avx2
; TODO: active AVX2 after debug
;AVG_FUNC 24, movdqu, movdqa
;AVGH 24, 32

;AVG_FUNC 64, movdqu, movdqa
;AVGH 64, 64
;AVGH 64, 48
;AVGH 64, 16

;AVG_FUNC 32, movdqu, movdqa
;AVGH 32, 64
;AVGH 32, 32
;AVGH 32, 24
;AVGH 32, 16
;AVGH 32, 8
AVG_FUNC 16, movdqu, movdqa
AVGH 16, 16
AVGH 16,  8

%endif ;HIGH_BIT_DEPTH



;=============================================================================
; pixel avg2
;=============================================================================

%if HIGH_BIT_DEPTH
;-----------------------------------------------------------------------------
; void pixel_avg2_wN( uint16_t *dst,  intptr_t dst_stride,
;                     uint16_t *src1, intptr_t src_stride,
;                     uint16_t *src2, int height );
;-----------------------------------------------------------------------------
%macro AVG2_W_ONE 1
cglobal pixel_avg2_w%1, 6,7,4
    sub     r4, r2
    lea     r6, [r4+r3*2]
.height_loop:
    movu    m0, [r2]
    movu    m1, [r2+r3*2]
%if cpuflag(avx) || mmsize == 8
    pavgw   m0, [r2+r4]
    pavgw   m1, [r2+r6]
%else
    movu    m2, [r2+r4]
    movu    m3, [r2+r6]
    pavgw   m0, m2
    pavgw   m1, m3
%endif
    mova   [r0], m0
    mova   [r0+r1*2], m1
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
    sub    r5d, 2
    jg .height_loop
    RET
%endmacro

%macro AVG2_W_TWO 3
cglobal pixel_avg2_w%1, 6,7,8
    sub     r4, r2
    lea     r6, [r4+r3*2]
.height_loop:
    movu    m0, [r2]
    %2      m1, [r2+mmsize]
    movu    m2, [r2+r3*2]
    %2      m3, [r2+r3*2+mmsize]
%if mmsize == 8
    pavgw   m0, [r2+r4]
    pavgw   m1, [r2+r4+mmsize]
    pavgw   m2, [r2+r6]
    pavgw   m3, [r2+r6+mmsize]
%else
    movu    m4, [r2+r4]
    %2      m5, [r2+r4+mmsize]
    movu    m6, [r2+r6]
    %2      m7, [r2+r6+mmsize]
    pavgw   m0, m4
    pavgw   m1, m5
    pavgw   m2, m6
    pavgw   m3, m7
%endif
    mova   [r0], m0
    %3     [r0+mmsize], m1
    mova   [r0+r1*2], m2
    %3     [r0+r1*2+mmsize], m3
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
    sub    r5d, 2
    jg .height_loop
    RET
%endmacro

INIT_MMX mmx2
AVG2_W_ONE  4
AVG2_W_TWO  8, movu, mova
INIT_XMM sse2
AVG2_W_ONE  8
AVG2_W_TWO 10, movd, movd
AVG2_W_TWO 16, movu, mova
INIT_YMM avx2
AVG2_W_ONE 16

INIT_MMX
cglobal pixel_avg2_w10_mmx2, 6,7
    sub     r4, r2
    lea     r6, [r4+r3*2]
.height_loop:
    movu    m0, [r2+ 0]
    movu    m1, [r2+ 8]
    movh    m2, [r2+16]
    movu    m3, [r2+r3*2+ 0]
    movu    m4, [r2+r3*2+ 8]
    movh    m5, [r2+r3*2+16]
    pavgw   m0, [r2+r4+ 0]
    pavgw   m1, [r2+r4+ 8]
    pavgw   m2, [r2+r4+16]
    pavgw   m3, [r2+r6+ 0]
    pavgw   m4, [r2+r6+ 8]
    pavgw   m5, [r2+r6+16]
    mova   [r0+ 0], m0
    mova   [r0+ 8], m1
    movh   [r0+16], m2
    mova   [r0+r1*2+ 0], m3
    mova   [r0+r1*2+ 8], m4
    movh   [r0+r1*2+16], m5
    lea     r2, [r2+r3*2*2]
    lea     r0, [r0+r1*2*2]
    sub    r5d, 2
    jg .height_loop
    RET

cglobal pixel_avg2_w16_mmx2, 6,7
    sub     r4, r2
    lea     r6, [r4+r3*2]
.height_loop:
    movu    m0, [r2+ 0]
    movu    m1, [r2+ 8]
    movu    m2, [r2+16]
    movu    m3, [r2+24]
    movu    m4, [r2+r3*2+ 0]
    movu    m5, [r2+r3*2+ 8]
    movu    m6, [r2+r3*2+16]
    movu    m7, [r2+r3*2+24]
    pavgw   m0, [r2+r4+ 0]
    pavgw   m1, [r2+r4+ 8]
    pavgw   m2, [r2+r4+16]
    pavgw   m3, [r2+r4+24]
    pavgw   m4, [r2+r6+ 0]
    pavgw   m5, [r2+r6+ 8]
    pavgw   m6, [r2+r6+16]
    pavgw   m7, [r2+r6+24]
    mova   [r0+ 0], m0
    mova   [r0+ 8], m1
    mova   [r0+16], m2
    mova   [r0+24], m3
    mova   [r0+r1*2+ 0], m4
    mova   [r0+r1*2+ 8], m5
    mova   [r0+r1*2+16], m6
    mova   [r0+r1*2+24], m7
    lea     r2, [r2+r3*2*2]
    lea     r0, [r0+r1*2*2]
    sub    r5d, 2
    jg .height_loop
    RET

cglobal pixel_avg2_w18_mmx2, 6,7
    sub     r4, r2
.height_loop:
    movu    m0, [r2+ 0]
    movu    m1, [r2+ 8]
    movu    m2, [r2+16]
    movu    m3, [r2+24]
    movh    m4, [r2+32]
    pavgw   m0, [r2+r4+ 0]
    pavgw   m1, [r2+r4+ 8]
    pavgw   m2, [r2+r4+16]
    pavgw   m3, [r2+r4+24]
    pavgw   m4, [r2+r4+32]
    mova   [r0+ 0], m0
    mova   [r0+ 8], m1
    mova   [r0+16], m2
    mova   [r0+24], m3
    movh   [r0+32], m4
    lea     r2, [r2+r3*2]
    lea     r0, [r0+r1*2]
    dec    r5d
    jg .height_loop
    RET

%macro PIXEL_AVG_W18 0
cglobal pixel_avg2_w18, 6,7
    sub     r4, r2
.height_loop:
    movu    m0, [r2+ 0]
    movd   xm2, [r2+32]
%if mmsize == 32
    pavgw   m0, [r2+r4+ 0]
    movd   xm1, [r2+r4+32]
    pavgw  xm2, xm1
%else
    movu    m1, [r2+16]
    movu    m3, [r2+r4+ 0]
    movu    m4, [r2+r4+16]
    movd    m5, [r2+r4+32]
    pavgw   m0, m3
    pavgw   m1, m4
    pavgw   m2, m5
    mova   [r0+16], m1
%endif
    mova   [r0+ 0], m0
    movd   [r0+32], xm2
    lea     r2, [r2+r3*2]
    lea     r0, [r0+r1*2]
    dec    r5d
    jg .height_loop
    RET
%endmacro

INIT_XMM sse2
PIXEL_AVG_W18
INIT_YMM avx2
PIXEL_AVG_W18

%endif ; HIGH_BIT_DEPTH

%if HIGH_BIT_DEPTH == 0
;-----------------------------------------------------------------------------
; void pixel_avg2_w4( uint8_t *dst,  intptr_t dst_stride,
;                     uint8_t *src1, intptr_t src_stride,
;                     uint8_t *src2, int height );
;-----------------------------------------------------------------------------
%macro AVG2_W8 2
cglobal pixel_avg2_w%1_mmx2, 6,7
    sub    r4, r2
    lea    r6, [r4+r3]
.height_loop:
    %2     mm0, [r2]
    %2     mm1, [r2+r3]
    pavgb  mm0, [r2+r4]
    pavgb  mm1, [r2+r6]
    lea    r2, [r2+r3*2]
    %2     [r0], mm0
    %2     [r0+r1], mm1
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    RET
%endmacro

INIT_MMX
AVG2_W8 4, movd
AVG2_W8 8, movq

%macro AVG2_W16 2
cglobal pixel_avg2_w%1_mmx2, 6,7
    sub    r2, r4
    lea    r6, [r2+r3]
.height_loop:
    movq   mm0, [r4]
    %2     mm1, [r4+8]
    movq   mm2, [r4+r3]
    %2     mm3, [r4+r3+8]
    pavgb  mm0, [r4+r2]
    pavgb  mm1, [r4+r2+8]
    pavgb  mm2, [r4+r6]
    pavgb  mm3, [r4+r6+8]
    lea    r4, [r4+r3*2]
    movq   [r0], mm0
    %2     [r0+8], mm1
    movq   [r0+r1], mm2
    %2     [r0+r1+8], mm3
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    RET
%endmacro

AVG2_W16 12, movd
AVG2_W16 16, movq

cglobal pixel_avg2_w20_mmx2, 6,7
    sub    r2, r4
    lea    r6, [r2+r3]
.height_loop:
    movq   mm0, [r4]
    movq   mm1, [r4+8]
    movd   mm2, [r4+16]
    movq   mm3, [r4+r3]
    movq   mm4, [r4+r3+8]
    movd   mm5, [r4+r3+16]
    pavgb  mm0, [r4+r2]
    pavgb  mm1, [r4+r2+8]
    pavgb  mm2, [r4+r2+16]
    pavgb  mm3, [r4+r6]
    pavgb  mm4, [r4+r6+8]
    pavgb  mm5, [r4+r6+16]
    lea    r4, [r4+r3*2]
    movq   [r0], mm0
    movq   [r0+8], mm1
    movd   [r0+16], mm2
    movq   [r0+r1], mm3
    movq   [r0+r1+8], mm4
    movd   [r0+r1+16], mm5
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    RET

INIT_XMM
cglobal pixel_avg2_w16_sse2, 6,7
    sub    r4, r2
    lea    r6, [r4+r3]
.height_loop:
    movu   m0, [r2]
    movu   m2, [r2+r3]
    movu   m1, [r2+r4]
    movu   m3, [r2+r6]
    lea    r2, [r2+r3*2]
    pavgb  m0, m1
    pavgb  m2, m3
    mova [r0], m0
    mova [r0+r1], m2
    lea    r0, [r0+r1*2]
    sub   r5d, 2
    jg .height_loop
    RET

cglobal pixel_avg2_w20_sse2, 6,7
    sub    r2, r4
    lea    r6, [r2+r3]
.height_loop:
    movu   m0, [r4]
    movu   m2, [r4+r3]
    movu   m1, [r4+r2]
    movu   m3, [r4+r6]
    movd  mm4, [r4+16]
    movd  mm5, [r4+r3+16]
    pavgb  m0, m1
    pavgb  m2, m3
    pavgb mm4, [r4+r2+16]
    pavgb mm5, [r4+r6+16]
    lea    r4, [r4+r3*2]
    mova [r0], m0
    mova [r0+r1], m2
    movd [r0+16], mm4
    movd [r0+r1+16], mm5
    lea    r0, [r0+r1*2]
    sub   r5d, 2
    jg .height_loop
    RET

INIT_YMM avx2
cglobal pixel_avg2_w20, 6,7
    sub    r2, r4
    lea    r6, [r2+r3]
.height_loop:
    movu   m0, [r4]
    movu   m1, [r4+r3]
    pavgb  m0, [r4+r2]
    pavgb  m1, [r4+r6]
    lea    r4, [r4+r3*2]
    mova [r0], m0
    mova [r0+r1], m1
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    RET

; Cacheline split code for processors with high latencies for loads
; split over cache lines.  See sad-a.asm for a more detailed explanation.
; This particular instance is complicated by the fact that src1 and src2
; can have different alignments.  For simplicity and code size, only the
; MMX cacheline workaround is used.  As a result, in the case of SSE2
; pixel_avg, the cacheline check functions calls the SSE2 version if there
; is no cacheline split, and the MMX workaround if there is.

%macro INIT_SHIFT 2
    and    eax, 7
    shl    eax, 3
    movd   %1, [sw_64]
    movd   %2, eax
    psubw  %1, %2
%endmacro

%macro AVG_CACHELINE_START 0
    %assign stack_offset 0
    INIT_SHIFT mm6, mm7
    mov    eax, r4m
    INIT_SHIFT mm4, mm5
    PROLOGUE 6,6
    and    r2, ~7
    and    r4, ~7
    sub    r4, r2
.height_loop:
%endmacro

%macro AVG_CACHELINE_LOOP 2
    movq   mm1, [r2+%1]
    movq   mm0, [r2+8+%1]
    movq   mm3, [r2+r4+%1]
    movq   mm2, [r2+r4+8+%1]
    psrlq  mm1, mm7
    psllq  mm0, mm6
    psrlq  mm3, mm5
    psllq  mm2, mm4
    por    mm0, mm1
    por    mm2, mm3
    pavgb  mm2, mm0
    %2 [r0+%1], mm2
%endmacro

%macro AVG_CACHELINE_FUNC 2
pixel_avg2_w%1_cache_mmx2:
    AVG_CACHELINE_START
    AVG_CACHELINE_LOOP 0, movq
%if %1>8
    AVG_CACHELINE_LOOP 8, movq
%if %1>16
    AVG_CACHELINE_LOOP 16, movd
%endif
%endif
    add    r2, r3
    add    r0, r1
    dec    r5d
    jg .height_loop
    RET
%endmacro

%macro AVG_CACHELINE_CHECK 3 ; width, cacheline, instruction set
%if %1 == 12
;w12 isn't needed because w16 is just as fast if there's no cacheline split
%define cachesplit pixel_avg2_w16_cache_mmx2
%else
%define cachesplit pixel_avg2_w%1_cache_mmx2
%endif
cglobal pixel_avg2_w%1_cache%2_%3
    mov    eax, r2m
    and    eax, %2-1
    cmp    eax, (%2-%1-(%1 % 8))
%if %1==12||%1==20
    jbe pixel_avg2_w%1_%3
%else
    jb pixel_avg2_w%1_%3
%endif
%if 0 ; or %1==8 - but the extra branch seems too expensive
    ja cachesplit
%if ARCH_X86_64
    test      r4b, 1
%else
    test byte r4m, 1
%endif
    jz pixel_avg2_w%1_%3
%else
    or     eax, r4m
    and    eax, 7
    jz pixel_avg2_w%1_%3
    mov    eax, r2m
%endif
%if mmsize==16 || (%1==8 && %2==64)
    AVG_CACHELINE_FUNC %1, %2
%else
    jmp cachesplit
%endif
%endmacro

INIT_MMX
AVG_CACHELINE_CHECK  8, 64, mmx2
AVG_CACHELINE_CHECK 12, 64, mmx2
%if ARCH_X86_64 == 0
AVG_CACHELINE_CHECK 16, 64, mmx2
AVG_CACHELINE_CHECK 20, 64, mmx2
AVG_CACHELINE_CHECK  8, 32, mmx2
AVG_CACHELINE_CHECK 12, 32, mmx2
AVG_CACHELINE_CHECK 16, 32, mmx2
AVG_CACHELINE_CHECK 20, 32, mmx2
%endif
INIT_XMM
AVG_CACHELINE_CHECK 16, 64, sse2
AVG_CACHELINE_CHECK 20, 64, sse2

; computed jump assumes this loop is exactly 48 bytes
%macro AVG16_CACHELINE_LOOP_SSSE3 2 ; alignment
ALIGN 16
avg_w16_align%1_%2_ssse3:
%if %1==0 && %2==0
    movdqa  xmm1, [r2]
    pavgb   xmm1, [r2+r4]
    add    r2, r3
%elif %1==0
    movdqa  xmm1, [r2+r4+16]
    palignr xmm1, [r2+r4], %2
    pavgb   xmm1, [r2]
    add    r2, r3
%elif %2&15==0
    movdqa  xmm1, [r2+16]
    palignr xmm1, [r2], %1
    pavgb   xmm1, [r2+r4]
    add    r2, r3
%else
    movdqa  xmm1, [r2+16]
    movdqa  xmm2, [r2+r4+16]
    palignr xmm1, [r2], %1
    palignr xmm2, [r2+r4], %2&15
    add    r2, r3
    pavgb   xmm1, xmm2
%endif
    movdqa  [r0], xmm1
    add    r0, r1
    dec    r5d
    jg     avg_w16_align%1_%2_ssse3
    ret
%if %1==0
    ; make sure the first ones don't end up short
    ALIGN 16
    times (48-($-avg_w16_align%1_%2_ssse3))>>4 nop
%endif
%endmacro

cglobal pixel_avg2_w16_cache64_ssse3
%if 0 ; seems both tests aren't worth it if src1%16==0 is optimized
    mov   eax, r2m
    and   eax, 0x3f
    cmp   eax, 0x30
    jb x264_pixel_avg2_w16_sse2
    or    eax, r4m
    and   eax, 7
    jz x264_pixel_avg2_w16_sse2
%endif
    PROLOGUE 6, 8
    lea    r6, [r4+r2]
    and    r4, ~0xf
    and    r6, 0x1f
    and    r2, ~0xf
    lea    r6, [r6*3]    ;(offset + align*2)*3
    sub    r4, r2
    shl    r6, 4         ;jump = (offset + align*2)*48
%define avg_w16_addr avg_w16_align1_1_ssse3-(avg_w16_align2_2_ssse3-avg_w16_align1_1_ssse3)
%ifdef PIC
    lea    r7, [avg_w16_addr]
    add    r6, r7
%else
    lea    r6, [avg_w16_addr + r6]
%endif
    TAIL_CALL r6, 1

%assign j 0
%assign k 1
%rep 16
AVG16_CACHELINE_LOOP_SSSE3 j, j
AVG16_CACHELINE_LOOP_SSSE3 j, k
%assign j j+1
%assign k k+1
%endrep
%endif ; !HIGH_BIT_DEPTH

;=============================================================================
; pixel copy
;=============================================================================

%macro COPY1 2
    movu  m0, [r2]
    movu  m1, [r2+r3]
    movu  m2, [r2+r3*2]
    movu  m3, [r2+%2]
    mova  [r0],      m0
    mova  [r0+r1],   m1
    mova  [r0+r1*2], m2
    mova  [r0+%1],   m3
%endmacro

%macro COPY2 2-4 0, 1
    movu  m0, [r2+%3*mmsize]
    movu  m1, [r2+%4*mmsize]
    movu  m2, [r2+r3+%3*mmsize]
    movu  m3, [r2+r3+%4*mmsize]
    mova  [r0+%3*mmsize],      m0
    mova  [r0+%4*mmsize],      m1
    mova  [r0+r1+%3*mmsize],   m2
    mova  [r0+r1+%4*mmsize],   m3
    movu  m0, [r2+r3*2+%3*mmsize]
    movu  m1, [r2+r3*2+%4*mmsize]
    movu  m2, [r2+%2+%3*mmsize]
    movu  m3, [r2+%2+%4*mmsize]
    mova  [r0+r1*2+%3*mmsize], m0
    mova  [r0+r1*2+%4*mmsize], m1
    mova  [r0+%1+%3*mmsize],   m2
    mova  [r0+%1+%4*mmsize],   m3
%endmacro

%macro COPY4 2
    COPY2 %1, %2, 0, 1
    COPY2 %1, %2, 2, 3
%endmacro

;-----------------------------------------------------------------------------
; void mc_copy_w4( uint8_t *dst, intptr_t i_dst_stride,
;                  uint8_t *src, intptr_t i_src_stride, int i_height )
;-----------------------------------------------------------------------------
INIT_MMX
cglobal mc_copy_w4_mmx, 4,6
    FIX_STRIDES r1, r3
    cmp dword r4m, 4
    lea     r5, [r3*3]
    lea     r4, [r1*3]
    je .end
%if HIGH_BIT_DEPTH == 0
    %define mova movd
    %define movu movd
%endif
    COPY1   r4, r5
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
.end:
    COPY1   r4, r5
    RET

%macro MC_COPY 1
%assign %%w %1*SIZEOF_PIXEL/mmsize
%if %%w > 0
cglobal mc_copy_w%1, 5,7
    FIX_STRIDES r1, r3
    lea     r6, [r3*3]
    lea     r5, [r1*3]
.height_loop:
    COPY %+ %%w r5, r6
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
    sub    r4d, 4
    jg .height_loop
    RET
%endif
%endmacro

INIT_MMX mmx
MC_COPY  8
MC_COPY 16
INIT_XMM sse
MC_COPY  8
MC_COPY 16
INIT_XMM aligned, sse
MC_COPY 16
%if HIGH_BIT_DEPTH
INIT_YMM avx
MC_COPY 16
INIT_YMM aligned, avx
MC_COPY 16
%endif

;=============================================================================
; prefetch
;=============================================================================
; assumes 64 byte cachelines
; FIXME doesn't cover all pixels in high depth and/or 4:4:4

;-----------------------------------------------------------------------------
; void prefetch_fenc( pixel *pix_y,  intptr_t stride_y,
;                     pixel *pix_uv, intptr_t stride_uv, int mb_x )
;-----------------------------------------------------------------------------

%macro PREFETCH_FENC 1
%if ARCH_X86_64
cglobal prefetch_fenc_%1, 5,5
    FIX_STRIDES r1, r3
    and    r4d, 3
    mov    eax, r4d
    imul   r4d, r1d
    lea    r0,  [r0+r4*4+64*SIZEOF_PIXEL]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]
    lea    r0,  [r0+r1*2]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]

    imul   eax, r3d
    lea    r2,  [r2+rax*2+64*SIZEOF_PIXEL]
    prefetcht0  [r2]
    prefetcht0  [r2+r3]
%ifidn %1, 422
    lea    r2,  [r2+r3*2]
    prefetcht0  [r2]
    prefetcht0  [r2+r3]
%endif
    RET

%else
cglobal prefetch_fenc_%1, 0,3
    mov    r2, r4m
    mov    r1, r1m
    mov    r0, r0m
    FIX_STRIDES r1
    and    r2, 3
    imul   r2, r1
    lea    r0, [r0+r2*4+64*SIZEOF_PIXEL]
    prefetcht0 [r0]
    prefetcht0 [r0+r1]
    lea    r0, [r0+r1*2]
    prefetcht0 [r0]
    prefetcht0 [r0+r1]

    mov    r2, r4m
    mov    r1, r3m
    mov    r0, r2m
    FIX_STRIDES r1
    and    r2, 3
    imul   r2, r1
    lea    r0, [r0+r2*2+64*SIZEOF_PIXEL]
    prefetcht0 [r0]
    prefetcht0 [r0+r1]
%ifidn %1, 422
    lea    r0,  [r0+r1*2]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]
%endif
    ret
%endif ; ARCH_X86_64
%endmacro

INIT_MMX mmx2
PREFETCH_FENC 420
PREFETCH_FENC 422

;-----------------------------------------------------------------------------
; void prefetch_ref( pixel *pix, intptr_t stride, int parity )
;-----------------------------------------------------------------------------
INIT_MMX mmx2
cglobal prefetch_ref, 3,3
    FIX_STRIDES r1
    dec    r2d
    and    r2d, r1d
    lea    r0,  [r0+r2*8+64*SIZEOF_PIXEL]
    lea    r2,  [r1*3]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]
    prefetcht0  [r0+r1*2]
    prefetcht0  [r0+r2]
    lea    r0,  [r0+r1*4]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]
    prefetcht0  [r0+r1*2]
    prefetcht0  [r0+r2]
    RET
