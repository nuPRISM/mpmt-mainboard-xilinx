#!/bin/bash
# midas2root script
# T. Lindner

# Go to right output directory
cd /daq/daqshare/mpmtdaq3/midas_files/rootfiles/

# Get current filename
export Filename=`odbedit -c 'ls "/Logger/Channels/0/Settings/Current filename"' | awk -F "  +" '{print $2}'`

# Set midas file directory
export MidasDir=/daq/daqshare/mpmtdaq3/midas_files

# Do the conversion
/home/mpmtdaq3/online/ptf-online-converter-xilinx/midas2root/midas2root_mpmt.exe $MidasDir/$Filename

