import subprocess
import time
import sys
import json

n = int(sys.argv[1])

Start_Channel=0
Num_CH = 19
HV_Change=0 #The change in the HV from the

# Specify the path to the JSON file in another directory
file_path = '/home/mpmttest/online/mpmt-mainboard-xilinx/hv_control/jsonFiles/data_Run_1265.json'

# Open the file and load its contents as a JSON object
with open(file_path, 'r') as file:
    json_data = json.load(file)

# Print the JSON data
print(json.dumps(json_data, indent=2))  # Use indent for pretty-printing



for i in range(Start_Channel,Start_Channel+int(Num_CH)):

    print("Starting for CH " + str(i))

    PMT_CIN= ""
    Get_values = f'curl "https://modulo.triumf.ca/api/get_all_installed_pmt_values?mpmtin=mPMT-TRI-00072&value=EBB,TTS&ordered_by=channel_id,coord_id"'
    #Record the output
    Output_values = subprocess.check_output(Get_values, shell=True)

    pmt_cin_data = json.loads(Output_values)
    cin = pmt_cin_data["CIN"]

    #Converts the output to an integer
    PMT_CIN = cin[i]

    Set_cin = f'odbedit -c "set /Analyzer/PMT_List[{i}] {PMT_CIN}"'
#    subprocess.run(Set_cin, shell=True, check=True)

    print("The CIN for the for the PMT on CH " + str(i) + " is " + PMT_CIN)

    #Get the HV value for the given PMT
    Get_HV = f'curl "https://modulo.triumf.ca/api/get_all_installed_pmt_values?mpmtin=mPMT-TRI-00072&value=EBB,TTS&ordered_by=channel_id,coord_id"'

    #Record the output
    Output_HV = subprocess.check_output(Get_HV, shell=True)

    pmt_cin_data = json.loads(Output_HV)
    hv_hama = pmt_cin_data["EBB"]
        
    #Converts the output to an integer
    HV_Value = int(hv_hama[i]) + n

    print("The HV value for the PMT on CH "+ str(i) + " is " + str(HV_Value))

    #Sets the HV value on the mpmt test site
    Set_HV = f'odbedit -c "set /Equipment/PMTS08/Settings/HVset[{i}] {HV_Value}"'
#    subprocess.run(Set_HV, shell=True, check=True)
