import subprocess
import time
import sys

n=int(sys.argv[1])
#n=1
#print(n)
#n=int(nq)
Start_Channel=0
Num_CH = 20
HV_Change=0 #The change in the HV from the 

#modbset(['/Sequencer/Command/Start script'],
#         ['hv_scan_new.msl', 'Yes', 'Yes'])

for i in range(Start_Channel,Start_Channel+int(Num_CH)):
    
    print("Starting for CH " + str(i))

    PMT_CIN= ""
    Get_PMT_CIN = f'odbedit -c "ls /Analyzer/PMT_List[{i}]"'
    PMT_CIN_Output = subprocess.check_output(Get_PMT_CIN, shell=True)
    PMT_CIN_str = str(PMT_CIN_Output.strip())
    
    #Make the string received readable
    if i>9:
        start_string = 48
    else:
        start_string = 47
    
    for j in range(start_string, len(PMT_CIN_str)-1):
        PMT_CIN += PMT_CIN_str[j]
        

    if PMT_CIN == "":
        Set_HV = f'odbedit -c "set /Equipment/PMTS08/Settings/HVset[{i}] 0"'
        print("PMT ID Empty")
        subprocess.run(Set_HV, shell=True, check=True)
        pass
    else:

        print("The CIN for the for the PMT on CH " + str(i) + " is " + PMT_CIN)

        #Get the HV value for the given PMT
        Get_HV = f'curl "https://modulo.triumf.ca/api/get_pmt_value?cin={PMT_CIN}&value=EBB"'
        #Record the output
        Output_HV = subprocess.check_output(Get_HV, shell=True)
        #Converts the output to an integer
        HV_Value = int(Output_HV.strip()) + n
        print("The HV value for the PMT on CH "+ str(i) + " is " + str(HV_Value))
    
        #Sets the HV value on the mpmt test site
        Set_HV = f'odbedit -c "set /Equipment/PMTS08/Settings/HVset[{i}] {HV_Value}"'
##################3        c = f'odbedit -c "set /Equipment/PMTS08/Settings/HVset[8] {n}"'
        subprocess.run(Set_HV, shell=True, check=True)


        #Set_HV = f'ODBSET "/Equipment/PMTS08/Settings/HVset[{i}]", 1000'
#######################        subprocess.run(c, shell=True, check=True)
