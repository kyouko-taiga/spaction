# This file is part of spaction
#
# Copyright 2014 Software Modeling and Verification Group
# University of Geneva
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

noinst_LTLIBRARIES = libcltlparser.la

FROM_YY_MAIN = \
	cltllex.cc \
	cltlparse.cc

FROM_YY_OTHER = \
	cltlparse.hh \
	location.hh \
	position.hh \
	stack.hh

BUILT_SOURCES = \
	$(FROM_YY_MAIN) \
	$(FROM_YY_OTHER)

MAINTAINERCLEANFILES = $(BUILT_SOURCES)

AM_YFLAGS = "-d"

libcltlparser_la_CPPFLAGS = -I../../include/
libcltlparser_la_SOURCES = \
	cltllex.ll \
	cltlparse.yy \
	$(FROM_YY_OTHER)
