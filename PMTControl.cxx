
#include "PMTControl.h"
#include "time.h"
#include "sys/time.h"


// Check which PMTs are plugged in and responding
int PMTControl::CheckActivePMTs(){
  
  int npmts_active = 0;

  std::cout << "Check active " << std::endl;
  
  for(int i = 0; i <20; i++){

    usleep(50000);
    SetCommand("SetChannel",i);

    usleep(50000);
    std::cout << "__________________\nChecking chan " << i << std::endl;
    char buffer[200];
    for(int j = 0; j < 200; j++){buffer[j]=0;}
    sprintf(buffer,"get_PMT_FW %i \n",i);
    int size=strlen(buffer);
    size = strlen(buffer);
    std::cout << "custom command : " << buffer;
    usleep(50000);
    fSocket->write(buffer,size);
    char bigbuffer[50];
    for(int j = 0; j < 50; j++){bigbuffer[j]=0;}
    size = sizeof(bigbuffer);
    usleep(200000);
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

  }

  cm_msg(MINFO,"PMTControl::SetCommand","Number of active PMTs: %i",npmts_active);

  fFirstEvent = true;
  
  return npmts_active;
}
  


float PMTControl::GetModbusFactor(std::string command){

  // Different factors for different commands; also checks that commands are valid.

  float factor = 1500.0;
  if(command.find("HVCurVal") != std::string::npos){
    factor = 10.0 / 65535.0;
  }else if(command.find("HVVolVal") != std::string::npos){
    factor = 1500.0 / 65535.0;
  }else if(command.find("HVVolNom") != std::string::npos){
    factor = 1500.0 / 65535.0;
  }else if((command.find("RampUpSpd") != std::string::npos) || (command.find("RampDwnSpd") != std::string::npos)){
    factor = 1000.0 / 65535.0;
  }else if(command.find("HVCurrMax") != std::string::npos){
    factor = 10.0 / 65535.0;
  }else if(command.find("TripTime") != std::string::npos){
    factor = 1.0;
  }else if(command.find("MCUTemp") != std::string::npos){
    factor = 1.0;
  }else if(command.find("STATUS1") != std::string::npos){
    factor = 1.0;
  }else if(command.find("0x1") != std::string::npos){ // STATUS0 reg
    factor = 1.0;
  }else if(command.find("0x27") != std::string::npos){ // HV tolerance reg
    factor = 1500.0 / 65535.0;
  }else if(command.find("0x2B") != std::string::npos){ // HV tolerance reg
    factor = 1500.0 / 65535.0;
  }else if(command.find("0x2C") != std::string::npos){ // 5V rail measure
    factor = 5.0 / 65535.0;
  }else if(command.find("0x2D") != std::string::npos){ // HV2 disable registers
    factor = 1.0;
  }else if((command.find("SetChannel") != std::string::npos) || (command.find("pmt_toggle_hv") != std::string::npos)){
    ;
  }else{
    std::cout << "Error, command " << command << " not defined! " << std::endl;
    return -1;
  }  
  
  return factor;
}


// Define what do do with callbacks from watch function
void PMTControl::callback(midas::odb &o) {

  // Special command to set defaults for all channels.
  if(o.get_full_path().find("SetDefaults") != std::string::npos){
    int status = SetDefaults();
    printf("Return from setdefaults\n");
    return;
  }
  
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
    SetCommand("HVVolNom", o[gSelectedChannel],gSelectedChannel);
  }else if(o.get_full_path().find("HVRampRate") != std::string::npos){
    std::cout << "Channel " << gSelectedChannel << " ramp rate changed to " << o[gSelectedChannel] << std::endl;
    SetCommand("SR", o[gSelectedChannel],gSelectedChannel);
  }else if(o.get_full_path().find("HVenable") != std::string::npos){
    int state = (int)o[gSelectedChannel];
    if(state){
      cm_msg(MINFO,"PMTControl","Turning on HV for channel %i",gSelectedChannel);
      SetCommand("pmt_toggle_hv", 1,gSelectedChannel);
    }else{
      cm_msg(MINFO,"PMTControl","Turning off HV for channel %i",gSelectedChannel);
      SetCommand("pmt_toggle_hv", 0,gSelectedChannel);
    }
  }

}


bool PMTControl::SetCommand(std::string command, float value, int ch){

  float factor = GetModbusFactor(command);
  if(factor < 0) return false;

  // command string
  char buffer[200];
  if(command.compare("SetChannel") == 0){
    sprintf(buffer,"select_pmt %i \n",value);
  }else if(command.compare("pmt_toggle_hv") == 0){
    // Set toggle HV command
    sprintf(buffer,"pmt_toggle_hv %i %i \n",ch,(int)value);
  }else{
    // Use the correct factor to convert from physical units to digital units
    
    int int_value = 0;
    if(factor != 0){ int_value = (int)(value / factor);}

    // create write command
    sprintf(buffer,"pmt_write_reg %i %s %i \n",ch,command.c_str(),int_value);
  }

  usleep(20000);
  int size=sizeof(buffer);
  size = strlen(buffer);
  //    std::cout << "Set Command : " << buffer << " " << size << std::endl;

  fSocket->write(buffer,size);
  char bigbuffer[500];
  size = sizeof(bigbuffer);
  usleep(50000);
  fSocket->read(bigbuffer,size);
  
  std::string readback(bigbuffer);
  //std::cout << "readback for set command: " << readback << " | " << readback.size() << std::endl;

  // Check that the reply has the expected except substring
  if(readback.find("1+OK") == std::string::npos and !(command.compare("SetChannel") == 0)){
    cm_msg(MERROR,"PMTControl::SetCommand","Reply did not 1+OK: %s",readback.c_str());
    return false;    
  }

  //  std::cout << "Reply matches expectation 1+OK: " << readback << std::endl;

  return true;
}

bool PMTControl::SetDefaults(){

  for(int i = 0; i < 20; i++){
    if(!fActivePMTs[i]) continue; // ignore inactive PMTs                                                                                          
    
    usleep(100000);    
    SetCommand("RampUpSpd",20.0,i);
    usleep(10000);    
    SetCommand("RampDwnSpd",20.0,i);
    usleep(10000);
    SetCommand("TripTime",2.0,i);
    usleep(10000);
    SetCommand("HVCurrMax",6.0,i);
    usleep(10000);
    SetCommand("0x27",20.0,i);
    usleep(10000);
  }
  
  cm_msg(MINFO,"PMTControl::SetDefaults","Set default values for HV ramp rate and trip points");

  return true;

}


PMTControl::PMTControl(KOsocket *socket, int index){

  // Initialize some variables:
  std::cout << "Make variables" << std::endl;
  ramp_rate_up = std::vector<float>(20,0);
  ramp_rate_down = std::vector<float>(20,0);
  trip_time = std::vector<float>(20,0);
  trip_threshold = std::vector<float>(20,0);
  std::cout << "Made variables" << std::endl;

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
    {"HVRampRate", std::array<int, 20>{} },
    {"SetDefaults", false }

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
  
  // Set ramp and trip defaults
  if(0)  SetDefaults();

  // Disable the HV2 trip condition
  for(int i = 0; i < 20; i++){
    if(!fActivePMTs[i]) continue; // ignore inactive PMTs                                                                                          
    printf("Disable HV2 check %i\n",i);
    usleep(10000);
    SetCommand("0x2D",65534,i);
    usleep(10000);
  }

}

float PMTControl::ReadModbusValue(std::string command,int chan){

  float factor = GetModbusFactor(command);

  usleep(2000);
  char buffer[200];
  sprintf(buffer,"pmt_read_reg %i %s\n",chan,command.c_str());
  //std::cout <<"Command= " << command.c_str() << " (Chan=" << chan << ") ";
  int size=strlen(buffer);
  size = strlen(buffer);
  fSocket->write(buffer,size);
  char bigbuffer[50];
  size = sizeof(bigbuffer);

  usleep(2000);
  fSocket->read(bigbuffer,size);

  std::string readback(bigbuffer);
  int ifind = readback.find("+");
  if(ifind == std::string::npos){printf("No find string\n"); return -9999.0;}

  std::string valstring = readback.substr(0,ifind);
 
  long int value = atoi(valstring.c_str());
  float fvalue = ((float)value)*factor;
  if(0)  std::cout << " |  readback: " << " " << value
            << " " << " " << fvalue << std::endl;

  return fvalue;

}

std::vector<float> PMTControl::ReadMultiModbusValue(std::string command,int chan){


  //  printf("ReadMultiModbusValue \n");
  usleep(2000);
  char buffer[200];
  sprintf(buffer,"pmt_read_n_regs %i TripTime 4\n",chan);
  int size=strlen(buffer);
  size = strlen(buffer);
  fSocket->write(buffer,size);
  char bigbuffer[500];
  size = sizeof(bigbuffer);

  usleep(50000);
  fSocket->read(bigbuffer,size);

  std::string readback(bigbuffer);
  if(0)std::cout << "multi readback " << readback << " " << std::endl;
  int ifind = readback.find("1+OK+");
  if(ifind == std::string::npos){printf("No find string\n"); return std::vector<float>(1);}


  std::string valstring = readback.substr(5);

  if(0)  std::cout << "valstring " << valstring << std::endl;

  // valstring is 4 values separated by + and finish with \n
  int first_plus = valstring.find("+",0);
  int second_plus = valstring.find("+",first_plus+1);
  int third_plus = valstring.find("+",second_plus+1);

  if(0)std::cout << "Indices " << first_plus << " " << second_plus << " " 
	    << third_plus << " " << std::endl;

  std::string first_value = valstring.substr(0,first_plus-0);
  std::string second_value = valstring.substr(first_plus+1,second_plus-first_plus-1);
  std::string third_value = valstring.substr(second_plus+1,third_plus-second_plus-1);
  std::string fourth_value = valstring.substr(third_plus+1,third_plus+1);

  int ivalues[4];
  ivalues[0] =  atoi(first_value.c_str());
  ivalues[1] =  atoi(second_value.c_str());
  ivalues[2] =  atoi(third_value.c_str());
  ivalues[3] =  atoi(fourth_value.c_str());
  
  if(0)std::cout << "Set of values: " << first_value << " "
	    << second_value << " "
	    << third_value << " "
	    << fourth_value << " "
	    <<  std::endl;

  if(0)  std::cout <<  ivalues[0]<< " "
	    <<  ivalues[1]<< " "
	    <<  ivalues[2] << " "
	    <<  ivalues[3]<< " "
	    <<   std::endl;

  std::vector<float> fvalues(4);
  fvalues[0] = ((float)ivalues[0]) * GetModbusFactor("TripTime");
  fvalues[1] = ((float)ivalues[1]) * GetModbusFactor("RampUpSpd");
  fvalues[2] = ((float)ivalues[2]) * GetModbusFactor("RampDwnSpd");
  fvalues[3] = ((float)ivalues[3]) * GetModbusFactor("HVCurrMax");

  if(0){  std::cout << "fvalues: ";
  for(int i = 0; i < 4; i++) std::cout << fvalues[i] << " ";
  std::cout << "\n";
  }
  return fvalues;

}



std::vector<float> PMTControl::ReadFreqModbusValue(std::string command,int chan){

  
  usleep(2000);
  char buffer[200];
  sprintf(buffer,"pmt_get_frequent_regs %i\n",chan);
  int size=strlen(buffer);
  size = strlen(buffer);
  fSocket->write(buffer,size);
  char bigbuffer[500];
  size = sizeof(bigbuffer);

  usleep(50000);
  fSocket->read(bigbuffer,size);

  std::string readback(bigbuffer);
  int ifind = readback.find("1+OK+");
  if(ifind == std::string::npos){printf("No find string\n"); return std::vector<float>(1);}

  if(0)std::cout << "readback " << readback << " " << ifind << std::endl;

  std::string valstring = readback.substr(5);

  if(0)  std::cout << "valstring " << valstring << std::endl;

  // valstring is 5 values separated by + and finish with \n
  int first_plus = valstring.find("+",0);
  int second_plus = valstring.find("+",first_plus+1);
  int third_plus = valstring.find("+",second_plus+1);
  int fourth_plus = valstring.find("+",third_plus+1);

  if(0)std::cout << "Indices " << first_plus << " " << second_plus << " " 
	    << third_plus << " " << fourth_plus << std::endl;

  std::string first_value = valstring.substr(0,first_plus-0);
  std::string second_value = valstring.substr(first_plus+1,second_plus-first_plus-1);
  std::string third_value = valstring.substr(second_plus+1,third_plus-second_plus-1);
  std::string fourth_value = valstring.substr(third_plus+1,fourth_plus-third_plus-1);
  std::string fifth_value = valstring.substr(fourth_plus+1);

  int ivalues[5];
  ivalues[0] =  atoi(first_value.c_str());
  ivalues[1] =  atoi(second_value.c_str());
  ivalues[2] =  atoi(third_value.c_str());
  ivalues[3] =  atoi(fourth_value.c_str());
  ivalues[4] =  atoi(fifth_value.c_str());  
  
  if(0)std::cout << "Set of values: " << first_value << " "
	    << second_value << " "
	    << third_value << " "
	    << fourth_value << " "
	    << fifth_value << std::endl;

  if(0)  std::cout <<  ivalues[0]<< " "
	    <<  ivalues[1]<< " "
	    <<  ivalues[2] << " "
	    <<  ivalues[3]<< " "
	    <<  ivalues[4]<< std::endl;

  std::vector<float> fvalues(5);
  fvalues[0] = ((float)ivalues[0]) * GetModbusFactor("HVVolNom");
  fvalues[1] = ((float)ivalues[1]) * GetModbusFactor("HVCurVal");
  fvalues[2] = ((float)ivalues[2]) * GetModbusFactor("HVVolVal");
  fvalues[3] = ((float)ivalues[3]) * GetModbusFactor("STATUS1");
  fvalues[4] = ((float)ivalues[4]) * GetModbusFactor("MCUTemp");

  if(0){  std::cout << "fvalues: ";
  for(int i = 0; i < 5; i++) std::cout << fvalues[i] << " ";
  std::cout << "\n";
  }
  
  return fvalues;

}



int PMTControl::GetStatus(char *pevent, INT off)
{

  struct timeval t1;
  gettimeofday(&t1, NULL);
  printf("Get PMT status\n");


  std::vector<float> current(20,0);
  std::vector<float> read_volt(20,0);
  std::vector<float> set_volt(20,0);
  std::vector<float> state(20,0);
  std::vector<float> status0(20,0);
  std::vector<float> trip_state(20,0);
  std::vector<float> pmt_temp(20,0);
  std::vector<float> HV2Vol(20,0);
  std::vector<float> V5Val(20,0);

  printf("PMTs ");
  for(int i = 0; i < 20; i++){
    //  printf("_____________ch %i _________\n",i);
    if(!fActivePMTs[i]) continue; // ignore inactive PMTs
    SetCommand("SetChannel",i);
    usleep(1000);
    std::vector<float> values = ReadFreqModbusValue("HVCurVal",i);
    set_volt[i] = values[0];//0;//ReadModbusValue("HVVolNom",i);
    current[i] = values[1];//0;//ReadModbusValue("HVCurVal",i);
    read_volt[i] = values[2];//0;//ReadModbusValue("HVVolVal",i);
    state[i] = values[3];//0;//ReadModbusValue("STATUS1",i);
    pmt_temp[i] = values[4];//0;//ReadModbusValue("MCUTemp",i);
    //    fFirstEvent = 0;

    HV2Vol[i] = ReadModbusValue("0x2B",i);
    V5Val[i] = ReadModbusValue("0x2C",i);
    
    if(1 && fFirstEvent){ // Read some variables only on first event
      std::vector<float> values2 = ReadMultiModbusValue("",i);

      ramp_rate_up[i] =   values2[1]; 
      ramp_rate_down[i] = values2[2];
      trip_time[i] =      values2[0];
      trip_threshold[i] = values2[3];
      status0[i] = ReadModbusValue("0x1",i);

    }
    printf(".");
    usleep(500);
  }
  printf(" done\n");

  
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
  printf("PMT status1: ");
  for(int i = 0; i < 20; i++){ *pddata42++ = (((int)state[i]));printf("%f ",state[i]);}   printf("\n");
  bk_close(pevent, pddata42);

  int *pddata43;
  // error status from PMT
  sprintf(bank_name,"PM0%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_INT, (void**)&pddata43);
  printf("PMT status0: ");
  for(int i = 0; i < 20; i++){ *pddata43++ = (((int)status0[i]));printf("%f ",status0[i]);}   printf("\n");
  bk_close(pevent, pddata43);


  float *pddata_tmp;
  // PMT Temp
  sprintf(bank_name,"PMC%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata_tmp);
  for(int i = 0; i < 20; i++){ *pddata_tmp++ = pmt_temp[i];}
  bk_close(pevent, pddata_tmp);

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


  float *pddata7a;
  // Trip time
  sprintf(bank_name,"PMT%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata7a);
  for(int i = 0; i < 20; i++){ *pddata7a++ = trip_time[i];}
  bk_close(pevent, pddata7a);


  float *pddata7b;
  // Trip threshold
  sprintf(bank_name,"PMM%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata7b);
  for(int i = 0; i < 20; i++){ *pddata7b++ = trip_threshold[i];}
  bk_close(pevent, pddata7b);


  float *pddata_hv2;
  sprintf(bank_name,"P2%02i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata_hv2);
  for(int i = 0; i < 20; i++){ *pddata_hv2++ = HV2Vol[i];printf("%f ",HV2Vol[i]);} printf("\n");
  bk_close(pevent, pddata_hv2);


  float *pddata_v5;
  sprintf(bank_name,"P5%02i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata_v5);
  for(int i = 0; i < 20; i++){ *pddata_v5++ = V5Val[i];printf("%f ",V5Val[i]);} printf("\n");
  bk_close(pevent, pddata_v5);

  struct timeval t2;
  gettimeofday(&t2, NULL);
  double dtime = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec)/1000000.0;                                                
  std::cout << "Time to finish reading all PMTs: " << dtime*1000.0 << " ms " << std::endl;                                         

  fFirstEvent = false;

  return bk_size(pevent);
}
