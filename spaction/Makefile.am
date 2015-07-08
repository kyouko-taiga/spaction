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

SUBDIRS = src

lib_LTLIBRARIES   = libspaction.la

spactiondir       = $(pkgincludedir)

spaction_hdrs     = include/automata/CA2tgba.h \
										include/automata/CltlTranslator.h \
										include/automata/ConfigurationAutomaton.h \
										include/automata/ControlBlock.h \
										include/automata/CounterAutomaton.h \
										include/automata/CounterAutomatonProduct.h \
										include/automata/DeterministicTransitionSystem.h \
										include/automata/RegisterAutomaton.h \
										include/automata/SupremumFinder.h \
										include/automata/TGBA2CA.h \
										include/automata/TransitionSystem.h \
										include/automata/TransitionSystemPrinter.h \
										include/automata/TransitionSystemProduct.h \
										include/automata/UndeterministicTransitionSystem.h \
										include/bdd_util.h \
										include/cltlparse/CLTLScanner.h \
										include/cltlparse/public.h \
										include/hash/hash.h \
										include/AtomicProposition.h \
										include/BinaryOperator.h \
										include/cltl2spot.h \
										include/CltlFormula.h \
										include/CltlFormulaFactory.h \
										include/CltlFormulaVisitor.h \
										include/ConstantExpression.h \
										include/Instantiator.h \
										include/Logger.h \
										include/MultOperator.h \
										include/spotcheck.h \
										include/UnaryOperator.h

spaction_HEADERS  = $(spaction_hdrs)

spaction_srcs     = src/automata/CltlTranslator.cpp \
										src/automata/CounterAutomaton.cpp \
										src/AtomicProposition.cpp \
										src/BinaryOperator.cpp \
										src/cltl2spot.cpp \
										src/CltlFormulaFactory.cpp \
										src/ConstantExpression.cpp \
										src/Instantiator.cpp \
										src/MultOperator.cpp \
										src/spotcheck.cc \
										src/UnaryOperator.cpp

if WITH_SPOT_PATH
SPOT_INC  = -I$(SPOT_ROOT)/include/ -I$(SPOT_ROOT)/include/spot/
SPOT_LD   = $(SPOT_ROOT)/lib/libspot.a
else
SPOT_LD   = -lspot
endif

libspaction_la_SOURCES    =   $(spaction_hdrs) $(spaction_srcs)
libspaction_la_CPPFLAGS   =   -Wall -I./include $(SPOT_INC)
