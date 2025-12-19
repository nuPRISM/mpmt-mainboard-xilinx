#!/bin/bash
# Setup the python environment.  Needs to be at least python 3.0 I think.
echo "Into Shell!" >& /home/mpmtdaq3/fepoe.log
export MIDASSYS=/home/mpmtdaq3/packages/midas
export PYTHONPATH=$PYTHONPATH:$MIDASSYS/python
export PATH=.:$HOME/online/bin:$PATH
export PATH=$PATH:$MIDASSYS/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

# setup root
source $HOME/packages/root/bin/thisroot.sh 

# export VIRTUAL_ENV_DISABLE_PROMPT=1
source ~/python3_env/bin/activate

cd /home/mpmtdaq3/online/mpmt-mainboard-xilinx/analyzer

python mpmt_analyzer.py
