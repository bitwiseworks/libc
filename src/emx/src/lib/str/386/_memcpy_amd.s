//  Copyright (c) 2001 Advanced Micro Devices, Inc.
//
// LIMITATION OF LIABILITY:  THE MATERIALS ARE PROVIDED *AS IS* WITHOUT ANY
// EXPRESS OR IMPLIED WARRANTY OF ANY KIND INCLUDING WARRANTIES OF MERCHANTABILITY,
// NONINFRINGEMENT OF THIRD-PARTY INTELLECTUAL PROPERTY, OR FITNESS FOR ANY
// PARTICULAR PURPOSE.  IN NO EVENT SHALL AMD OR ITS SUPPLIERS BE LIABLE FOR ANY
// DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF PROFITS,
// BUSINESS INTERRUPTION, LOSS OF INFORMATION) ARISING OUT OF THE USE OF OR
// INABILITY TO USE THE MATERIALS, EVEN IF AMD HAS BEEN ADVISED OF THE POSSIBILITY
// OF SUCH DAMAGES.  BECAUSE SOME JURISDICTIONS PROHIBIT THE EXCLUSION OR LIMITATION
// OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES, THE ABOVE LIMITATION MAY
// NOT APPLY TO YOU.
//
// AMD does not assume any responsibility for any errors which may appear in the
// Materials nor any responsibility to support or update the Materials.  AMD retains
// the right to make changes to its test specifications at any time, without notice.
//
// NO SUPPORT OBLIGATION: AMD is not obligated to furnish, support, or make any
// further information, software, technical information, know-how, or show-how
// available to you.
//
// So that all may benefit from your experience, please report  any  problems
// or  suggestions about this software to 3dsdk.support@amd.com
//
// AMD Developer Technologies, M/S 585
// Advanced Micro Devices, Inc.
// 5900 E. Ben White Blvd.
// Austin, TX 78741
// 3dsdk.support@amd.com

// Very optimized memcpy() routine for all AMD Athlon and Duron family.
// This code uses any of FOUR different basic copy methods, depending
// on the transfer size.
// NOTE:  Since this code uses MOVNTQ (also known as "Non-Temporal MOV" or
// "Streaming Store"), and also uses the software prefetchnta instructions,
// be sure you're running on Athlon/Duron or other recent CPU before calling!

#include <emx/asm386.h>


#define TINY_BLOCK_COPY  $64 			// upper limit for movsd type copy
// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".

#define IN_CACHE_COPY 	(64 * 1024)		// upper limit for movq/movq copy w/SW prefetch
// Next is a copy that uses the MMX registers to copy 8 bytes at a time,
// also using the "unrolled loop" optimization.   This code uses
// the software prefetch instruction to get the data into the cache.

#define UNCACHED_COPY 	(197 * 1024)	// upper limit for movq/movntq w/SW prefetch
// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.
// USE 64 * 1024 FOR THIS VALUE IF YOU'RE ALWAYS FILLING A "CLEAN CACHE"

#define BLOCK_PREFETCH_COPY $-1 		// no limit for movq/movntq w/block prefetch
#define CACHEBLOCK  0x80       		// number of 64-byte blocks (cache lines) for block prefetch
// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch.  The technique is great for
// getting maximum read bandwidth, especially in DDR memory systems.

    .globl LABEL(_memcpy_amd)
LABEL(_memcpy_amd):
	pushl   %esi
	pushl	%edi
	pushl	%ebx

	movl	(4+12)(%esp), %edi				// destination
	movl	(8+12)(%esp), %esi				// source
	movl	(12+12)(%esp), %ecx				// number of bytes to copy
	movl	%ecx, %ebx					// keep a copy of count

	cld
	cmpl	TINY_BLOCK_COPY, %ecx
	jb		l_memcpy_ic_3				// tiny? skip mmx copy

	cmpl	$32 * 1024, %ecx			// don't align between 32k-64k because
	jbe		l_memcpy_do_align			//  it appears to be slower
	cmpl	$64 * 1024, %ecx
	jbe		l_memcpy_align_done
l_memcpy_do_align:
	movl	$8, %ecx					// a trick that's faster than rep movsb...
	subl	%edi, %ecx					// align destination to qword
	andl	$7, %ecx					// get the low bits
	subl	%ecx, %ebx					// update copy count
	negl	%ecx						// set up to jump into the array
	addl	$l_memcpy_align_done, %ecx
	jmp		*%ecx						// jump to array of movsb's

ALIGNP2(2)
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb

l_memcpy_align_done:					// destination is dword aligned
	movl	%ebx, %ecx					// number of bytes left to copy
	shrl	$6, %ecx					// get 64-byte block count
	jz		l_memcpy_ic_2				// finish the last few bytes

	cmpl	$ IN_CACHE_COPY / 64, %ecx	// too big 4 cache? use uncached copy
	jae		l_memcpy_uc_test
	jmp     l_memcpy_ic_1

// This is small block copy that uses the MMX registers to copy 8 bytes
// at a time.  It uses the "unrolled loop" optimization, and also uses
// the software prefetch instruction to get the data into the cache.
	ALIGNP2(4)
l_memcpy_ic_1:							// 64-byte block copies, in-cache copy

	prefetchnta (200 * 64 / 34 + 192)(%esi)	// start reading ahead

	movq	(%esi), %mm0 				// read 64 bits
	movq	8(%esi), %mm1
	movq	%mm0, (%edi)				// write 64 bits
	movq	%mm1, 8(%edi)				//    note:  the normal movq writes the
	movq	16(%esi), %mm2	 			//    data to cache// a cache line will be
	movq	24(%esi), %mm3 				//    allocated as needed, to store the data
	movq	%mm2, 16(%edi)
	movq	%mm3, 24(%edi)
	movq	32(%esi), %mm0
	movq	40(%esi), %mm1
	movq	%mm0, 32(%edi)
	movq	%mm1, 40(%edi)
	movq	48(%esi), %mm2
	movq	56(%esi), %mm3
	movq	%mm2, 48(%edi)
	movq	%mm3, 56(%edi)

	addl	$64, %esi					// update source pointer
	addl	$64, %edi					// update destination pointer
	decl	%ecx						// count down
	jnz		l_memcpy_ic_1				// last 64-byte block?

l_memcpy_ic_2:
	movl	%ebx, %ecx					// has valid low 6 bits of the byte count
l_memcpy_ic_3:
	shrl	$2, %ecx					// dword count
	andl	$15, %ecx					// only look at the "remainder" bits
	negl	%ecx						// set up to jump into the array
	addl	$l_memcpy_last_few, %ecx
	jmp		*%ecx						// jump to array of movsd's

l_memcpy_uc_test:
	cmpl	$ UNCACHED_COPY / 64, %ecx	// big enough? use block prefetch copy
	jae		l_memcpy_bp_1

l_memcpy_64_test:
	orl		%ecx, %ecx					// tail end of block prefetch will jump here
	jz		l_memcpy_ic_2				// no more 64-byte blocks left

// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.
	ALIGNP2(4)
l_memcpy_uc_1:							// 64-byte blocks, uncached copy

	prefetchnta (200 * 64 / 34 + 192)(%esi)		// start reading ahead

	movq	(%esi), %mm0				// read 64 bits
	addl	$64, %edi					// update destination pointer
	movq	8(%esi), %mm1
	addl	$64, %esi					// update source pointer
	movq	-48(%esi), %mm2
	movntq	%mm0, -64(%edi)				// write 64 bits, bypassing the cache
	movq	-40(%esi),%mm0				//    note: movntq also prevents the CPU
	movntq	%mm1, -56(%edi)				//    from READING the destination address
	movq	-32(%esi),%mm1				//    into the cache, only to be over-written
	movntq	%mm2, -48(%edi)				//    so that also helps performance
	movq	-24(%esi), %mm2
	movntq	%mm0, -40(%edi)
	movq	-16(%esi), %mm0
	movntq	%mm1, -32(%edi)
	movq	-8(%esi), %mm1
	movntq	%mm2, -24(%edi)
	movntq	%mm0, -16(%edi)
	decl	%ecx
	movntq	%mm1, -8(%edi)
	jnz		l_memcpy_uc_1				// last 64-byte block?

	jmp		l_memcpy_ic_2				// almost done

// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch, in this case.
// The technique is great for getting maximum read bandwidth,
// especially in DDR memory systems.
l_memcpy_bp_1:							// large blocks, block prefetch copy

	cmp		$ CACHEBLOCK, %ecx			// big enough to run another prefetch loop?
	jl		l_memcpy_64_test			// no, back to regular uncached copy
	movl	$ CACHEBLOCK / 2, %eax	 	// block prefetch loop, unrolled 2X
	addl	$ CACHEBLOCK * 64, %esi 	// move to the top of the block
	jmp     l_memcpy_bp_2
	
	ALIGNP2(4)						
l_memcpy_bp_2:
	movl	-64(%esi), %edx  			// grab one address per cache line
	movl	-128(%esi), %edx 			// grab one address per cache line
	subl	$128, %esi					// go reverse order
	decl	%eax						// count down the cache lines
	jnz		l_memcpy_bp_2				// keep grabbing more lines into cache
	
	movl	$ CACHEBLOCK, %eax			// now that it's in cache, do the copy
	jmp     l_memcpy_bp_3

	ALIGNP2(4)
l_memcpy_bp_3:
	movq	  (%esi), %mm0				// read 64 bits
	movq	 8(%esi), %mm1
	movq	16(%esi), %mm2
	movq	24(%esi), %mm3
	movq	32(%esi), %mm4
	movq	40(%esi), %mm5
	movq	48(%esi), %mm6
	movq	56(%esi), %mm7
	addl	$64, %esi					// update source pointer
	
	movntq	%mm0,   (%edi) 				// write 64 bits, bypassing cache
	movntq	%mm1,  8(%edi) 				//    note: movntq also prevents the CPU
	movntq	%mm2, 16(%edi) 				//    from READING the destination address
	movntq	%mm3, 24(%edi) 				//    into the cache, only to be over-written,
	movntq	%mm4, 32(%edi) 				//    so that also helps performance
	movntq	%mm5, 40(%edi)
	movntq	%mm6, 48(%edi)
	movntq	%mm7, 56(%edi)
	addl	$64, %edi					// update dest pointer

	decl	%eax						// count down
	jnz		l_memcpy_bp_3				// keep copying

	subl	$ CACHEBLOCK, %ecx			// update the 64-byte block count
	jmp		l_memcpy_bp_1				// keep processing chunks

// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".   Then it handles the last few bytes.
	ALIGNP2(2)
	movsd
	movsd								// perform last 1-15 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd								// perform last 1-7 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd

l_memcpy_last_few:						// dword aligned from before movsd's
	movl	%ebx, %ecx					// has valid low 2 bits of the byte count
	andl	$3, %ecx					// the last few cows must come home
	jz		l_memcpy_final				// no more, let's leave
	rep		movsb						// the last 1, 2, or 3 bytes

l_memcpy_final:
	emms								// clean up the MMX state
	sfence	

	popl	%ebx
	popl	%edi
	popl	%esi
	movl    4(%esp), %eax
	ret

