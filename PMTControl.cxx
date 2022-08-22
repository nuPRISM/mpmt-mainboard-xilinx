
#include "PMTControl.h"
#include "time.h"
#include "sys/time.h"
#include <array>
#include <unistd.h>

// Check which PMTs are plugged in and responding
int PMTControl::CheckActivePMTs(){
  
  int npmts_active = 0;

  std::cout << "Check active " << std::endl;
  
  for(int i = 0; i < 12; i++){

    usleep(150000);
    SetCommand("SetChannel",i);

    usleep(150000);
    std::cout << "__________________\nChecking chan " << i << std::endl;
    char buffer[200];
    for(int j = 0; j < 200; j++){buffer[j]=0;}
    sprintf(buffer,"get_PMT_FW %i \n",i);
    int size=strlen(buffer);
    size = strlen(buffer);
    std::cout << "custom command : " << buffer;
    usleep(70000);
    fSocket->write(buffer,size);
    char bigbuffer[50];
    for(int j = 0; j < 50; j++){bigbuffer[j]=0;}
    size = sizeof(bigbuffer);
    usleep(250000);
    fSocket->read(bigbuffer,size);
    std::string readback(bigbuffer);
    std::cout << "Readback from  get_PMT_FW: " << i << " | " << readback << " |  " <<  readback.size() << std::endl;
    if((readback.size() == 22) 
       && (readback.substr(0,17) == std::string("Modbus+FW+present"))){
      std::cout << "Active... " << i << " " << readback.size() << std::endl;
      npmts_active++;
      fActivePMTs[i] = true;
    }else{
      std::cout << "Not active: " << i << " size=" << readback.size() << std::endl;
      std::cout       << "|| readback=" << readback << " ||" << std::endl;
      fActivePMTs[i] = false;
    }

    //        std::cout << "response: "  << " " << readback.size()<< std::endl;
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
    SetCommand("SH", o[gSelectedChannel],gSelectedChannel);
  }else if(o.get_full_path().find("HVRampRate") != std::string::npos){
    std::cout << "Channel " << gSelectedChannel << " ramp rate changed to " << o[gSelectedChannel] << std::endl;
    SetCommand("SR", o[gSelectedChannel],gSelectedChannel);
  }else if(o.get_full_path().find("HVenable") != std::string::npos){
    int state = (int)o[gSelectedChannel];
    if(state){
      std::cout << "Turning on HV for channel " << gSelectedChannel << std::endl;
      SetCommand("HV", 1,gSelectedChannel);
    }else{
      std::cout << "Turning off HV for channel " << gSelectedChannel << std::endl;
      SetCommand("HV", 0,gSelectedChannel);
    }
  }

}


bool PMTControl::SetCommand(std::string command, int value, int ch){


  char buffer[200];
  if(command.compare("SetChannel") == 0){
    sprintf(buffer,"select_pmt %i \n",value);
  }else{
    if(command.compare("SH") == 0){
      sprintf(buffer,"exec_pmt_cmd %02i%s%04i \n",ch+1,command.c_str(),value);
    }else if(command.compare("HV") == 0){
      sprintf(buffer,"exec_pmt_cmd %02i%s%i \n",ch+1,command.c_str(),value);
    }else if(command.compare("SR") == 0){
      sprintf(buffer,"exec_pmt_cmd %02i%s%03i \n",ch+1,command.c_str(),value);
    }else{
      cm_msg(MERROR,"PMTControl::SetCommand","Invalid set command %s",command.c_str());
      return false;
    }
  }

  usleep(60000);
  int size=sizeof(buffer);
  size = strlen(buffer);
  std::cout << "Command : " << buffer << " " << size << std::endl;
  fSocket->write(buffer,size);
  char bigbuffer[500];
  size = sizeof(bigbuffer);
  usleep(100000);
  fSocket->read(bigbuffer,size);
  
  
  std::string readback(bigbuffer);
  std::cout << "readback for set command: " << readback << " | " << readback.size() << std::endl;
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
  std::cout << "Reply matches expectation: " << readback << " " << origcommand << std::endl;

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

float PMTControl::ReadModbusValue(std::string command,int chan){

  float factor = 1500.0;
  if(command.find("HVCurVal") != std::string::npos){
    factor = 10.0 / 65535.0;
  }else if(command.find("HVVolVal") != std::string::npos){
    factor = 1500.0 / 65535.0;
  }else if(command.find("HVVolNom") != std::string::npos){
    factor = 1500.0 / 65535.0;
  }else if((command.find("RampUpSpd") != std::string::npos) || (command.find("RampDwnSpd") != std::string::npos)){
    factor = 1000.0 / 65535.0;
  }else if(command.find("STATUS1") != std::string::npos){
    factor = 1.0;
  }else{
    std::cout << "Error, command " << command << " not defined! " << std::endl;
  }


  usleep(20000);
  char buffer[200];
  sprintf(buffer,"pmt_read_reg %i %s\n",chan,command.c_str());
  std::cout <<"Command= " << command.c_str() << " (Chan=" << chan << ") ";
  int size=strlen(buffer);
  size = strlen(buffer);
  fSocket->write(buffer,size);
  char bigbuffer[50];
  size = sizeof(bigbuffer);

  usleep(30000);
  fSocket->read(bigbuffer,size);

  std::string readback(bigbuffer);
  int ifind = readback.find("+");
  if(ifind == std::string::npos){printf("No find string\n"); return -9999.0;}

  std::string valstring = readback.substr(0,ifind);
 
  long int value = atoi(valstring.c_str());
  float fvalue = ((float)value)*factor;
  std::cout << " |  readback: " << " " << value
            << " " << " " << fvalue << std::endl;

  return fvalue;

}


//Read value
float PMTControl::ReadValue(std::string command,int chan){

  //  std::cout <<"Read value" << std::endl;  
  int length = 7;
  float factor = 1.0;
  if(command.find("LI") != std::string::npos){
    factor = 0.001;
  }else if(command.find("LV") != std::string::npos){
    factor = 0.001;
  }else if(command.find("LS") != std::string::npos){
    length = 4;
  }else if(command.find("LG") != std::string::npos){
    length = 2;
  }else if(command.find("LR") != std::string::npos){
    length = 3;
  }else if(command.find("LH") != std::string::npos){
    length = 4;
  }else if(command.find("LD") != std::string::npos){
    length = 3;
  }else{
    std::cout << "Error, command " << command << " not defined! " << std::endl;
  }

  usleep(50000);
  char buffer[200];
  sprintf(buffer,"exec_pmt_cmd %s\n",command.c_str());
  std::cout <<"Command= " << command.c_str();
  int size=strlen(buffer);
  size = strlen(buffer);
  fSocket->write(buffer,size);
  char bigbuffer[50];
  size = sizeof(bigbuffer);

  struct timeval t1;
  gettimeofday(&t1, NULL);
  usleep(50000);
  fSocket->read(bigbuffer,size);
  struct timeval t2;
  gettimeofday(&t2, NULL);
  //double dtime = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec)/1000000.0;
  //  std::cout << "Time to read one value:: " << dtime*1000.0 << " ms " << std::endl;
  
  std::string readback(bigbuffer);
  //std::cout << "size: " << readback.size() << std::endl;
  //std::cout << "readback valuve: " << readback << std::endl;
  if(readback.size() < 4){ return -9999.0; }
  //  long int value = strtol(readback.substr(4,length).c_str(),NULL,0);
  long int value = atoi(readback.substr(4,length).c_str());
  float fvalue = ((float)value)*factor;
  std::cout << " readback: " <<  readback.substr(4,length) << " " << value 
	    << " " << " " << fvalue << std::endl;
  
  return fvalue;
}

int PMTControl::GetStatus(char *pevent, INT off)
{

  struct timeval t1;
  gettimeofday(&t1, NULL);



  std::vector<float> current(20,0);
  std::vector<float> read_volt(20,0);
  std::vector<float> set_volt(20,0);
  std::vector<float> state(20,0);
  std::vector<float> trip_state(20,0);
  std::vector<float> ramp_rate_up(20,0);
  std::vector<float> ramp_rate_down(20,0);

  for(int i = 0; i < 8; i++){
    //  printf("_____________ch %i _________\n",i);
    if(!fActivePMTs[i]) continue; // ignore inactive PMTs

    current[i] = ReadModbusValue("HVCurVal",i);
    read_volt[i] = ReadModbusValue("HVVolVal",i);
    set_volt[i] = ReadModbusValue("HVVolNom",i);
    state[i] = ReadModbusValue("STATUS1",i);
    //    trip_state[i] = ReadValue("01LD",0);
    ramp_rate_up[i] = ReadModbusValue("RampUpSpd",i);
    ramp_rate_down[i] = ReadModbusValue("RampDwnSpd",i);
    //    ramp_rate[i] = ReadValue("01LR",0);
    usleep(500);
  }

  
  float *pddata;  
  // Read currents from PMT
  char bank_name[20];
  sprintf(bank_name,"PMI%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata);
  printf("PMT current: ");
  for(int i = 0; i < 20; i++){ *pddata++ = current[i]; printf("%f ",current[i]); } printf("\n");
  bk_close(pevent, pddata);

  float *pddata2;
  // Readback voltages from PMT
  sprintf(bank_name,"PMV%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata2);
  printf("PMT read volt: ");
  for(int i = 0; i < 20; i++){ *pddata2++ = read_volt[i]; printf("%f ",read_volt[i]);}   printf("\n");
  bk_close(pevent, pddata2);
  
  float *pddata3;
  // measured voltages from PMT
  sprintf(bank_name,"PMH%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata3);  
  printf("PMT set volt: ");
  for(int i = 0; i < 20; i++){ *pddata3++ = set_volt[i]; printf("%f ",set_volt[i]);}    printf("\n");
  bk_close(pevent, pddata3);

  int *pddata4;
  // ON/OFF status from PMT
  sprintf(bank_name,"PMG%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_INT, (void**)&pddata4);
  printf("PMT state: ");
  for(int i = 0; i < 20; i++){ *pddata4++ = (((int)state[i]) & 0x3);printf("%f ",state[i]);}   printf("\n");
  bk_close(pevent, pddata4);

  int *pddata42;
  // error status from PMT
  sprintf(bank_name,"PME%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_INT, (void**)&pddata42);
  printf("PMT error: ");
  for(int i = 0; i < 20; i++){ *pddata42++ = (((int)state[i]) & 0x3c);printf("%f ",state[i]);}   printf("\n");
  bk_close(pevent, pddata42);

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


  float *pddata6;
  // ON/OFF status from PMT
  sprintf(bank_name,"PMU%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata6);
  printf("Ramp up: ");
  for(int i = 0; i < 20; i++){ *pddata6++ = ramp_rate_up[i];printf("%f ",ramp_rate_up[i]);}   printf("\n");
  bk_close(pevent, pddata6);

  float *pddata6b;
  // ON/OFF status from PMT
  sprintf(bank_name,"PMD%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata6b);
  printf("Ramp up: ");
  for(int i = 0; i < 20; i++){ *pddata6b++ = ramp_rate_down[i];printf("%f ",ramp_rate_down[i]);}   printf("\n");
  bk_close(pevent, pddata6b);

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


  struct timeval t2;
  gettimeofday(&t2, NULL);
  double dtime = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec)/1000000.0;                                                
  std::cout << "Time to finish reading all PMTs: " << dtime*1000.0 << " ms " << std::endl;                                         


  return bk_size(pevent);
}
