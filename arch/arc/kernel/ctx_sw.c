/*
 * Copyright (C) 2004, 2007-2010, 2011-2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Vineetg: Aug 2009
 *  -"C" version of lowest level context switch asm macro called by schedular
 *   gcc doesn't generate the dward CFI info for hand written asm, hence can't
 *   backtrace out of it (e.g. tasks sleeping in kernel).
 *   So we cheat a bit by writing almost similar code in inline-asm.
 *  -This is a hacky way of doing things, but there is no other simple way.
 *   I don't want/intend to extend unwinding code to understand raw asm
 */

#include <asm/asm-offsets.h>
#include <linux/sched.h>

#define KSP_WORD_OFF 	((TASK_THREAD + THREAD_KSP) / 4)

struct task_struct *__sched
__switch_to(struct task_struct *prev_task, struct task_struct *next_task)
{
	unsigned int tmp;
	unsigned int prev = (unsigned int)prev_task;
	unsigned int next = (unsigned int)next_task;

	__asm__ __volatile__(
		/* FP/BLINK save generated by gcc (standard function prologue */
		"st.a    r13, [sp, -4]   \n\t"
		"st.a    r14, [sp, -4]   \n\t"
		"st.a    r15, [sp, -4]   \n\t"
		"st.a    r16, [sp, -4]   \n\t"
		"st.a    r17, [sp, -4]   \n\t"
		"st.a    r18, [sp, -4]   \n\t"
		"st.a    r19, [sp, -4]   \n\t"
		"st.a    r20, [sp, -4]   \n\t"
		"st.a    r21, [sp, -4]   \n\t"
		"st.a    r22, [sp, -4]   \n\t"
		"st.a    r23, [sp, -4]   \n\t"
		"st.a    r24, [sp, -4]   \n\t"
#ifndef CONFIG_ARC_CURR_IN_REG
		"st.a    r25, [sp, -4]   \n\t"
#else
		"sub     sp, sp, 4      \n\t"	/* usual r25 placeholder */
#endif

		/* set ksp of outgoing task in tsk->thread.ksp */
#if KSP_WORD_OFF <= 255
		"st.as   sp, [%3, %1]    \n\t"
#else
		/*
		 * Workaround for NR_CPUS=4k
		 * %1 is bigger than 255 (S9 offset for st.as)
		 */
		"add2    r24, %3, %1     \n\t"
		"st      sp, [r24]       \n\t"
#endif

		"sync   \n\t"

		/*
		 * setup _current_task with incoming tsk.
		 * optionally, set r25 to that as well
		 * For SMP extra work to get to &_current_task[cpu]
		 * (open coded SET_CURR_TASK_ON_CPU)
		 */
#ifndef CONFIG_SMP
		"st  %2, [@_current_task]	\n\t"
#else
		"lr   r24, [identity]		\n\t"
		"lsr  r24, r24, 8		\n\t"
		"bmsk r24, r24, 7		\n\t"
		"add2 r24, @_current_task, r24	\n\t"
		"st   %2,  [r24]		\n\t"
#endif
#ifdef CONFIG_ARC_CURR_IN_REG
		"mov r25, %2   \n\t"
#endif

		/* get ksp of incoming task from tsk->thread.ksp */
		"ld.as  sp, [%2, %1]   \n\t"

		/* start loading it's CALLEE reg file */

#ifndef CONFIG_ARC_CURR_IN_REG
		"ld.ab   r25, [sp, 4]   \n\t"
#else
		"add    sp, sp, 4       \n\t"
#endif
		"ld.ab   r24, [sp, 4]   \n\t"
		"ld.ab   r23, [sp, 4]   \n\t"
		"ld.ab   r22, [sp, 4]   \n\t"
		"ld.ab   r21, [sp, 4]   \n\t"
		"ld.ab   r20, [sp, 4]   \n\t"
		"ld.ab   r19, [sp, 4]   \n\t"
		"ld.ab   r18, [sp, 4]   \n\t"
		"ld.ab   r17, [sp, 4]   \n\t"
		"ld.ab   r16, [sp, 4]   \n\t"
		"ld.ab   r15, [sp, 4]   \n\t"
		"ld.ab   r14, [sp, 4]   \n\t"
		"ld.ab   r13, [sp, 4]   \n\t"

		/* last (ret value) = prev : although for ARC it mov r0, r0 */
		"mov     %0, %3        \n\t"

		/* FP/BLINK restore generated by gcc (standard func epilogue */

		: "=r"(tmp)
		: "n"(KSP_WORD_OFF), "r"(next), "r"(prev)
		: "blink"
	);

	return (struct task_struct *)tmp;
}
