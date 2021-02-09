// (C) Copyright IBM Corp. 2020
// SPDX-License-Identifier: GPL-3.0+

#include <byteswap.h>
#include <elfutils/libdw.h>
#include <elfutils/libdwfl.h>

#include "drgn.h"
#include "error.h"
#include "linux_kernel.h"
#include "platform.h"
#include "program.h"

#include "arch_ppc64.inc"

static struct drgn_error *
set_initial_registers_from_struct_ppc64(Dwfl_Thread *thread, const void *regs,
					size_t size, bool bswap,
					bool linux_kernel_prstatus,
					bool linux_kernel_switched_out)
{
	if (size < 312) {
		return drgn_error_create(DRGN_ERROR_INVALID_ARGUMENT,
					 "registers are truncated");
	}

	Dwarf_Word dwarf_regs[32];

#define READ_REGISTER(n) ({					\
	uint64_t reg;						\
	memcpy(&reg, (uint64_t *)regs + (n), sizeof(reg));	\
	bswap ? bswap_64(reg) : reg;				\
})

	/*
	 * The NT_PRSTATUS note in Linux kernel vmcores is odd. Since Linux
	 * kernel commit d16a58f8854b ("powerpc: Improve ppc_save_regs()") (in
	 * v5.7), the saved stack pointer (r1) is for the caller of the program
	 * counter saved in nip. Before that, the saved nip is set to the same
	 * as the link register. So, use the link register instead of nip.
	 */
	uint64_t nip = READ_REGISTER(32);
	uint64_t link = READ_REGISTER(36);
	if (linux_kernel_prstatus) {
		dwfl_thread_state_register_pc(thread, link);
	} else {
		dwfl_thread_state_register_pc(thread, nip);
		/*
		 * Switched out tasks in the Linux kernel don't save the link
		 * register.
		 */
		if (!linux_kernel_switched_out) {
			dwarf_regs[0] = link;
			if (!dwfl_thread_state_registers(thread, 65, 1,
							 dwarf_regs))
				return drgn_error_libdwfl();
		}
	}

	/*
	 * Switched out tasks in the Linux kernel only save the callee-saved
	 * general purpose registers (14-31).
	 */
	int min_gpr = linux_kernel_switched_out ? 14 : 0;
	for (int i = min_gpr; i < 32; i++)
		dwarf_regs[i] = READ_REGISTER(i);
	if (!dwfl_thread_state_registers(thread, min_gpr, 32 - min_gpr,
					 dwarf_regs))
		return drgn_error_libdwfl();

	/* cr0 - cr7 */
	uint64_t ccr = READ_REGISTER(38);
	for (int i = 0; i < 8; i++)
		dwarf_regs[i] = (ccr >> (4 * i)) & 0xf;
	if (!dwfl_thread_state_registers(thread, 68, 8, dwarf_regs))
		return drgn_error_libdwfl();

#undef READ_REGISTER

	return NULL;
}

static struct drgn_error *
pt_regs_set_initial_registers_ppc64(Dwfl_Thread *thread,
				    const struct drgn_object *obj)
{
	bool bswap = (obj->little_endian !=
		      (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__));
	return set_initial_registers_from_struct_ppc64(thread,
						       drgn_object_buffer(obj),
						       drgn_object_size(obj),
						       bswap, false, false);
}

static struct drgn_error *
prstatus_set_initial_registers_ppc64(struct drgn_program *prog,
				     Dwfl_Thread *thread, const void *prstatus,
				     size_t size)
{
	if (size < 112) {
		return drgn_error_create(DRGN_ERROR_INVALID_ARGUMENT,
				"NT_PRSTATUS is truncated");
	}
	bool bswap;
	struct drgn_error *err = drgn_program_bswap(prog, &bswap);
	if (err)
		return err;
	bool is_linux_kernel = prog->flags & DRGN_PROGRAM_IS_LINUX_KERNEL;
	return set_initial_registers_from_struct_ppc64(thread,
						       (char *)prstatus + 112,
						       size - 112, bswap,
						       is_linux_kernel, false);
}

static struct drgn_error *
linux_kernel_set_initial_registers_ppc64(Dwfl_Thread *thread,
					 const struct drgn_object *task_obj)
{
	static const uint64_t STACK_FRAME_OVERHEAD = 112;
	static const uint64_t SWITCH_FRAME_SIZE = STACK_FRAME_OVERHEAD + 368;

	struct drgn_error *err;
	struct drgn_program *prog = drgn_object_program(task_obj);
	bool bswap;
	err = drgn_program_bswap(prog, &bswap);
	if (err)
		return err;

	struct drgn_object sp_obj;
	drgn_object_init(&sp_obj, prog);

	err = drgn_object_member_dereference(&sp_obj, task_obj, "thread");
	if (err)
		goto out;
	err = drgn_object_member(&sp_obj, &sp_obj, "ksp");
	if (err)
		goto out;
	uint64_t ksp;
	err = drgn_object_read_unsigned(&sp_obj, &ksp);
	if (err)
		goto out;

	char regs[312];
	err = drgn_program_read_memory(prog, regs, ksp + STACK_FRAME_OVERHEAD,
				       sizeof(regs), false);
	if (err)
		goto out;

	err = set_initial_registers_from_struct_ppc64(thread, regs,
						      sizeof(regs), bswap,
						      false, true);
	if (err)
		goto out;

	/* r1 */
	Dwarf_Word dwarf_reg = ksp + SWITCH_FRAME_SIZE;
	if (!dwfl_thread_state_registers(thread, 1, 1, &dwarf_reg)) {
		err = drgn_error_libdwfl();
		goto out;
	}

	err = NULL;
out:
	drgn_object_deinit(&sp_obj);
	return err;
}

const struct drgn_architecture_info arch_info_ppc64 = {
	.name = "ppc64",
	.arch = DRGN_ARCH_PPC64,
	.default_flags = (DRGN_PLATFORM_IS_64_BIT |
			  DRGN_PLATFORM_IS_LITTLE_ENDIAN),
	ARCHITECTURE_REGISTERS,
	.pt_regs_set_initial_registers = pt_regs_set_initial_registers_ppc64,
	.prstatus_set_initial_registers = prstatus_set_initial_registers_ppc64,
	.linux_kernel_set_initial_registers =
		linux_kernel_set_initial_registers_ppc64,
};
