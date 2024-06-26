#####################################################################
#
#  Name:         Makefile
#
#####################################################################
#
#--------------------------------------------------------------------
# The MIDASSYS should be defined prior the use of this Makefile
ifndef MIDASSYS
missmidas::
	@echo "...";
	@echo "Missing definition of environment variable 'MIDASSYS' !";
	@echo "...";
endif

#--------------------------------------------------------------------
# The following lines contain specific switches for different UNIX
# systems. Find the one which matches your OS and outcomment the 
# lines below.

#-----------------------------------------
# This is for Linux
ifeq ($(OSTYPE),Linux)
OSTYPE = linux
endif

#ifeq ($(OSTYPE),linux)

OS_DIR = linux-m64
OSFLAGS = -DOS_LINUX
CFLAGS = -g -O2 -Wall -fpermissive -std=c++11
LIBS = -lm -lz -lutil -lnsl -lrt -lpthread  -ldl
#endif


#-----------------------
# MacOSX/Darwin is just a funny Linux
#
ifeq ($(OSTYPE),Darwin)
OSTYPE = darwin
endif

ifeq ($(OSTYPE),darwin)
OS_DIR = darwin
FF = cc
OSFLAGS = -DOS_LINUX -DOS_DARWIN -DHAVE_STRLCPY -DAbsoftUNIXFortran -fPIC -Wno-unused-function
LIBS = -lpthread -lrt
SPECIFIC_OS_PRG = $(BIN_DIR)/mlxspeaker
NEED_STRLCPY=
NEED_RANLIB=1
NEED_SHLIB=
NEED_RPATH=

endif

#-----------------------------------------
# ROOT flags and libs
#
ifdef ROOTSYS
ROOTCFLAGS := $(shell  $(ROOTSYS)/bin/root-config --cflags)
ROOTCFLAGS += -DHAVE_ROOT -DUSE_ROOT
ROOTLIBS   := $(shell  $(ROOTSYS)/bin/root-config --libs) -Wl,-rpath,$(ROOTSYS)/lib
ROOTLIBS   += -lThread
else
missroot:
	@echo "...";
	@echo "Missing definition of environment variable 'ROOTSYS' !";
	@echo "...";
endif
#-------------------------------------------------------------------
# The following lines define directories. Adjust if necessary
#
MIDAS_INC = $(MIDASSYS)/include -I$(MIDASSYS)/mxml/
MIDAS_LIB = $(MIDASSYS)/lib
MIDAS_SRC = $(MIDASSYS)/src
MIDAS_DRV = $(MIDASSYS)/drivers/vme

# Hardware driver can be (camacnul, kcs2926, kcs2927, hyt1331)
#
DRIVERS =

#-------------------------------------------------------------------
# Frontend code name defaulted to frontend in this example.
# comment out the line and run your own frontend as follow:
# gmake UFE=my_frontend
#
UFE = febrb

####################################################################
# Lines below here should not be edited
####################################################################
#
# compiler
CC   = gcc
CXX  = g++
#
# MIDAS library
LIBMIDAS = -L$(MIDAS_LIB) -lmidas
#
#
# All includes
INCS = -I. -I$(MIDAS_INC) -I$(MIDAS_DRV) 
all: $(UFE).exe  feudp.exe feudpMulti.exe test_brb.exe print_brb_currents.exe


feudp.exe: $(LIB) $(MIDAS_LIB)/mfe.o $(DRIVERS) feudp.o
	$(CXX) $(CFLAGS) $(OSFLAGS) $(INCS) -o feudp.exe feudp.o $(DRIVERS) \
	$(MIDAS_LIB)/mfe.o  $(LIBMIDAS) $(LIBS)

feudpMulti.exe: $(LIB) $(MIDAS_LIB)/mfe.o $(DRIVERS) feudpMulti.o
	$(CXX) $(CFLAGS) $(OSFLAGS) $(INCS) -o feudpMulti.exe feudpMulti.o $(DRIVERS) \
	$(MIDAS_LIB)/mfe.o  $(LIBMIDAS) $(LIBS)


feodbxx_test.exe: $(LIB) $(MIDAS_LIB)/mfe.o $(DRIVERS) feodbxx_test.o
	$(CXX) $(CFLAGS) $(OSFLAGS) $(INCS) -o feodbxx_test.exe feodbxx_test.o $(DRIVERS) \
	$(MIDAS_LIB)/mfe.o  $(LIBMIDAS) $(LIBS)

feudp.o: feudp.cxx
	$(CXX) $(CFLAGS) $(INCS) $(OSFLAGS) -o $@ -c $<

feudpMulti.o: feudpMulti.cxx
	$(CXX) $(CFLAGS) $(INCS) $(OSFLAGS) -o $@ -c $<

feodbxx_test.o: feodbxx_test.cxx
	$(CXX) $(CFLAGS) $(INCS) $(OSFLAGS) -o $@ -c $<

$(UFE).exe: $(LIB) $(MIDAS_LIB)/mfe.o $(DRIVERS) KOsocket.o PMTControl.o $(UFE).o
	$(CXX) $(CFLAGS) $(OSFLAGS) $(INCS) -o $(UFE).exe $(UFE).o $(DRIVERS) \
	$(MIDAS_LIB)/mfe.o KOsocket.o PMTControl.o $(LIBMIDAS) $(LIBS)

print_brb_currents.exe: print_brb_currents.o KOsocket.o
	$(CXX) $(CFLAGS) $(OSFLAGS) $(INCS) -o print_brb_currents.exe print_brb_currents.o $(DRIVERS) \
	KOsocket.o $(LIBS)

test_brb.exe: test_brb.o KOsocket.o
	$(CXX) $(CFLAGS) $(OSFLAGS) $(INCS) -o test_brb.exe test_brb.o $(DRIVERS) \
	KOsocket.o $(LIBS)


febrb.o: febrb.cxx
	$(CXX) $(CFLAGS) $(INCS) $(OSFLAGS) -o $@ -c $<

KOsocket.o: KOsocket.cxx
	$(CXX) $(CFLAGS) $(INCS) $(OSFLAGS) -o $@ -c $<

test_brb.o: test_brb.cxx
	$(CXX) $(CFLAGS) $(INCS) $(OSFLAGS) -o $@ -c $<

print_brb_currents.o: print_brb_currents.cxx
	$(CXX) $(CFLAGS) $(INCS) $(OSFLAGS) -o $@ -c $<


PMTControl.o: PMTControl.cxx
	$(CXX) $(CFLAGS) $(INCS) $(OSFLAGS) -o $@ -c $<


$(MIDAS_LIB)/mfe.o:
	@cd $(MIDASSYS) && make

# %.o: %.cxx
# 	$(CXX) $(USERFLAGS) $(CFLAGS) $(OSFLAGS) $(INCS) -o $@ -c $<

clean::
	rm -f *.exe *.o *~ \#*

#end file
