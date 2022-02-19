
#include "PMTControl.h"
#include "time.h"
#include "sys/time.h"
#include <array>
#include <unistd.h>

// Check which PMTs are plugged in and responding
int PMTControl::CheckActivePMTs(){
  
  int npmts_active = 0;

  std::cout << "Check active " << std::endl;
  
  for(int i = 0; i < 4; i++){
    usleep(300000);
    std::cout << "Checking chan " << i << std::endl;
    SetCommand("SetChannel", i);
    usleep(1200000);
    char buffer[200];
    for(int i = 0; i < 200; i++){buffer[i]=0;}
    sprintf(buffer,"custom_command exec_pmt_cmd 01LG \n");
    int size=strlen(buffer);
    size = strlen(buffer);
    std::cout << "custom command : " << buffer << " size="<< size << std::endl;
    usleep(10000);
    fSocket->write(buffer,size);
    char bigbuffer[50];
    for(int i = 0; i < 50; i++){bigbuffer[i]=0;}
    size = sizeof(bigbuffer);
    usleep(500000);
    fSocket->read(bigbuffer,size);
    std::string readback(bigbuffer);
    std::cout << "Readback from PMT 01LG: " << i << " | " << readback << " |  " <<  readback.size() << std::endl;
    if((readback.size() == 14 or readback.size() == 11 or readback.size() == 10) 
       && (readback.substr(0,4) == std::string("01LG"))){
      std::cout << "Active... " << i << " " << readback.size() << std::endl;
      npmts_active++;
      fActivePMTs[i] = true;
    }else{
      std::cout << "Not active: " << i << " size=" << readback.size() << std::endl;
      std::cout       << "|| readback=" << readback << " ||" << std::endl;
      fActivePMTs[i] = false;
    }

        std::cout << "response: "  << " " << readback.size()<< std::endl;
    ///	      << readback.compare(0,4,"01LG") <<  std::endl;
    
  }

  cm_msg(MINFO,"PMTControl::SetCommand","Number of active PMTs: %i",npmts_active);

  return npmts_active;
}
  



// Define what do do with callbacks from watch function
void PMTControl::callback(midas::odb &o) {

  int gSelectedChannel = o.get_last_index();
  std::cout << "Change value for " << gSelectedChannel << " " << o.get_full_path() << std::endl;
  // check that the selected PMT is active
  if(!fActivePMTs[gSelectedChannel]){
    std::cout << "Channel " << gSelectedChannel << " is inactive; ignoring set command " << std::endl;
    return; 
  }

  SetCommand("SetChannel",gSelectedChannel);
  
  if(o.get_full_path().find("HVset") != std::string::npos){
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
  }

}


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
  size = strlen(buffer);
  std::cout << "Command : " << buffer << " " << size << std::endl;
  fSocket->write(buffer,size);
  char bigbuffer[500];
  size = sizeof(bigbuffer);
  fSocket->read(bigbuffer,size);
  
  
  std::string readback(bigbuffer);
  std::cout << "readback for set command: " << readback << " | " << readback.size() << std::endl;
  readback.pop_back();
  readback.pop_back(); 
  readback.pop_back(); 
  readback.pop_back(); 
  readback.pop_back(); 
  readback.pop_back(); 

  // Check that the reply has the expected except substring
  std::string origcommand(buffer);  
  if(origcommand.find(readback) == std::string::npos and !(command.compare("SetChannel") == 0)){
    // Extra check for the HV on / off message
    if((command.compare("HV") == 0)){
      readback.pop_back(); readback.pop_back(); readback.pop_back();  // extra characters for this particular command
      if(origcommand.find(readback) == std::string::npos){
	cm_msg(MERROR,"PMTControl::SetCommand","Reply did not match original HV command: %s |  %s",readback.c_str(),origcommand.c_str());
	std::cout << "PMTControl : Reply did not match original command: " << readback
		  << " | " << origcommand << std::endl;
	return false;
      }
    }else{
      cm_msg(MERROR,"PMTControl::SetCommand","Reply did not match original command: %s |  %s",readback.c_str(),origcommand.c_str());
      std::cout << "PMTControl : Reply did not match original command: " << readback
		<< " | " << origcommand << std::endl;
      return false;
    }
  }
  //  std::cout << "Reply matches expectation: " << readback << " " << origcommand << std::endl;

  return true;
}

PMTControl::PMTControl(KOsocket *socket, int index){

  // Save connection to socket.
  fSocket = socket;

  // Save frontend index
  fe_index = index;

  //  midas::odb::set_debug(true); 
  
  //Get ODB values (new C++ ODB!)                                                                                                                             
  midas::odb pmts = {
    {"HVmax", std::array<double, 20>{} },
    {"HVenable", std::array<bool, 20>{} },
    {"HVset", std::array<double, 20>{} },
    {"HVRampRate", std::array<int, 20>{} }

  };


  char eq_dir[200];
  sprintf(eq_dir,"/Equipment/PMTS%02i/Settings",get_frontend_index());
  pmts.connect(eq_dir);

  // Setup the DB watch for the PMT settings
  pmt_watch.connect(eq_dir);
  std::function<void(midas::odb &o)> f = [=](midas::odb &o) {  this->callback(o);  };
  pmt_watch.watch(f);

  // Need to add functionality to check if board was turned off... if board was turned off then find that
  // the enable don't match the board, then enables should be turned off in ODB...

  // Check which PMTs are plugged in and responding.
  fActivePMTs = std::vector<bool>(20,false);
  CheckActivePMTs();

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
  }else if(command.compare("01LR")== 0){
    length = 3;
  }else if(command.compare("01LH")== 0){
    length = 4;
  }else if(command.compare("01LD")== 0){
    length = 3;
  }else{
    std::cout << "Error, command " << command << " not defined! " << std::endl;
  }


  char buffer[200];
  sprintf(buffer,"custom_command exec_pmt_cmd %s \n",command.c_str());
  int size=strlen(buffer);
  size = strlen(buffer);
  fSocket->write(buffer,size);
  char bigbuffer[50];
  size = sizeof(bigbuffer);

  struct timeval t1;
  gettimeofday(&t1, NULL);
  fSocket->read(bigbuffer,size);
  struct timeval t2;
  gettimeofday(&t2, NULL);
  //double dtime = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec)/1000000.0;
  //  std::cout << "Time to read one value:: " << dtime*1000.0 << " ms " << std::endl;
  
  std::string readback(bigbuffer);
  //  std::cout << "size: " << readback.size() << std::endl;
  if(readback.size() < 4){ return -9999.0; }
  //  long int value = strtol(readback.substr(4,length).c_str(),NULL,0);
  long int value = atoi(readback.substr(4,length).c_str());
  float fvalue = ((float)value)*factor;
  std::cout << "readback: " <<  readback.substr(4,length) << " " << value 
	    << " " << " " << fvalue << std::endl;
  
  return fvalue;
}

int PMTControl::GetStatus(char *pevent, INT off)
{

  std::vector<float> current(20,0);
  std::vector<float> read_volt(20,0);
  std::vector<float> set_volt(20,0);
  std::vector<float> state(20,0);
  std::vector<float> trip_state(20,0);
  std::vector<float> ramp_rate(20,0);

  struct timeval t1;
  gettimeofday(&t1, NULL);
  std::cout << "Start readout PMTs " << std::endl;
  // Only readout first 4 PMTs for now.
  for(int i = 0; i < 20; i++){
    if(!fActivePMTs[i]) continue; // ignore inactive PMTs
    usleep(100000);
    SetCommand("SetChannel", i);
    usleep(500000);
    current[i] = ReadValue("01LI",0);
    read_volt[i] = ReadValue("01LV",0);
    set_volt[i] = ReadValue("01LH",0);
    state[i] = ReadValue("01LG",0);
    //    trip_state[i] = ReadValue("01LD",0);
    //    ramp_rate[i] = ReadValue("01LR",0);
    //    ramp_rate[i] = ReadValue("01LR",0);
    usleep(100000);
  }

  struct timeval t2;
  gettimeofday(&t2, NULL);

  ///  double dtime = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec)/1000000.0;
  
  float *pddata;  
  // Read currents from PMT
  char bank_name[20];
  sprintf(bank_name,"PMI%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata);
  for(int i = 0; i < 20; i++){ *pddata++ = current[i];  }
  bk_close(pevent, pddata);

  float *pddata2;
  // Readback voltages from PMT
  sprintf(bank_name,"PMV%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata2);
  for(int i = 0; i < 20; i++){ *pddata2++ = read_volt[i];} 
  bk_close(pevent, pddata2);
  
  float *pddata3;
  // measured voltages from PMT
  sprintf(bank_name,"PMH%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata3);  
  for(int i = 0; i < 20; i++){ *pddata3++ = set_volt[i];}  
  bk_close(pevent, pddata3);

  int *pddata4;
  // ON/OFF  from PMT
  sprintf(bank_name,"PMG%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_INT, (void**)&pddata4);
  for(int i = 0; i < 20; i++){ *pddata4++ = (int)state[i];} 
  bk_close(pevent, pddata4);

  int *pddata5; // is channel plugged in?
  //  PMT Active bank
  sprintf(bank_name,"PMA%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_INT, (void**)&pddata5);
  for(int i = 0; i < 20; i++){
    if(fActivePMTs[i])
      *pddata5++ = 1;
    else
      *pddata5++ = 0;
  }

  bk_close(pevent, pddata5);

  //int *pddata5;
  // trip state  from PMT
  //bk_create(pevent, "PMD0", TID_INT, (void**)&pddata5);
  //for(int i = 0; i < 20; i++){ *pddata5++ = (int)trip_state[i];} 
  //bk_close(pevent, pddata5);

  //  int *pddata5;
  // HV setpoint  from PMT
  //bk_create(pevent, "PMR0", TID_INT, (void**)&pddata5);
  //for(int i = 0; i < 20; i++){ *pddata5++ = (int)ramp_rate[i];} 
  //bk_close(pevent, pddata5);




  return bk_size(pevent);
}
