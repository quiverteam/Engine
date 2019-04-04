//========= Copyright Valve Corporation, All rights reserved. ============//
/******************************************************************************

 Copyright (c) 1999 Advanced Micro Devices, Inc.

 LIMITATION OF LIABILITY:  THE MATERIALS ARE PROVIDED *AS IS* WITHOUT ANY
 EXPRESS OR IMPLIED WARRANTY OF ANY KIND INCLUDING WARRANTIES OF MERCHANTABILITY,
 NONINFRINGEMENT OF THIRD-PARTY INTELLECTUAL PROPERTY, OR FITNESS FOR ANY
 PARTICULAR PURPOSE.  IN NO EVENT SHALL AMD OR ITS SUPPLIERS BE LIABLE FOR ANY
 DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF PROFITS,
 BUSINESS INTERRUPTION, LOSS OF INFORMATION) ARISING OUT OF THE USE OF OR
 INABILITY TO USE THE MATERIALS, EVEN IF AMD HAS BEEN ADVISED OF THE POSSIBILITY
 OF SUCH DAMAGES.  BECAUSE SOME JURISDICTIONS PROHIBIT THE EXCLUSION OR LIMITATION
 OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES, THE ABOVE LIMITATION MAY
 NOT APPLY TO YOU.

 AMD does not assume any responsibility for any errors which may appear in the
 Materials nor any responsibility to support or update the Materials.  AMD retains
 the right to make changes to its test specifications at any time, without notice.

 NO SUPPORT OBLIGATION: AMD is not obligated to furnish, support, or make any
 further information, software, technical information, know-how, or show-how
 available to you.

 So that all may benefit from your experience, please report  any  problems
 or  suggestions about this software to 3dsdk.support@amd.com

 AMD Developer Technologies, M/S 585
 Advanced Micro Devices, Inc.
 5900 E. Ben White Blvd.
 Austin, TX 78741
 3dsdk.support@amd.com

*******************************************************************************

 AMD3DX.H

 MACRO FORMAT
 ============
 This file contains inline assembly macros that
 generate AMD-3D instructions in binary format.
 Therefore, C or C++ programmer can use AMD-3D instructions
 without any penalty in their C or C++ source code.

 The macro's name and format conventions are as follow:


 1. First argument of macro is a destination and
    second argument is a source operand.
      ex) _asm PFCMPEQ (mm3, mm4)
                         |    |
                        dst  src

 2. The destination operand can be m0 to m7 only.
    The source operand can be any one of the register
    m0 to m7 or _eax, _ecx, _edx, _ebx, _esi, or _edi
    that contains effective address.
      ex) _asm PFRCP    (MM7, MM6)
      ex) _asm PFRCPIT2 (mm0, mm4)
      ex) _asm PFMUL    (mm3, _edi)

  3. The prefetch(w) takes one src operand _eax, ecx, _edx,
     _ebx, _esi, or _edi that contains effective address.
      ex) _asm PREFETCH (_edi)

 For WATCOM C/C++ users, when using #pragma aux instead if 
 _asm, all macro names should be prefixed by a p_ or P_. 
 Macros should not be enclosed in quotes.
              ex) p_pfrcp (MM7,MM6)

 NOTE: Not all instruction macros, nor all possible
       combinations of operands have been explicitely
       tested. If any errors are found, please report
       them.

 EXAMPLE
 =======
 Following program doesn't do anything but it shows you
 how to use inline assembly AMD-3D instructions in C.
 Note that this will only work in flat memory model which
 segment registers cs, ds, ss and es point to the same
 linear address space total less than 4GB.

 Used Microsoft VC++ 5.0

 #include <stdio.h>
 #include "amd3d.h"

 void main ()
 {
      float x = (float)1.25;
      float y = (float)1.25;
      float z, zz;

     _asm {
              movd mm1, x
              movd mm2, y
              pfmul (mm1, mm2)
              movd z, mm1
              femms
      }

      printf ("value of z = %f\n", z);

      //
      // Demonstration of using the memory instead of
      // multimedia register
      //
      _asm {
              movd mm3, x
              lea esi, y   // load effective address of y
              pfmul (mm3, _esi)
              movd zz, mm3
              femms
      }

      printf ("value of zz = %f\n", zz);
  }

 #pragma aux EXAMPLE with WATCOM C/C++ v11.x
 ===========================================

    extern void Add(float *__Dest, float *__A, float *__B);
    #pragma aux Add =               \
            p_femms                 \
            "movd mm6,[esi]"        \
            p_pfadd(mm6,_edi)       \
            "movd [ebx],mm6"        \
            p_femms                 \
            parm [ebx] [esi] [edi];

*******************************************************************************/

#ifndef _K3DMACROSINCLUDED_
#define _K3DMACROSINCLUDED_

#if defined (_MSC_VER) && !defined (__MWERKS__)
// The Microsoft Visual C++ version of the 3DNow! macros.

// Stop the "no EMMS" warning, since it doesn't detect FEMMS properly
#pragma warning(disable:4799)

// Defines for operands.
#define _K3D_MM0 0xc0
#define _K3D_MM1 0xc1
#define _K3D_MM2 0xc2
#define _K3D_MM3 0xc3
#define _K3D_MM4 0xc4
#define _K3D_MM5 0xc5
#define _K3D_MM6 0xc6
#define _K3D_MM7 0xc7
#define _K3D_mm0 0xc0
#define _K3D_mm1 0xc1
#define _K3D_mm2 0xc2
#define _K3D_mm3 0xc3
#define _K3D_mm4 0xc4
#define _K3D_mm5 0xc5
#define _K3D_mm6 0xc6
#define _K3D_mm7 0xc7
#define _K3D_EAX 0x00
#define _K3D_ECX 0x01
#define _K3D_EDX 0x02
#define _K3D_EBX 0x03
#define _K3D_ESI 0x06
#define _K3D_EDI 0x07
#define _K3D_eax 0x00
#define _K3D_ecx 0x01
#define _K3D_edx 0x02
#define _K3D_ebx 0x03
#define _K3D_esi 0x06
#define _K3D_edi 0x07

// These defines are for compatibility with the previous version of the header file.
#define _K3D_M0   0xc0
#define _K3D_M1   0xc1
#define _K3D_M2   0xc2
#define _K3D_M3   0xc3
#define _K3D_M4   0xc4
#define _K3D_M5   0xc5
#define _K3D_M6   0xc6
#define _K3D_M7   0xc7
#define _K3D_m0   0xc0
#define _K3D_m1   0xc1
#define _K3D_m2   0xc2
#define _K3D_m3   0xc3
#define _K3D_m4   0xc4
#define _K3D_m5   0xc5
#define _K3D_m6   0xc6
#define _K3D_m7   0xc7
#define _K3D__EAX 0x00
#define _K3D__ECX 0x01
#define _K3D__EDX 0x02
#define _K3D__EBX 0x03
#define _K3D__ESI 0x06
#define _K3D__EDI 0x07
#define _K3D__eax 0x00
#define _K3D__ecx 0x01
#define _K3D__edx 0x02
#define _K3D__ebx 0x03
#define _K3D__esi 0x06
#define _K3D__edi 0x07

// General 3DNow! instruction format that is supported by 
// these macros. Note that only the most basic form of memory 
// operands are supported by these macros. 

#define InjK3DOps(dst,src,inst)                         \
{                                                       \
   _asm _emit 0x0f                                      \
   _asm _emit 0x0f                                      \
   _asm _emit ((_K3D_##dst & 0x3f) << 3) | _K3D_##src   \
   _asm _emit _3DNowOpcode##inst                        \
}

#define InjK3DMOps(dst,src,off,inst)                    \
{                                                       \
   _asm _emit 0x0f                                      \
   _asm _emit 0x0f                                      \
   _asm _emit (((_K3D_##dst & 0x3f) << 3) | _K3D_##src | 0x40) \
   _asm _emit off                                       \
   _asm _emit _3DNowOpcode##inst                        \
}

#define InjMMXOps(dst,src,inst)                         \
{                                                       \
   _asm _emit 0x0f                                      \
   _asm _emit _3DNowOpcode##inst                        \
   _asm _emit ((_K3D_##dst & 0x3f) << 3) | _K3D_##src   \
}

#define InjMMXMOps(dst,src,off,inst)                    \
{                                                       \
   _asm _emit 0x0f                                      \
   _asm _emit _3DNowOpcode##inst                        \
   _asm _emit (((_K3D_##dst & 0x3f) << 3) | _K3D_##src | 0x40) \
   _asm _emit off                                       \
}

#define _3DNowOpcodePF2ID    0x1d
#define _3DNowOpcodePFACC    0xae
#define _3DNowOpcodePFADD    0x9e
#define _3DNowOpcodePFCMPEQ  0xb0
#define _3DNowOpcodePFCMPGE  0x90
#define _3DNowOpcodePFCMPGT  0xa0
#define _3DNowOpcodePFMAX    0xa4
#define _3DNowOpcodePFMIN    0x94
#define _3DNowOpcodePFMUL    0xb4
#define _3DNowOpcodePFRCP    0x96
#define _3DNowOpcodePFRCPIT1 0xa6
#define _3DNowOpcodePFRCPIT2 0xb6
#define _3DNowOpcodePFRSQRT  0x97
#define _3DNowOpcodePFRSQIT1 0xa7
#define _3DNowOpcodePFSUB    0x9a
#define _3DNowOpcodePFSUBR   0xaa
#define _3DNowOpcodePI2FD    0x0d
#define _3DNowOpcodePAVGUSB  0xbf
#define _3DNowOpcodePMULHRW  0xb7
#define _3DNowOpcodePFNACC   0x8a
#define _3DNowOpcodeFPPNACC  0x8e
#define _3DNowOpcodePSWAPD   0xbb
#define _3DNowOpcodePMINUB   0xda
#define _3DNowOpcodePMAXUB   0xde
#define _3DNowOpcodePMINSW   0xea
#define _3DNowOpcodePMAXSW   0xee
#define _3DNowOpcodePMULHUW  0xe4
#define _3DNowOpcodePAVGB    0xe0
#define _3DNowOpcodePAVGW    0xe3
#define _3DNowOpcodePSADBW   0xf6
#define _3DNowOpcodePMOVMSKB 0xd7
#define _3DNowOpcodePMASKMOVQ   0xf7
#define _3DNowOpcodePINSRW   0xc4
#define _3DNowOpcodePEXTRW   0xc5
#define _3DNowOpcodePSHUFW   0x70
#define _3DNowOpcodeMOVNTQ   0xe7
#define _3DNowOpcodePREFETCHT 0x18


#define PF2ID(dst,src)      InjK3DOps(dst, src, PF2ID)
#define PFACC(dst,src)      InjK3DOps(dst, src, PFACC)
#define PFADD(dst,src)      InjK3DOps(dst, src, PFADD)
#define PFCMPEQ(dst,src)    InjK3DOps(dst, src, PFCMPEQ)
#define PFCMPGE(dst,src)    InjK3DOps(dst, src, PFCMPGE)
#define PFCMPGT(dst,src)    InjK3DOps(dst, src, PFCMPGT)
#define PFMAX(dst,src)      InjK3DOps(dst, src, PFMAX)
#define PFMIN(dst,src)      InjK3DOps(dst, src, PFMIN)
#define PFMUL(dst,src)      InjK3DOps(dst, src, PFMUL)
#define PFRCP(dst,src)      InjK3DOps(dst, src, PFRCP)
#define PFRCPIT1(dst,src)   InjK3DOps(dst, src, PFRCPIT1)
#define PFRCPIT2(dst,src)   InjK3DOps(dst, src, PFRCPIT2)
#define PFRSQRT(dst,src)    InjK3DOps(dst, src, PFRSQRT)
#define PFRSQIT1(dst,src)   InjK3DOps(dst, src, PFRSQIT1)
#define PFSUB(dst,src)      InjK3DOps(dst, src, PFSUB)
#define PFSUBR(dst,src)     InjK3DOps(dst, src, PFSUBR)
#define PI2FD(dst,src)      InjK3DOps(dst, src, PI2FD)
#define PAVGUSB(dst,src)    InjK3DOps(dst, src, PAVGUSB)
#define PMULHRW(dst,src)    InjK3DOps(dst, src, PMULHRW)

#define FEMMS                                   \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0e                              \
}

#define PREFETCH(src)                           \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d                              \
   _asm _emit (_K3D_##src & 0x07)               \
}

/* Prefetch with a short offset, < 127 or > -127
   Carefull!  Doesn't check for your offset being
   in range. */

#define PREFETCHM(src,off)					    \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d								\
   _asm _emit (0x40 | (_K3D_##src & 0x07))		\
   _asm _emit off								\
}

/* Prefetch with a long offset */

#define PREFETCHMLONG(src,off)					\
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d								\
   _asm _emit (0x80 | (_K3D_##src & 0x07))		\
   _asm _emit (off & 0x000000ff)				\
   _asm _emit (off & 0x0000ff00) >>	8			\
   _asm _emit (off & 0x00ff0000) >>	16			\
   _asm _emit (off & 0xff000000) >>	24			\
}

#define PREFETCHW(src)                          \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d                              \
   _asm _emit (0x08 | (_K3D_##src & 0x07))      \
}

#define PREFETCHWM(src,off)                     \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d                              \
   _asm _emit 0x48 | (_K3D_##src & 0x07)        \
   _asm	_emit off								\
}

#define PREFETCHWMLONG(src,off)                 \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d                              \
   _asm _emit 0x88 | (_K3D_##src & 0x07)        \
   _asm _emit (off & 0x000000ff)				\
   _asm _emit (off & 0x0000ff00) >>	8			\
   _asm _emit (off & 0x00ff0000) >>	16			\
   _asm _emit (off & 0xff000000) >>	24			\
}

#define CPUID                                   \
{                                               \
    _asm _emit 0x0f                             \
    _asm _emit 0xa2                             \
}


/* Defines for new, K7 opcodes */
#define SFENCE                                  \
{                                               \
    _asm _emit 0x0f                             \
    _asm _emit 0xae                             \
    _asm _emit 0xf8                             \
}

#define PFNACC(dst,src)         InjK3DOps(dst,src,PFNACC)
#define PFPNACC(dst,src)        InjK3DOps(dst,src,PFPNACC)
#define PSWAPD(dst,src)         InjK3DOps(dst,src,PSWAPD)
#define PMINUB(dst,src)         InjMMXOps(dst,src,PMINUB)
#define PMAXUB(dst,src)         InjMMXOps(dst,src,PMAXUB)
#define PMINSW(dst,src)         InjMMXOps(dst,src,PMINSW)
#define PMAXSW(dst,src)         InjMMXOps(dst,src,PMAXSW)
#define PMULHUW(dst,src)        InjMMXOps(dst,src,PMULHUW)
#define PAVGB(dst,src)          InjMMXOps(dst,src,PAVGB)
#define PAVGW(dst,src)          InjMMXOps(dst,src,PAVGW)
#define PSADBW(dst,src)         InjMMXOps(dst,src,PSADBW)
#define PMOVMSKB(dst,src)       InjMMXOps(dst,src,PMOVMSKB)
#define PMASKMOVQ(dst,src)      InjMMXOps(dst,src,PMASKMOVQ)
#define PINSRW(dst,src,msk)     InjMMXOps(dst,src,PINSRW) _asm _emit msk
#define PEXTRW(dst,src,msk)     InjMMXOps(dst,src,PEXTRW) _asm _emit msk
#define PSHUFW(dst,src,msk)     InjMMXOps(dst,src,PSHUFW) _asm _emit msk
#define MOVNTQ(dst,src)         InjMMXOps(src,dst,MOVNTQ)
#define PREFETCHNTA(mem)        InjMMXOps(mm0,mem,PREFETCHT)
#define PREFETCHT0(mem)         InjMMXOps(mm1,mem,PREFETCHT)
#define PREFETCHT1(mem)         InjMMXOps(mm2,mem,PREFETCHT)
#define PREFETCHT2(mem)         InjMMXOps(mm3,mem,PREFETCHT)


/* Memory/offset versions of the opcodes */
#define PAVGUSBM(dst,src,off)   InjK3DMOps(dst,src,off,PAVGUSB)
#define PF2IDM(dst,src,off)     InjK3DMOps(dst,src,off,PF2ID)
#define PFACCM(dst,src,off)     InjK3DMOps(dst,src,off,PFACC)
#define PFADDM(dst,src,off)     InjK3DMOps(dst,src,off,PFADD)
#define PFCMPEQM(dst,src,off)   InjK3DMOps(dst,src,off,PFCMPEQ)
#define PFCMPGEM(dst,src,off)   InjK3DMOps(dst,src,off,PFCMPGE)
#define PFCMPGTM(dst,src,off)   InjK3DMOps(dst,src,off,PFCMPGT)
#define PFMAXM(dst,src,off)     InjK3DMOps(dst,src,off,PFMAX)
#define PFMINM(dst,src,off)     InjK3DMOps(dst,src,off,PFMIN)
#define PFMULM(dst,src,off)     InjK3DMOps(dst,src,off,PFMUL)
#define PFRCPM(dst,src,off)     InjK3DMOps(dst,src,off,PFRCP)
#define PFRCPIT1M(dst,src,off)  InjK3DMOps(dst,src,off,PFRCPIT1)
#define PFRCPIT2M(dst,src,off)  InjK3DMOps(dst,src,off,PFRCPIT2)
#define PFRSQRTM(dst,src,off)   InjK3DMOps(dst,src,off,PFRSQRT)
#define PFRSQIT1M(dst,src,off)  InjK3DMOps(dst,src,off,PFRSQIT1)
#define PFSUBM(dst,src,off)     InjK3DMOps(dst,src,off,PFSUB)
#define PFSUBRM(dst,src,off)    InjK3DMOps(dst,src,off,PFSUBR)
#define PI2FDM(dst,src,off)     InjK3DMOps(dst,src,off,PI2FD)
#define PMULHRWM(dst,src,off)   InjK3DMOps(dst,src,off,PMULHRW)


/* Memory/offset versions of the K7 opcodes */
#define PFNACCM(dst,src,off)     InjK3DMOps(dst,src,off,PFNACC)
#define PFPNACCM(dst,src,off)    InjK3DMOps(dst,src,off,PFPNACC)
#define PSWAPDM(dst,src,off)     InjK3DMOps(dst,src,off,PSWAPD)
#define PMINUBM(dst,src,off)     InjMMXMOps(dst,src,off,PMINUB)
#define PMAXUBM(dst,src,off)     InjMMXMOps(dst,src,off,PMAXUB)
#define PMINSWM(dst,src,off)     InjMMXMOps(dst,src,off,PMINSW)
#define PMAXSWM(dst,src,off)     InjMMXMOps(dst,src,off,PMAXSW)
#define PMULHUWM(dst,src,off)    InjMMXMOps(dst,src,off,PMULHUW)
#define PAVGBM(dst,src,off)      InjMMXMOps(dst,src,off,PAVGB)
#define PAVGWM(dst,src,off)      InjMMXMOps(dst,src,off,PAVGW)
#define PSADBWM(dst,src,off)     InjMMXMOps(dst,src,off,PSADBW)
#define PMOVMSKBM(dst,src,off)   InjMMXMOps(dst,src,off,PMOVMSKB)
#define PMASKMOVQM(dst,src,off)  InjMMXMOps(dst,src,off,PMASKMOVQ)
#define PINSRWM(dst,src,off,msk) InjMMXMOps(dst,src,off,PINSRW) _asm _emit msk
#define PSHUFWM(dst,src,off,msk) InjMMXMOps(dst,src,off,PSHUFW) _asm _emit msk
#define MOVNTQM(dst,src,off)     InjMMXMOps(src,dst,off,MOVNTQ)
#define PREFETCHNTAM(mem,off)    InjMMXMOps(mm0,mem,off,PREFETCHT)
#define PREFETCHT0M(mem,off)     InjMMXMOps(mm1,mem,off,PREFETCHT)
#define PREFETCHT1M(mem,off)     InjMMXMOps(mm2,mem,off,PREFETCHT)
#define PREFETCHT2M(mem,off)     InjMMXMOps(mm3,mem,off,PREFETCHT)


#else

/* Assume built-in support for 3DNow! opcodes, replace macros with opcodes */
#define PAVGUSB(dst,src)    pavgusb     dst,src
#define PF2ID(dst,src)      pf2id       dst,src
#define PFACC(dst,src)      pfacc       dst,src
#define PFADD(dst,src)      pfadd       dst,src
#define PFCMPEQ(dst,src)    pfcmpeq     dst,src
#define PFCMPGE(dst,src)    pfcmpge     dst,src
#define PFCMPGT(dst,src)    pfcmpgt     dst,src
#define PFMAX(dst,src)      pfmax       dst,src
#define PFMIN(dst,src)      pfmin       dst,src
#define PFMUL(dst,src)      pfmul       dst,src
#define PFRCP(dst,src)      pfrcp       dst,src
#define PFRCPIT1(dst,src)   pfrcpit1    dst,src
#define PFRCPIT2(dst,src)   pfrcpit2    dst,src
#define PFRSQRT(dst,src)    pfrsqrt     dst,src
#define PFRSQIT1(dst,src)   pfrsqit1    dst,src
#define PFSUB(dst,src)      pfsub       dst,src
#define PFSUBR(dst,src)     pfsubr      dst,src
#define PI2FD(dst,src)      pi2fd       dst,src
#define PMULHRW(dst,src)    pmulhrw     dst,src
#define PREFETCH(src)       prefetch    src
#define PREFETCHW(src)      prefetchw   src

#define PAVGUSBM(dst,src,off)   pavgusb     dst,[src+off]
#define PF2IDM(dst,src,off)     PF2ID       dst,[src+off]
#define PFACCM(dst,src,off)     PFACC       dst,[src+off]
#define PFADDM(dst,src,off)     PFADD       dst,[src+off]
#define PFCMPEQM(dst,src,off)   PFCMPEQ     dst,[src+off]
#define PFCMPGEM(dst,src,off)   PFCMPGE     dst,[src+off]
#define PFCMPGTM(dst,src,off)   PFCMPGT     dst,[src+off]
#define PFMAXM(dst,src,off)     PFMAX       dst,[src+off]
#define PFMINM(dst,src,off)     PFMIN       dst,[src+off]
#define PFMULM(dst,src,off)     PFMUL       dst,[src+off]
#define PFRCPM(dst,src,off)     PFRCP       dst,[src+off]
#define PFRCPIT1M(dst,src,off)  PFRCPIT1    dst,[src+off]
#define PFRCPIT2M(dst,src,off)  PFRCPIT2    dst,[src+off]
#define PFRSQRTM(dst,src,off)   PFRSQRT     dst,[src+off]
#define PFRSQIT1M(dst,src,off)  PFRSQIT1    dst,[src+off]
#define PFSUBM(dst,src,off)     PFSUB       dst,[src+off]
#define PFSUBRM(dst,src,off)    PFSUBR      dst,[src+off]
#define PI2FDM(dst,src,off)     PI2FD       dst,[src+off]
#define PMULHRWM(dst,src,off)   PMULHRW     dst,[src+off]


#if defined (__MWERKS__)
// At the moment, CodeWarrior does not support these opcodes, so hand-assemble them

// Defines for operands.
#define _K3D_MM0 0xc0
#define _K3D_MM1 0xc1
#define _K3D_MM2 0xc2
#define _K3D_MM3 0xc3
#define _K3D_MM4 0xc4
#define _K3D_MM5 0xc5
#define _K3D_MM6 0xc6
#define _K3D_MM7 0xc7
#define _K3D_mm0 0xc0
#define _K3D_mm1 0xc1
#define _K3D_mm2 0xc2
#define _K3D_mm3 0xc3
#define _K3D_mm4 0xc4
#define _K3D_mm5 0xc5
#define _K3D_mm6 0xc6
#define _K3D_mm7 0xc7
#define _K3D_EAX 0x00
#define _K3D_ECX 0x01
#define _K3D_EDX 0x02
#define _K3D_EBX 0x03
#define _K3D_ESI 0x06
#define _K3D_EDI 0x07
#define _K3D_eax 0x00
#define _K3D_ecx 0x01
#define _K3D_edx 0x02
#define _K3D_ebx 0x03
#define _K3D_esi 0x06
#define _K3D_edi 0x07
#define _K3D_EAX 0x00
#define _K3D_ECX 0x01
#define _K3D_EDX 0x02
#define _K3D_EBX 0x03
#define _K3D_ESI 0x06
#define _K3D_EDI 0x07
#define _K3D_eax 0x00
#define _K3D_ecx 0x01
#define _K3D_edx 0x02
#define _K3D_ebx 0x03
#define _K3D_esi 0x06
#define _K3D_edi 0x07

#define InjK3DOps(dst,src,inst) \
    db 0x0f, 0x0f, (((_K3D_##dst & 0x3f) << 3) | _K3D_##src), _3DNowOpcode##inst

#define InjK3DMOps(dst,src,off,inst) \
    db 0x0f, 0x0f, (((_K3D_##dst & 0x3f) << 3) | _K3D_##src | 0x40), off, _3DNowOpcode##inst

#define InjMMXOps(dst,src,inst)                     \
    db 0x0f, _3DNowOpcode##inst, (((_K3D_##dst & 0x3f) << 3) | _K3D_##src)

#define InjMMXMOps(dst,src,off,inst)                \
    db 0x0f, _3DNowOpcode##inst, (((_K3D_##dst & 0x3f) << 3) | _K3D_##src | 0x40), off

#define PFNACC(dst,src)         InjK3DOps(dst,src,PFNACC)
#define PFPNACC(dst,src)        InjK3DOps(dst,src,PFPNACC)
#define PSWAPD(dst,src)         InjK3DOps(dst,src,PSWAPD)
#define PMINUB(dst,src)         InjMMXOps(dst,src,PMINUB)
#define PMAXUB(dst,src)         InjMMXOps(dst,src,PMAXUB)
#define PMINSW(dst,src)         InjMMXOps(dst,src,PMINSW)
#define PMAXSW(dst,src)         InjMMXOps(dst,src,PMAXSW)
#define PMULHUW(dst,src)        InjMMXOps(dst,src,PMULHUW)
#define PAVGB(dst,src)          InjMMXOps(dst,src,PAVGB)
#define PAVGW(dst,src)          InjMMXOps(dst,src,PAVGW)
#define PSADBW(dst,src)         InjMMXOps(dst,src,PSADBW)
#define PMOVMSKB(dst,src)       InjMMXOps(dst,src,PMOVMSKB)
#define PMASKMOVQ(dst,src)      InjMMXOps(dst,src,PMASKMOVQ)
#define PINSRW(dst,src,msk)     InjMMXOps(dst,src,PINSRW) db msk
#define PEXTRW(dst,src,msk)     InjMMXOps(dst,src,PEXTRW) db msk
#define PSHUFW(dst,src,msk)     InjMMXOps(dst,src,PSHUFW) db msk
#define MOVNTQ(dst,src)         InjMMXOps(src,dst,MOVNTQ)
#define PREFETCHNTA(mem)        InjMMXOps(mm0,mem,PREFETCHT)
#define PREFETCHT0(mem)         InjMMXOps(mm1,mem,PREFETCHT)
#define PREFETCHT1(mem)         InjMMXOps(mm2,mem,PREFETCHT)
#define PREFETCHT2(mem)         InjMMXOps(mm3,mem,PREFETCHT)


/* Memory/offset versions of the K7 opcodes */
#define PFNACCM(dst,src,off)     InjK3DMOps(dst,src,off,PFNACC)
#define PFPNACCM(dst,src,off)    InjK3DMOps(dst,src,off,PFPNACC)
#define PSWAPDM(dst,src,off)     InjK3DMOps(dst,src,off,PSWAPD)
#define PMINUBM(dst,src,off)     InjMMXMOps(dst,src,off,PMINUB)
#define PMAXUBM(dst,src,off)     InjMMXMOps(dst,src,off,PMAXUB)
#define PMINSWM(dst,src,off)     InjMMXMOps(dst,src,off,PMINSW)
#define PMAXSWM(dst,src,off)     InjMMXMOps(dst,src,off,PMAXSW)
#define PMULHUWM(dst,src,off)    InjMMXMOps(dst,src,off,PMULHUW)
#define PAVGBM(dst,src,off)      InjMMXMOps(dst,src,off,PAVGB)
#define PAVGWM(dst,src,off)      InjMMXMOps(dst,src,off,PAVGW)
#define PSADBWM(dst,src,off)     InjMMXMOps(dst,src,off,PSADBW)
#define PMOVMSKBM(dst,src,off)   InjMMXMOps(dst,src,off,PMOVMSKB)
#define PMASKMOVQM(dst,src,off)  InjMMXMOps(dst,src,off,PMASKMOVQ)
#define PINSRWM(dst,src,off,msk) InjMMXMOps(dst,src,off,PINSRW), msk
#define PEXTRWM(dst,src,off,msk) InjMMXMOps(dst,src,off,PEXTRW), msk
#define PSHUFWM(dst,src,off,msk) InjMMXMOps(dst,src,off,PSHUFW), msk
#define MOVNTQM(dst,src,off)     InjMMXMOps(src,dst,off,MOVNTQ)
#define PREFETCHNTAM(mem,off)    InjMMXMOps(mm0,mem,off,PREFETCHT)
#define PREFETCHT0M(mem,off)     InjMMXMOps(mm1,mem,off,PREFETCHT)
#define PREFETCHT1M(mem,off)     InjMMXMOps(mm2,mem,off,PREFETCHT)
#define PREFETCHT2M(mem,off)     InjMMXMOps(mm3,mem,off,PREFETCHT)


#else

#define PFNACC(dst,src)         PFNACC      dst,src
#define PFPNACC(dst,src)        PFPNACC     dst,src
#define PSWAPD(dst,src)         PSWAPD      dst,src
#define PMINUB(dst,src)         PMINUB      dst,src
#define PMAXUB(dst,src)         PMAXUB      dst,src
#define PMINSW(dst,src)         PMINSW      dst,src
#define PMAXSW(dst,src)         PMAXSW      dst,src
#define PMULHUW(dst,src)        PMULHUW     dst,src
#define PAVGB(dst,src)          PAVGB       dst,src
#define PAVGW(dst,src)          PAVGW       dst,src
#define PSADBW(dst,src)         PSADBW      dst,src
#define PMOVMSKB(dst,src)       PMOVMSKB    dst,src
#define PMASKMOVQ(dst,src)      PMASKMOVQ   dst,src
#define PINSRW(dst,src,msk)     PINSRW      dst,src,msk
#define PEXTRW(dst,src,msk)     PEXTRW      dst,src,msk
#define PSHUFW(dst,src,msk)     PSHUFW      dst,src,msk
#define MOVNTQ(dst,src)         MOVNTQ      dst,src

#define PFNACCM(dst,src,off)    PFNACC      dst,[src+off]
#define PFPNACCM(dst,src,off)   PFPNACC     dst,[src+off]
#define PSWAPDM(dst,src,off)    PSWAPD      dst,[src+off]
#define PMINUBM(dst,src,off)    PMINUB      dst,[src+off]
#define PMAXUBM(dst,src,off)    PMAXUB      dst,[src+off]
#define PMINSWM(dst,src,off)    PMINSW      dst,[src+off]
#define PMAXSWM(dst,src,off)    PMAXSW      dst,[src+off]
#define PMULHUWM(dst,src,off)   PMULHUW     dst,[src+off]
#define PAVGBM(dst,src,off)     PAVGB       dst,[src+off]
#define PAVGWM(dst,src,off)     PAVGW       dst,[src+off]
#define PSADBWM(dst,src,off)    PSADBW      dst,[src+off]
#define PMOVMSKBM(dst,src,off)  PMOVMSKB    dst,[src+off]
#define PMASKMOVQM(dst,src,off) PMASKMOVQ   dst,[src+off]
#define PINSRWM(dst,src,off,msk) PINSRW     dst,[src+off],msk
#define PEXTRWM(dst,src,off,msk) PEXTRW     dst,[src+off],msk
#define PSHUFWM(dst,src,off,msk) PSHUFW     dst,[src+off],msk
#define MOVNTQM(dst,src,off)    MOVNTQ      dst,[src+off]

#endif

#endif

/* Just to deal with lower case. */
#define pf2id(dst,src)          PF2ID(dst,src)
#define pfacc(dst,src)          PFACC(dst,src)
#define pfadd(dst,src)          PFADD(dst,src)
#define pfcmpeq(dst,src)        PFCMPEQ(dst,src)
#define pfcmpge(dst,src)        PFCMPGE(dst,src)
#define pfcmpgt(dst,src)        PFCMPGT(dst,src)
#define pfmax(dst,src)          PFMAX(dst,src)
#define pfmin(dst,src)          PFMIN(dst,src)
#define pfmul(dst,src)          PFMUL(dst,src)
#define pfrcp(dst,src)          PFRCP(dst,src)
#define pfrcpit1(dst,src)       PFRCPIT1(dst,src)
#define pfrcpit2(dst,src)       PFRCPIT2(dst,src)
#define pfrsqrt(dst,src)        PFRSQRT(dst,src)
#define pfrsqit1(dst,src)       PFRSQIT1(dst,src)
#define pfsub(dst,src)          PFSUB(dst,src)
#define pfsubr(dst,src)         PFSUBR(dst,src)
#define pi2fd(dst,src)          PI2FD(dst,src)
#define femms                   FEMMS
#define pavgusb(dst,src)        PAVGUSB(dst,src)
#define pmulhrw(dst,src)        PMULHRW(dst,src)
#define prefetch(src)           PREFETCH(src)
#define prefetchw(src)          PREFETCHW(src)

#define prefetchm(src,off)      PREFETCHM(src,off)
#define prefetchmlong(src,off)	PREFETCHMLONG(src,off)
#define prefetchwm(src,off)     PREFETCHWM(src,off)
#define prefetchwmlong(src,off)	 PREFETCHWMLONG(src,off)

#define pfnacc(dst,src)         PFNACC(dst,src)
#define pfpnacc(dst,src)        PFPNACC(dst,src)
#define pswapd(dst,src)         PSWAPD(dst,src)
#define pminub(dst,src)         PMINUB(dst,src)
#define pmaxub(dst,src)         PMAXUB(dst,src)
#define pminsw(dst,src)         PMINSW(dst,src)
#define pmaxsw(dst,src)         PMAXSW(dst,src)
#define pmulhuw(dst,src)        PMULHUW(dst,src)
#define pavgb(dst,src)          PAVGB(dst,src)
#define pavgw(dst,src)          PAVGW(dst,src)
#define psadbw(dst,src)         PSADBW(dst,src)
#define pmovmskb(dst,src)       PMOVMSKB(dst,src)
#define pmaskmovq(dst,src)      PMASKMOVQ(dst,src)
#define pinsrw(dst,src,msk)     PINSRW(dst,src,msk)
#define pextrw(dst,src,msk)     PEXTRW(dst,src,msk)
#define pshufw(dst,src,msk)     PSHUFW(dst,src,msk)
#define movntq(dst,src)         MOVNTQ(dst,src)
#define prefetchnta(mem)        PREFETCHNTA(mem)
#define prefetcht0(mem)         PREFETCHT0(mem)  
#define prefetcht1(mem)         PREFETCHT1(mem)  
#define prefetcht2(mem)         PREFETCHT2(mem)  


#define pavgusbm(dst,src,off)   PAVGUSBM(dst,src,off)
#define pf2idm(dst,src,off)     PF2IDM(dst,src,off)
#define pfaccm(dst,src,off)     PFACCM(dst,src,off)
#define pfaddm(dst,src,off)     PFADDM(dst,src,off)
#define pfcmpeqm(dst,src,off)   PFCMPEQM(dst,src,off)
#define pfcmpgem(dst,src,off)   PFCMPGEM(dst,src,off)
#define pfcmpgtm(dst,src,off)   PFCMPGTM(dst,src,off)
#define pfmaxm(dst,src,off)     PFMAXM(dst,src,off)
#define pfminm(dst,src,off)     PFMINM(dst,src,off)
#define pfmulm(dst,src,off)     PFMULM(dst,src,off)
#define pfrcpm(dst,src,off)     PFRCPM(dst,src,off)
#define pfrcpit1m(dst,src,off)  PFRCPIT1M(dst,src,off)
#define pfrcpit2m(dst,src,off)  PFRCPIT2M(dst,src,off)
#define pfrsqrtm(dst,src,off)   PFRSQRTM(dst,src,off)
#define pfrsqit1m(dst,src,off)  PFRSQIT1M(dst,src,off)
#define pfsubm(dst,src,off)     PFSUBM(dst,src,off)
#define pfsubrm(dst,src,off)    PFSUBRM(dst,src,off)
#define pi2fdm(dst,src,off)     PI2FDM(dst,src,off)
#define pmulhrwm(dst,src,off)   PMULHRWM(dst,src,off)
#define cpuid                   CPUID
#define sfence                  SFENCE

#define pfnaccm(dst,src,off)    PFNACCM(dst,src,off)
#define pfpnaccm(dst,src,off)   PFPNACCM(dst,src,off)
#define pswapdm(dst,src,off)    PSWAPDM(dst,src,off)
#define pminubm(dst,src,off)    PMINUBM(dst,src,off)
#define pmaxubm(dst,src,off)    PMAXUBM(dst,src,off)
#define pminswm(dst,src,off)    PMINSWM(dst,src,off)
#define pmaxswm(dst,src,off)    PMAXSWM(dst,src,off)
#define pmulhuwm(dst,src,off)   PMULHUWM(dst,src,off)
#define pavgbm(dst,src,off)     PAVGBM(dst,src,off)
#define pavgwm(dst,src,off)     PAVGWM(dst,src,off)
#define psadbwm(dst,src,off)    PSADBWM(dst,src,off)
#define pmovmskbm(dst,src,off)  PMOVMSKBM(dst,src,off)
#define pmaskmovqm(dst,src,off) PMASKMOVQM(dst,src,off)
#define pinsrwm(dst,src,off,msk)    PINSRWM(dst,src,off,msk)
#define pextrwm(dst,src,off,msk)    PEXTRWM(dst,src,off,msk)
#define pshufwm(dst,src,off,msk)    PSHUFWM(dst,src,off,msk)
#define movntqm(dst,src,off)    MOVNTQM(dst,src,off)
#define prefetchntam(mem,off)   PREFETCHNTA(mem,off)
#define prefetcht0m(mem,off)    PREFETCHT0(mem,off)  
#define prefetcht1m(mem,off)    PREFETCHT1(mem,off)  
#define prefetcht2m(mem,off)    PREFETCHT2(mem,off)  

#endif
