# Makefile for backfill scheduler plugin

AUTOMAKE_OPTIONS = foreign

AM_CXXFLAGS = -fexceptions

PLUGIN_FLAGS = -module -avoid-version --export-dynamic

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src/common

pkglib_LTLIBRARIES = sched_backfill.la

sched_backfill_la_SOURCES = backfill_wrapper.c	\
			backfill.c	\
			backfill.h	\
			dynamic_licenses.h\
			dynamic_licenses.c

sched_backfill_la_LDFLAGS = $(SO_LDFLAGS) $(PLUGIN_FLAGS) -lrt
