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

noinst_PROGRAMS = spaction

SPACTION_INC = -I$(top_srcdir)/spaction/include
SPACTION_LD  = $(top_srcdir)/spaction/libspaction.la $(top_srcdir)/spaction/src/cltlparse/libcltlparser.la

if WITH_SPOT_PATH
SPOT_LIB  = $(SPOT_ROOT)/lib
SPOT_INC  = -I$(SPOT_ROOT)/include/ -I$(SPOT_ROOT)/include/spot/
endif

#SPOT_LD   = -L$(SPOT_LIB) -lbddx -lspot -lspotltsmin
SPOT_LD   = $(SPOT_LIB)/libbddx.a $(SPOT_LIB)/libspot.a $(SPOT_LIB)/libspotltsmin.a

spaction_SOURCES      = main.cpp
spaction_DEPENDENCIES = $(top_srcdir)/spaction/libspaction.la
spaction_CPPFLAGS     = $(SPOT_INC) $(SPACTION_INC) -Wall
spaction_LDFLAGS      = $(SPACTION_LD) $(SPOT_LD)
