
#include "PMTControl.h"

bool PMTControl::SetCommand(std::string command, int value){


  char buffer[200];
  if(command.compare("SetChannel") == 0){
    sprintf(buffer,"custom_command select_pmt %i \n",value);
  }else{
    if(command.compare("SH") == 0){
      sprintf(buffer,"custom_command exec_pmt_cmd 01%s%04i \n",command.c_str(),value);
    }else if(command.compare("HV") == 0){
      sprintf(buffer,"custom_command exec_pmt_cmd 01%s%i \n",command.c_str(),value);
    }else if(command.compare("SR") == 0){
      sprintf(buffer,"custom_command exec_pmt_cmd 01%s%03i \n",command.c_str(),value);
    }else{
      cm_msg(MERROR,"PMTControl::SetCommand","Invalid set command %s",command.c_str());
      return false;
    }
  }

  int size=sizeof(buffer);
  std::cout << "Command : " << buffer << " " << size << std::endl;
  fSocket->write(buffer,size);
  char bigbuffer[500];
  size = sizeof(bigbuffer);
  fSocket->read(bigbuffer,size);
  
  std::string readback(bigbuffer);
  readback.pop_back();
  readback.pop_back(); 
  readback.pop_back(); 
  readback.pop_back(); 
  readback.pop_back(); 

  // Check that the reply has the expected except substring
  std::string origcommand(buffer);  
  if(origcommand.find(readback) == std::string::npos){
    cm_msg(MERROR,"PMTControl::SetCommand","Reply did not match original command: %s %s",readback.c_str(),origcommand.c_str());
    return false;
  }
  std::cout << "Reply matches expectation: " << readback << " " << origcommand << std::endl;

  return true;
}

PMTControl::PMTControl(KOsocket *socket){

  // Save connection to socket.
  fSocket = socket;

  midas::odb::set_debug(true); 
  
  //Get ODB values (new C++ ODB!)                                                                                                                             
  midas::odb pmts = {
    {"ChannelSelect", 0 },
    {"HVmax", std::array<double, 20>{} },
    {"HVenable", std::array<bool, 20>{} },
    {"HVset", std::array<double, 20>{} },
    {"HVRampRate", std::array<int, 20>{} }

  };

  pmts.connect("/Equipment/PMTS/Settings");
  gSelectedChannel = pmts["ChannelSelect"];

  // Setup the DB watch for the PMT settings
  pmt_watch.connect("/Equipment/PMTS/Settings");
  pmt_watch.watch([this](midas::odb &o) {

      if(o.get_full_path().find("ChannelSelect") != std::string::npos){
	std::cout << "Changed selected to channel:  \"" + o.get_full_path() + "\" changed to " << o << std::endl;
	gSelectedChannel = (int)o;	
	SetCommand("SetChannel", gSelectedChannel);
      }else if(o.get_full_path().find("HVset") != std::string::npos){
	std::cout << "Channel " << gSelectedChannel << " HV changed to " << o[gSelectedChannel] << std::endl;
	SetCommand("SH", o[gSelectedChannel]);
      }else if(o.get_full_path().find("HVRampRate") != std::string::npos){
	std::cout << "Channel " << gSelectedChannel << " ramp rate changed to " << o[gSelectedChannel] << std::endl;
	SetCommand("SR", o[gSelectedChannel]);
      }else if(o.get_full_path().find("HVenable") != std::string::npos){
	int state = (int)o[gSelectedChannel];
	if(state){
	  std::cout << "Turning on HV for channel " << gSelectedChannel << std::endl;
	  SetCommand("HV", 1);
	}else{
	  std::cout << "Turning off HV for channel " << gSelectedChannel << std::endl;
	  SetCommand("HV", 0);
	}
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
