#!/bin/bash
xterm -e " nios2_command_shell.sh do_connect_to_jtag_uart1_linux.sh" & 
echo "sleep 50"
sleep 50
xterm -e " nios2_command_shell.sh do_app_nios_load_linux.sh" & 
echo "sleep 90"
sleep 90
xterm -e " nios2_command_shell.sh do_dut_nios_load_linux.sh" & 
