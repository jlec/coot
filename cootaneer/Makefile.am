# Makefile.am
# 
# Copyright 2007 by Paul Emsley
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or (at
# your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA

EXTRA_DIST = 		\
	cootaneer-llk-2.40.dat

AM_CPPFLAGS = $(CLIPPER_CXXFLAGS) $(MMDB_CXXFLAGS) 

cootaneer_demo_SOURCES = cootaneer-demo.cpp

cootaneer_demo_LDADD = ./libcootaneer.la $(CLIPPER_LIBS) $(MMDB_LIBS) 

check_PROGRAMS = cootaneer-demo

lib_LTLIBRARIES=libcootaneer.la

libcootaneer_la_SOURCES =  buccaneer-lib.cpp buccaneer-prot.cpp \
	buccaneer-sequence.cpp cootaneer-sequence.cpp \
	buccaneer-lib.h buccaneer-prot.h \
	buccaneer-sequence.h cootaneer-sequence.h

libcootaneer_la_LIBADD = $(CLIPPER_LIBS)

libcootaneer_la_LDFLAGS = $(SHARED_LDFLAGS)

subpkgdatadir = $(pkgdatadir)
subpkgdata_DATA = cootaneer-llk-2.40.dat 
