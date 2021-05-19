#!/bin/bash
# midas2root script
# T. Lindner

# Go to right output directory
cd /data2/mpmtdaq/data/rootfiles/

# Get current filename
export Filename=`odbedit -c 'ls "/Logger/Channels/0/Settings/Current filename"' | awk -F "  +" '{print $2}'`

# Set midas file directory
export MidasDir=/data2/mpmtdaq/data

# Do the conversion
/home/mpmtdaq/online/ptf-online-converter/midas2root/midas2root_mpmt.exe $MidasDir/$Filename

