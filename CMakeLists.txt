cmake_minimum_required(VERSION 3.10)

# set the project name
project(UDP)

# MIDAS include and libraries
link_libraries( "-L$ENV{MIDASSYS}/lib -lmidas -lm -lz -lutil  -lpthread  -ldl")
include_directories($ENV{MIDASSYS}/include)

# add the feUDP executable
add_executable(UDP feudp.cxx $ENV{MIDASSYS}/lib/mfe.o)


