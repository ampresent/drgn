# Copyright (c) Facebook, Inc. and its affiliates.
# SPDX-License-Identifier: GPL-3.0+

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = elfutils include

.DELETE_ON_ERROR:

AM_CPPFLAGS = -isystem include -D_GNU_SOURCE

include_HEADERS = drgn.h

BUILT_SOURCES = drgn.h

noinst_LTLIBRARIES = libdrgnimpl.la

ARCH_INS = arch_ppc64.c.in \
	   arch_x86_64.c.in

libdrgnimpl_la_SOURCES = $(ARCH_INS:.c.in=.c) \
			 binary_buffer.c \
			 binary_buffer.h \
			 binary_search_tree.h \
			 bitops.h \
			 cityhash.h \
			 debug_info.c \
			 debug_info.h \
			 dwarf_index.c \
			 dwarf_index.h \
			 error.c \
			 error.h \
			 hash_table.c \
			 hash_table.h \
			 language.c \
			 language.h \
			 language_c.c \
			 lazy_object.c \
			 lazy_object.h \
			 lexer.c \
			 lexer.h \
			 linux_kernel.c \
			 linux_kernel.h \
			 linux_kernel_helpers.c \
			 memory_reader.c \
			 memory_reader.h \
			 minmax.h \
			 object.c \
			 object.h \
			 object_index.c \
			 object_index.h \
			 path.c \
			 path.h \
			 platform.c \
			 platform.h \
			 pp.h \
			 program.c \
			 program.h \
			 serialize.c \
			 serialize.h \
			 siphash.h \
			 splay_tree.c \
			 stack_trace.c \
			 stack_trace.h \
			 string_builder.c \
			 string_builder.h \
			 symbol.c \
			 symbol.h \
			 type.c \
			 type.h \
			 util.h \
			 vector.c \
			 vector.h

libdrgnimpl_la_CFLAGS = -fvisibility=hidden $(OPENMP_CFLAGS)
libdrgnimpl_la_LIBADD = $(OPENMP_LIBS)

if WITH_LIBKDUMPFILE
libdrgnimpl_la_SOURCES += kdump.c
libdrgnimpl_la_CFLAGS += $(libkdumpfile_CFLAGS)
libdrgnimpl_la_LIBADD += $(libkdumpfile_LIBS)
endif

arch_%.c: arch_%.c.in build-aux/gen_arch.awk
	$(AM_V_GEN)gawk -f $(word 2, $^) $< > $@

drgn.h: drgn.h.in configure.ac
	$(AM_V_GEN)sed \
		-e 's/@DRGN_VERSION_MAJOR@/$(word 1,$(subst ., ,@PACKAGE_VERSION@))/g' \
		-e 's/@DRGN_VERSION_MINOR@/$(word 2,$(subst ., ,@PACKAGE_VERSION@))/g' \
		-e 's/@DRGN_VERSION_PATCH@/$(word 3,$(subst ., ,@PACKAGE_VERSION@))/g' \
		$< > $@

elfutils_LIBS = elfutils/libdw/libdw.a elfutils/libelf/libelf.a -lz -llzma -lbz2 -ldl

lib_LTLIBRARIES = libdrgn.la

libdrgn_la_SOURCES =
libdrgn_la_LDFLAGS = -version-info 0:0:0
libdrgn_la_LIBADD = libdrgnimpl.la $(elfutils_LIBS)

if WITH_PYTHON
BUILT_SOURCES += python/docstrings.h

noinst_LTLIBRARIES += _drgn.la
endif

CLEANFILES = python/constants.c python/docstrings.c python/docstrings.h

_drgn_la_SOURCES = python/docstrings.h \
		   python/drgnpy.h \
		   python/error.c \
		   python/helpers.c \
		   python/language.c \
		   python/module.c \
		   python/object.c \
		   python/platform.c \
		   python/program.c \
		   python/stack_trace.c \
		   python/symbol.c \
		   python/test.c \
		   python/type.c \
		   python/util.c

nodist__drgn_la_SOURCES = python/constants.c python/docstrings.c

_drgn_la_CFLAGS = -fvisibility=hidden
_drgn_la_CPPFLAGS = $(AM_CPPFLAGS) $(PYTHON_CPPFLAGS) -iquote $(srcdir)/python \
		    -iquote python
_drgn_la_LDFLAGS = -Wl,--exclude-libs,ALL -avoid-version -module -shared \
		   -rpath $(pkgpyexecdir)
_drgn_la_LIBADD = libdrgnimpl.la $(elfutils_LIBS)

if WITH_LIBKDUMPFILE
_drgn_la_CFLAGS += $(libkdumpfile_CFLAGS)
endif

python/constants.c: drgn.h build-aux/gen_constants.py
	$(AM_V_GEN)$(PYTHON) $(word 2, $^) < $< > $@

drgndoc_docstrings_deps = $(wildcard $(srcdir)/../docs/exts/drgndoc/*.py)
drgndoc_docstrings = PYTHONPATH="$(srcdir)/../docs/exts:$$PYTHONPATH" $(PYTHON) -m drgndoc.docstrings

python/docstrings.c: ../_drgn.pyi $(drgndoc_docstrings_deps)
	$(AM_V_GEN)$(drgndoc_docstrings) -m _drgn:drgn $< > $@

python/docstrings.h: ../_drgn.pyi $(drgndoc_docstrings_deps)
	$(AM_V_GEN)$(drgndoc_docstrings) -H -m _drgn:drgn $< > $@

EXTRA_DIST = $(ARCH_INS) build-aux/gen_arch.awk build-aux/gen_constants.py

EXTRA_PROGRAMS = examples/load_debug_info

examples_load_debug_info_SOURCES = examples/load_debug_info.c
examples_load_debug_info_LDADD = libdrgnimpl.la $(elfutils_LIBS)
