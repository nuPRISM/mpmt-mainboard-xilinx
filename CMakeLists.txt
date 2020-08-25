cmake_minimum_required(VERSION 3.10)

# compiler options
SET(CMAKE_CXX_FLAGS  "-g -O2 -Wall -fpermissive -std=c++11 -DOS_LINUX")

# MIDAS include and libraries
link_libraries( "-L$ENV{MIDASSYS}/lib -lmidas -lm -lz -lutil  -lpthread  -ldl")
include_directories($ENV{MIDASSYS}/include)

# add the feUDP executable
project(UDP)
add_executable(UDP feudp.cxx $ENV{MIDASSYS}/lib/mfe.o)

# add the feBRB executable
project(BRB)
add_executable(BRB febrb.cxx PMTControl.cxx KOsocket.cxx $ENV{MIDASSYS}/lib/mfe.o)


