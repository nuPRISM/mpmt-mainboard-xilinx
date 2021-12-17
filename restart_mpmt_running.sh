#!/bin/bash
source ~/.bashrc 

# Stop run
echo "Stopping run"
odbedit -c "stop now"
sleep 5

# Turn off HV
echo "Turning off HV"
source /home/mpmtdaq3/online/mpmt-mainboard/hv_control/hv_off.sh
sleep 60

# Stop frontend
echo "Stop frontend"
odbedit -c "sh febrb01"
sleep 5

# Turn off mainboard
echo "Turn off mainboard"
odbedit -c "set /Equipment/PoeSwitch/Settings/port_enable[0] n"
sleep 20

# Turn on mainboard
echo "Turn on mainboard; wait 6 minutes for it to come up"
odbedit -c "set /Equipment/PoeSwitch/Settings/port_enable[0] y"
sleep 60
echo "wait"
sleep 60
echo "wait"
sleep 60
echo "wait"
sleep 60
echo "wait"
sleep 60
echo "wait"
sleep 60
echo "wait"

# Start frontend
echo "start frontend"
/home/mpmtdaq3/online/mpmt-mainboard/febrb.exe -i 1 -D
sleep 20

# Turn on HV
echo "Turning on HV"
source /home/mpmtdaq3/online/mpmt-mainboard/hv_control/set_hv.sh
sleep 20

source /home/mpmtdaq3/online/mpmt-mainboard/hv_control/hv_off.sh
sleep 20

source /home/mpmtdaq3/online/mpmt-mainboard/hv_control/hv_on.sh
sleep 40

# Start run
echo "Starting run"
odbedit -c "start now"
sleep 5
