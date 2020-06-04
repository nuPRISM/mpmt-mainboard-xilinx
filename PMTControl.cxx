
#include "PMTControl.h"

PMTControl::PMTControl(KOsocket *socket){

  // Save connection to socket.
  fSocket = socket;

  midas::odb::set_debug(true); 
  
  //Get ODB values (new C++ ODB!)                                                                                                                             
  midas::odb pmts = {
    {"HVmax", std::array<double, 20>{} },
    {"HVenable", std::array<bool, 20>{} },
    {"HVset", std::array<double, 20>{} }

  };

  std::cout << "PMT connect" << std::endl;
  pmts.connect("/Equipment/BRB/Settings/PMTS");
  std::cout << "PMT enable : " << pmts["HVenable"][4] << std::endl;
  std::cout << "PMT max : " << pmts["HVmax"][4] << std::endl;

  // Setup the DB watch for the PMT settings
  pmt_watch.connect("/Equipment/BRB/Settings/PMTS");
  pmt_watch.watch([](midas::odb &o) {
      if(o.get_full_path().find("HVset") != std::string::npos){
	std::cout << "HV set points changed:  \"" + o.get_full_path() + "\" changed to " << o << std::endl;
	// Change the HV set point
      }else if(o.get_full_path().find("HVenable") != std::string::npos){
	std::cout << "HV enable changed:  \"" + o.get_full_path() + "\" changed to " << o << std::endl;
	// Change the enable and HV set point
      };

    });

  // Need to add functionality to check if board was turned off... if board was turned off then find that
  // the enable don't match the board, then enables should be turned off in ODB...


}



//Read value
float PMTControl::ReadValue(std::string command,int chan){
  
  int length = 7;
  float factor = 1.0;
  if(command.compare("01LI")== 0){
    factor = 0.001;
  }else if(command.compare("01LV")== 0){
    factor = 0.001;
  }else if(command.compare("01LS")== 0){
    length = 4;
  }else if(command.compare("01LG")== 0){
    length = 1;
  }else{
    std::cout << "Error, command " << command << " not defined! " << std::endl;
  }


  char buffer[200];
  sprintf(buffer,"custom_command exec_pmt_cmd %s \n",command.c_str());
  int size=sizeof(buffer);
  fSocket->write(buffer,size);
  char bigbuffer[500];
  size = sizeof(bigbuffer);
  fSocket->read(bigbuffer,size);
  
  std::string readback(bigbuffer);
  //  long int value = strtol(readback.substr(4,length).c_str(),NULL,0);
  long int value = atoi(readback.substr(4,length).c_str());
  float fvalue = ((float)value)*factor;
  std::cout << "readback: " << readback.substr(4,length) << " " << value 
	    << " " << " " << fvalue << std::endl;
  
  return fvalue;
}

int PMTControl::GetStatus(char *pevent, INT off)
{


  float *pddata;
  
  // Read currents from PMT
  bk_create(pevent, "PMI0", TID_FLOAT, (void**)&pddata);

  for(int i = 0; i < 20; i++){
    if(i == 0){ // only connect to PMT0 right now.
      *pddata++ = ReadValue("01LI",0);      
    }else{
      *pddata++ = 0.0;
    }
  }

  bk_close(pevent, pddata);

  float *pddata2;
  // Readback voltages from PMT
  bk_create(pevent, "PMV0", TID_FLOAT, (void**)&pddata2);

  for(int i = 0; i < 20; i++){
    if(i == 0){ // only connect to PMT0 right now.
      *pddata2++ = ReadValue("01LV",0);      
    }else{
      *pddata2++ = 0.0;
    }
  }

  bk_close(pevent, pddata2);

  float *pddata3;
  // Setpoint voltages from PMT
  if(0){  
    bk_create(pevent, "PMS0", TID_FLOAT, (void**)&pddata3);
    
    for(int i = 0; i < 20; i++){
      if(i == 0){ // only connect to PMT0 right now.
	*pddata3++ = ReadValue("01LS",0);      
      }else{
	*pddata3++ = 0.0;
      }
    }
    bk_close(pevent, pddata3);
  }

  int *pddata4;
  // ON/OFF  from PMT
  bk_create(pevent, "PMG0", TID_INT, (void**)&pddata4);

  for(int i = 0; i < 20; i++){
    if(i == 0){ // only connect to PMT0 right now.
      *pddata4++ = (int)ReadValue("01LG",0);      
    }else{
      *pddata4++ = 0;
    }
  }
  bk_close(pevent, pddata4);






  return bk_size(pevent);
}
