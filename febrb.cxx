/******************************************************************** \

Control and slow readout of BRB (Big Red Board), aka mPMT mainboard

  $Id$
\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "iostream"
#include "KOsocket.h"
#include "PMTControl.h"

#include "odbxx.h"
#include "midas.h"
#include "mfe.h"
#include "unistd.h"
#include "time.h"
#include "sys/time.h"
#include <stdint.h>


#define  EQ_NAME   "BRB"
#define  EQ_EVID   1
#define  EQ_TRGMSK 0x1111

/* Hardware */
extern HNDLE hDB;
/* make frontend functions callable from the C framework */
extern BOOL equipment_common_overwrite = false;
/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "febrb";
/* The frontend file name, don't change it */
const char *frontend_file_name = (char*)__FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 000;

/* maximum event size produced by this frontend */
INT max_event_size = 32 * 34000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 2 * max_event_size + 10000;


/*-- Function declarations -----------------------------------------*/
INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();
extern void interrupt_routine(void);
INT read_slow_control(char *pevent, INT off);
INT read_pmt_status(char *pevent, INT off);


/*-- Equipment list ------------------------------------------------*/
#undef USE_INT
EQUIPMENT equipment[] = {

  { EQ_NAME "%02d",                 /* equipment name */
    {
      EQ_EVID, EQ_TRGMSK,     /* event ID, trigger mask */
      "SYSTEM",              /* event buffer */
      EQ_PERIODIC ,      /* equipment type */
      LAM_SOURCE(0, 0x8111),     /* event source crate 0, all stations */
      "MIDAS",                /* format */
      TRUE,                   /* enabled */
      RO_ALWAYS | RO_ODB,             /* read always */
      500,                    /* poll for 500ms */
      0,                      /* stop run after this event limit */
      0,                      /* number of sub events */
      1,                      /* do log history */
      "", "", "",
    },
    read_slow_control,       /* readout routine */
  }, 
  { "PMTS"  "%02d",                 /* equipment name */
    {
      EQ_EVID, EQ_TRGMSK,     /* event ID, trigger mask */
      "SYSTEM",              /* event buffer */
      EQ_PERIODIC ,      /* equipment type */
      LAM_SOURCE(0, 0x8111),     /* event source crate 0, all stations */
      "MIDAS",                /* format */
      FALSE,                   /* enabled */
      RO_ALWAYS | RO_ODB,             /* read always */
      1000,                    /* poll for 500ms */
      0,                      /* stop run after this event limit */
      0,                      /* number of sub events */
      1,                      /* do log history */
      "", "", "",
    },
    read_pmt_status,       /* readout routine */
  },
 {""}
};



/********************************************************************\
              Callback routines for system transitions

  These routines are called whenever a system transition like start/
  stop of a run occurs. The routines are called on the following
  occations:

  frontend_init:  When the frontend program is started. This routine
                  should initialize the hardware.

  frontend_exit:  When the frontend program is shut down. Can be used
                  to releas any locked resources like memory, commu-
                  nications ports etc.

  begin_of_run:   When a new run is started. Clear scalers, open
                  rungates, etc.

  end_of_run:     Called on a request to stop a run. Can send
                  end-of-run event and close run gates.

  pause_run:      When a run is paused. Should disable trigger events.

  resume_run:     When a run is resumed. Should enable trigger events.
\********************************************************************/

/********************************************************************/

int gAddress; // BRB address
int gPort; // BRB port
int gSelectADC; // Which ADC to use?

KOsocket *gSocket;
PMTControl *pmts = 0;



// Function for sending command to BRB and receiving requests
void SendBrbCommand(std::string command){

  midas::odb o = {
    {"host", "brb00"},
    {"port", 40},
  };

  char eq_dir[200];
  sprintf(eq_dir,"/Equipment/BRB%02i/Settings",get_frontend_index());
  o.connect(eq_dir);

  //  gSocket = new KOsocket(o["host"], o["port"]);
  if(gSocket->getErrorCode() != 0){
    cm_msg(MERROR,"init","Failed to connect to host; hostname/port = %s %i",((std::string)o["host"]).c_str(),(int)o["port"]);
  }


  usleep(200000);

  char buffer[200];
  char bigbuffer[500];
  int size=sizeof(buffer);
  int size2 = sizeof(bigbuffer);
  size = command.size();

  sprintf(buffer,"%s",command.c_str());
  gSocket->write(buffer,size);
  usleep(50000);
  int val = gSocket->read(bigbuffer,size2);
  std::cout << command << " (" << val << ") : " << bigbuffer ;
  usleep(100000);

  //  gSocket->shutdown();
  usleep(100000);

}

// Function for reading return from  command to BRB and receiving requests
std::string ReadBrbCommand(std::string command){

  midas::odb o = {
    {"host", "brb00"},
    {"port", 40},
  };

  char eq_dir[200];
  sprintf(eq_dir,"/Equipment/BRB%02i/Settings",get_frontend_index());
  o.connect(eq_dir);

  //  gSocket = new KOsocket(o["host"], o["port"]);
  //if(gSocket->getErrorCode() != 0){
  //cm_msg(MERROR,"init","Failed to connect to host; hostname/port = %s %i",((std::string)o["host"]).c_str(),(int)o["port"]);
  //}

  usleep(200000);

  char buffer[200];
  char bigbuffer[500];
  int size=sizeof(buffer);
  int size2 = sizeof(bigbuffer);
  size = command.size();

  sprintf(buffer,"%s",command.c_str());
  gSocket->write(buffer,size);
  usleep(100000);
  gSocket->read(bigbuffer,size2);
  //int val = gSocket->read(bigbuffer,size2);
  //  std::cout << "Send command: " << command << " (" << val << ") : " << bigbuffer ;
  usleep(150000);



  std::string rstring(bigbuffer);

  std::size_t current;
  current = rstring.find("\r");
  std::string rstring2 = rstring.substr(0, current);
  if(0)std::cout << "\n before=" << rstring << "\n after="
  	    << rstring2 << "\n";
  //gSocket->shutdown();
  usleep(100000);

  return rstring2;


}


midas::odb lpc_watch;

void lpc_callback(midas::odb &o) {

  int gSelectedChannel = o.get_last_index();
  std::cout << "Change value for " << gSelectedChannel << " " << o.get_full_path() << " " << o << std::endl;
  // check that the selected PMT is active                                                                                                         
 
  // Handle Turning on/off fast LED
  if(o.get_full_path().find("EnableFastLED") != std::string::npos){
    
    // Turn on or off?
    if(o){ //turn on
      
      // Turn on fast LED
      SendBrbCommand("enable_fast_led \r\n");

      cm_msg(MINFO,"lpc_callback","Enabling fast LED");
      
    }else{ // turn off

      // Turn off fast LED
      SendBrbCommand("disable_fast_led \r\n");

      cm_msg(MINFO,"lpc_callback","Disabling fast LED");
      
    }
  }else if(o.get_full_path().find("EnableSlowLED") != std::string::npos){
    
    // Turn on or off?
    if(o){ //turn on

      // Enable VCC LDO and DAC control
      SendBrbCommand("enable_ldo_en\r\n");
      SendBrbCommand("enable_vccl_en\r\n");
      SendBrbCommand("enable_mezzanine_dac\r\n");
      
      // Turn on slow LED
      sleep(1);
      SendBrbCommand("turn_slow_leds_on\r\n");

      cm_msg(MINFO,"lpc_callback","Enabling slow LEDs");
      
    }else{ // turn off

      // Turn off fast LED
      SendBrbCommand("turn_slow_leds_off\r\n");

      cm_msg(MINFO,"lpc_callback","Disabling slow LEDs");
      
    }

  }else if(o.get_full_path().find("LED_DAC") != std::string::npos){

    // Set DAC value
    int dac = o;
    // Sanity checks
    if(dac > 255) dac = 255;
    if(dac < 0) dac = 0;
    
    char buffer[256];
    sprintf(buffer,"write_mezzanine_dac 1 1 %i 1 \r\n",dac);
    SendBrbCommand(buffer);

    cm_msg(MINFO,"lpc_callback","Setting LPC DAC to %i",dac);

  }

}


/*-- Frontend Init -------------------------------------------------*/
INT frontend_init()
{


  // Setup the socket connection
  // using new C++ ODB!
  
  //  midas::odb::set_debug(true);
  char names1[200], names2[200], names3[200], names4[200], names5[200], names6[200];
  sprintf(names1,"Names BRV%i",get_frontend_index());
  sprintf(names2,"Names BRT%i",get_frontend_index());
  sprintf(names3,"Names BRH%i",get_frontend_index());
  sprintf(names4,"Names BRC%i",get_frontend_index());
  sprintf(names5,"Names BRL%i",get_frontend_index());
  sprintf(names6,"Names BRM%i",get_frontend_index());
 
  midas::odb o = {
    {"host", "brb00"},
    {"port", 40},
    {names1, std::array<std::string, 18>{}},
    {names2, std::array<std::string, 6>{}},
    {names3, std::array<std::string, 2>{}},
    {names4, std::array<std::string, 1>{}},
    {names5, std::array<std::string, 4>{}},
    {names6, std::array<std::string, 4>{}},
  };

  std::cout << "Names : " << names2 << std::endl;

  // Set the names for the ODB keys
  o[names1][0] = "+6V Amp Current";
  o[names1][1] = "+6V Amp Voltage";
  o[names1][2] = "-5V PMT Current";
  o[names1][3] = "-5V PMT Voltage";
  o[names1][4] = "+1.8V ADC Current";
  o[names1][5] = "+1.8V ADC Voltage";
  o[names1][6] = "+5V PMT Current";
  o[names1][7] = "+5V PMT Voltage";
  o[names1][8] = "+3.3V PMT Current";
  o[names1][9] = "+3.3V PMT Voltage";
  o[names1][10] = "-4V Amp Current";
  o[names1][11] = "-4V Amp Voltage";
  o[names1][12] = "+12V POE Current";
  o[names1][13] = "+12V POE Voltage";
  o[names1][14] = "+3.3V non-SoM Current";
  o[names1][15] = "+3.3V non-SoM Voltage";
  o[names1][16] = "+12V SoM Current";
  o[names1][17] = "+12V SoM Voltage";

  o[names2][0] = "ADC3 Temp";
  o[names2][1] = "Temp2";
  o[names2][2] = "Temp3";
  o[names2][3] = "Pressure Sensor Temp";
  o[names2][4] = "RTC Temp";
  o[names2][5] = "Humidity Sensor Temp";

  o[names3][0] = "Humidity";
  o[names3][1] = "Pressure";

  o[names4][0] = "Clock Status";

  o[names5][0] = "Fast LED Enabled";
  o[names5][1] = "Fast LED Mode";
  o[names5][2] = "LED DAC";
  o[names5][3] = "Slow LED Enabled";

  o[names6][0] = "Mag Field X (gauss)";
  o[names6][1] = "Mag Field Y (gauss)";
  o[names6][2] = "Mag Field Z (gauss)";
  o[names6][3] = "Mag Field tota (gauss)";

  char eq_dir[200];
  sprintf(eq_dir,"/Equipment/BRB%02i/Settings",get_frontend_index());
  o.connect(eq_dir);

  // Open socket to BRB
  std::cout << "Opening socket to  " << o["host"] << ":" << o["port"] << std::endl;
  gSocket = new KOsocket(o["host"], o["port"]);
  if(gSocket->getErrorCode() != 0){
    cm_msg(MERROR,"init","Failed to connect to host; hostname/port = %s %i",((std::string)o["host"]).c_str(),(int)o["port"]);
    return FE_ERR_HW;
  }

  std::cout << "Finished;... " << gSocket->getErrorCode() << std::endl;  
  std::cout << "Socket status : " << gSocket->getErrorString() << std::endl;
  //  gSocket->shutdown();
  //  SendBrbCommand("custom_command SET_PMT_UART_TIMEOUT_MS 50\r\n");

  for(int i = 0 ; i < 2;++i){
    usleep(50000);
    
    std::string temp = ReadBrbCommand("get_pressure_sensor_temp\n");
    printf("Temp %s\n",temp.c_str());

    std::string temp2 = ReadBrbCommand( "ldo_get_voltage 1\n");
    printf("LDO1: %s\n",temp2.c_str());;
  }


  std::string hw_version = ReadBrbCommand("get_hw_version\r\n");
  std::string sw_version = ReadBrbCommand("get_sw_version\r\n");

  cm_msg(MINFO,"init","BRB Firmware HW version: %s",hw_version.c_str()); 
  cm_msg(MINFO,"init","BRB Firmware SW version: %s",sw_version.c_str()); 

  // Setup control of PMTs
  std::cout << "Setting up PMTs.  Reset addresses" <<std::endl;
  // Need to reset addresses first
  //  SendBrbCommand("reset_all_PMT_addresses\n");
  std::cout << "Checking for active PMTs" <<std::endl;
  pmts = new PMTControl(gSocket, get_frontend_index());
  std::cout << "Finished setting up PMTs" << std::endl;

  // initialize the temp correction for magnetometer
  SendBrbCommand("mmeter_get_new_offset \r\n");


  // Set LPC to external trigger and set DAC to minimum light
  SendBrbCommand("select_sync_to_external_fast_led \r\n");
  SendBrbCommand("enable_mezzanine_dac \r\n");
  SendBrbCommand("write_mezzanine_dac 1 1 255 1 \r\n");
  cm_msg(MINFO,"init","Setting LPC to use external trigger"); 

  // Setup the LPC ODB keys and setup callback

  midas::odb lpc = {
    {"EnableFastLED", false },
    {"EnableSlowLED", false },
    {"LED_DAC", 255 }
  };

  sprintf(eq_dir,"/Equipment/BRB%02i/Settings/LPC",get_frontend_index());
  // printf("Connecting ODB directory %s\n",eq_dir);
  lpc.connect(eq_dir);

  // Setup the DB watch for the LPC settings
  lpc_watch.connect(eq_dir);
  std::function<void(midas::odb &o)> f2 = [=](midas::odb &o) {  lpc_callback(o);  };
  lpc_watch.watch(f2);
  std::cout << "Finished setting up LPC ODB" << std::endl;

  return SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/
INT frontend_exit()
{

  std::cout << "Closing socket." << std::endl;
  gSocket->shutdown();

  return SUCCESS;
}


/*-- Begin of Run --------------------------------------------------*/
INT begin_of_run(INT run_number, char *error)
{


  // Setup the ADC readout...
  char buffer[200];

  // Get ODB values (new C++ ODB!)
  midas::odb o = {
    {"testPatternADC", false},
    {"enableSoftwareTrigger", false },
    {"soft trigger rate", 450.0f },
    {"channel mask", 0x1f },
    {"trigger delay", 250 }
  };
  
  char eq_dir[200];
  sprintf(eq_dir,"/Equipment/BRB%02i/Settings",get_frontend_index());
  o.connect(eq_dir);

  std::cout << "Connected?  " << o.is_connected_odb() << std::endl;

  // Use the test pattern, if requested
  BOOL testPattern = o["testPatternADC"];
  
  // Do settings for each ADC
  if(1)  for(int i = 0; i < 5; i++){
    
    // Use offset binary encoding (rathers than twos complement)
    //sprintf(buffer,"custom_command set_adc_data_format %i 1\r\n",i);
    //SendBrbCommand(buffer);

    if(testPattern){
      cm_msg(MINFO,"BOR","Using test pattern for ADC %i",i);
      sprintf(buffer,"enable_adc_test_signals %i\r\n",i);
      SendBrbCommand(buffer);
      for(int j = 0; j < 4; j++){
	sprintf(buffer,"set_adc_test_signal_type %i %i 9 \r\n",i,j);
	SendBrbCommand(buffer);
      }
    }else{
      std::cout << "Not using test pattern " << std::endl;
      sprintf(buffer,"disable_adc_test_signals %i\r\n",i);
      SendBrbCommand(buffer);
      for(int j = 0; j < 4; j++){
	sprintf(buffer,"set_adc_test_signal_type %i %i 0 \r\n",i,j);
	SendBrbCommand(buffer);
      }
    }
  }


  // Set the trigger delay as per ODB
  sprintf(buffer,"SET_PRE_TRIGGER_DELAY %i\r\n",(int)(o["trigger delay"]));
  cm_msg(MINFO,"BOR","Setting pre-trigger delay to %i",(int)(o["trigger delay"]));
  SendBrbCommand(buffer);

  // Set the channel mask
  unsigned int mask = (unsigned int)(o["channel mask"]) & 0x1ff;
  sprintf(buffer,"set_adc_mask  0x%x \n",mask);
  cm_msg(MINFO,"BOR","Setting ADC mask to 0x%x",mask);
  SendBrbCommand(buffer);

  // Start the events
  usleep(20000);
  BOOL software_trigger = (bool)(o["enableSoftwareTrigger"]);
  if(software_trigger){
    cm_msg(MINFO,"BOR","Enabling software trigger with rate = %f", (float)(o["soft trigger rate"]));

    // Set the trigger rate as per ODB
    sprintf(buffer,"set_trigger_freq %i \n",(int)(o["soft trigger rate"]));
    SendBrbCommand(buffer);

    sprintf(buffer,"set_emulated_trigger_speed %i \n",(int)(o["soft trigger rate"]));
    SendBrbCommand(buffer);

    
    SendBrbCommand("ENABLE_EMULATED_TRIGGER\r\n");


    
  }else{
    cm_msg(MINFO,"BOR","Disabling software trigger");
    SendBrbCommand("DISABLE_EMULATED_TRIGGER\r\n");
  }

  usleep(100000);
  SendBrbCommand("set_num_samples_per_packet 4096\n");
  

  usleep(1000000);


  // Check which PMTs are active.
  if(pmts){
    // Need to reset addresses first
    //printf("Resetting all PMT addresses\n");
    //SendBrbCommand("reset_all_PMT_addresses\n");
    pmts->CheckActivePMTs();
  }

  // initialize the temp correction for magnetometer
  SendBrbCommand("mmeter_get_new_offset \r\n");

  usleep(1000000);

  
  //  SendBrbCommand("udp_stream_start 0 192.168.0.253 1500\r\n");
  // start acquisition... send to port 1500
  SendBrbCommand("start_periodic_acquisition_ext_trigger 192.168.0.253 1500 1\n");
  usleep(200000);


  //------ FINAL ACTIONS before BOR -----------
  printf("End of BOR\n");

  return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{


  // Stop the events                                                                                                                                                      
  //  SendBrbCommand("custom_command disable_dsp_processing \r\n");
  SendBrbCommand("stop_acquisition \n");
  usleep(200000);



  printf("EOR\n");
  
  return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/
INT frontend_loop()
{

  return SUCCESS;
}

/*------------------------------------------------------------------*/
/********************************************************************\
  Readout routines for different events
\********************************************************************/
int Nloop, Ncount;

/*-- Trigger event routines ----------------------------------------*/
 INT poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event
   is available. If test equals TRUE, don't return. The test
   flag is used to time the polling */
{
  register int i;  // , mod=-1;
  register int lam = 0;

  for (i = 0; i < count; i++) {
    
    if (lam) {
      if (!test){
        return lam;
      }
    }
  }
  return 0;
}

/*-- Interrupt configuration ---------------------------------------*/
 INT interrupt_configure(INT cmd, INT source, POINTER_T adr)
{
  switch (cmd) {
  case CMD_INTERRUPT_ENABLE:
    break;
  case CMD_INTERRUPT_DISABLE:
    break;
  case CMD_INTERRUPT_ATTACH:
    break;
  case CMD_INTERRUPT_DETACH:
    break;
  }
  return SUCCESS;
}

int dummy_counter = 0;
struct timeval last_event_time;  


// Function for returning a BRB value
float get_brb_value(std::string command, bool with_ok=false){


  usleep(2500);

  // Read temperature                                                                                                                                                                               
  char buffer[200];
  sprintf(buffer,"%s\r\n",command.c_str());
  //  printf("command: %s\n",command.c_str());
  int size=sizeof(buffer);
  size = command.size() + 2;

  //std::cout << "w";
  gSocket->write(buffer,size);

  usleep(50000);
  char bigbuffer[500];
  size = sizeof(bigbuffer);
  //std::cout << "r";
  gSocket->read(bigbuffer,size);
  //std::cout << "d";
  std::string readback(bigbuffer);
  std::vector<std::string> values;
  std::size_t current, previous = 0;
  current = readback.find("\r");
  while (current != std::string::npos) {
    values.push_back(readback.substr(previous, current - previous));
    previous = current + 1;
    current = readback.find("\r", previous);
  }
  values.push_back(readback.substr(previous, current - previous));

  float value = strtof (values[0].c_str(), NULL);
  if(with_ok){
    std::vector<std::string> values2;
    std::size_t current, previous = 0;
    current = values[0].find("+");
    while (current != std::string::npos) {
      values2.push_back(values[0].substr(previous, current - previous));
      previous = current + 1;
      current = values[0].find("+", previous);
    }
    values2.push_back(values[0].substr(previous, current - previous));
    
    //    std::cout << "with ok Values: " << values2.size() << " " << values2[0] << " " << values2[2] << std::endl;
    if(values2.size() == 3) value = strtof (values2[2].c_str(), NULL);

  }

  usleep(250);

  return value;


}

#include <math.h>   

  static int mag_counter= 0;

/*-- Event readout -------------------------------------------------*/
INT read_slow_control(char *pevent, INT off)
{

  printf("BRB read_slow_controlk\n");
  sleep(1);

  bk_init32(pevent);

  float *pddata, *ptmp;
  char command[200];
  
  // Bank names must be exactly four char
  char bank_name[20];
  sprintf(bank_name,"BRV%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata);

  ptmp = pddata;
  for(int j = 0; j < 8; j++){

    double resistor = 1.0;
    if(j==0){ resistor = 0.1; }
    if(j==1){ resistor = 0.03; }
    if(j==2){ resistor =150; }
    if(j==4){ resistor = 0.05;}
    if(j==5){ resistor =0.03; }
    if(j==6){ resistor =0.03; }
    if(j==7){ resistor =0.05;}


    struct timeval t1;  
    gettimeofday(&t1, NULL);

    // Read voltage
    sprintf(command,"ldo_get_voltage %i",j+1);
    double voltage = get_brb_value(command);
    sprintf(command,"ldo_get_shunt_voltage %i",j+1);
    double shunt_voltage = get_brb_value(command);
    
    float shunt_current = 1000.0*shunt_voltage/resistor;
    if(j==2){
      shunt_current = 1000.0*(shunt_voltage/resistor)/0.0004;
    }
    *pddata++ = shunt_current;
    
    *pddata++ = voltage;


  }

  // Calculate the 12V power used by SoM
  double power_12VPOE = -ptmp[12] * 12.0 / 1000.0;
  double power_6Vamp  = ptmp[0] * 6.0 / 1000.0;
  double power_n5Vpmt = ptmp[2] * 5.0 / 1000.0;
  double power_1_8Vadc = ptmp[4] * 1.8 / 1000.0;
  double power_n4Vamp = ptmp[10] * 4.0 / 1000.0;
  double power_3_3Vclock = ptmp[14] * 3.3 / 1000.0;
  double power_som = power_12VPOE - power_6Vamp - power_n5Vpmt - power_1_8Vadc - power_n4Vamp - power_3_3Vclock;

  std::cout << "POE = " << power_12VPOE 
	    << "W, 6V(amp) = " << power_6Vamp
	    << "W, -5V(PMT) = " << power_n5Vpmt 
	    << "W, 1.8V(ADC) = " << power_1_8Vadc
	    << "W, -4V(amp) = " << power_n4Vamp
	    << "W, 3.3V(clock) = " << power_3_3Vclock
	    << "W, 12V(SOM) = " << power_som << "W" << std::endl;
    
  *pddata++ = power_som / 12.0;
  *pddata++ = 12.0;


  bk_close(pevent, pddata);	


  
  // Get temperatures

  float *pddata2;
  
  sprintf(bank_name,"BRT%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata2);

  std::cout << "Temp: ";
  for(int j = 1; j < 4; j++){

    // Read temperature
    sprintf(command,"get_temp %i",j);
    float temperature = get_brb_value(command);
    std::cout << temperature << " " ; 
    *pddata2++ = temperature;

  }

  float temperature1 = get_brb_value("get_pressure_sensor_temp",true);
  *pddata2++ = temperature1;

  float temperature2 = get_brb_value("get_fpga_temp",true);
  *pddata2++ = temperature2;

  float temperature3 = get_brb_value("get_hdc1080_temp",true);
  *pddata2++ = temperature3;
  
  std::cout << " " << temperature1 << " " 
	    << temperature2 << " "
	    << temperature3 << " "
	    << std::endl;

  bk_close(pevent, pddata2);	



  // Save humidity and pressure
  float *pddata3;

  sprintf(bank_name,"BRH%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata3);

  float humidity = get_brb_value("get_humidity",true);
  float pressure = get_brb_value("get_pressure",true);
  
  *pddata3++ = humidity;
  *pddata3++ = pressure;

  std::cout << "Pressure/Humidity : " << pressure << " " << humidity << std::endl;

  bk_close(pevent, pddata3);

  
  //Clock status
  int *pddata4;

  sprintf(bank_name,"BRC%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_INT, (void**)&pddata4);

  float clock_status = 0.0;//get_brb_value("custom_command get_clnr_status_pins",true);

  printf("Clock status %f\n",clock_status);

  *pddata4++ = (int)clock_status;

  bk_close(pevent, pddata4);



  //LPC status
  int *pddata5;

  sprintf(bank_name,"BRL%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_INT, (void**)&pddata5);

  float led_fast_status = 0;//get_brb_value("get_enable_fast_led_status",true);
  float led_fast_mode = 0;//get_brb_value("get_fast_led_mode",true);
  float led_dac = 0.0;//get_brb_value("get_mezzanine_dac",true);
  float led_slow_status = 0;
  //  printf("FAST LED status %f %f %f\n",led_fast_status, led_fast_mode,led_dac);

  *pddata5++ = (int)led_fast_status;
  *pddata5++ = (int)led_fast_mode;
  *pddata5++ = (int)led_dac;
  *pddata5++ = (int)led_slow_status;

  bk_close(pevent, pddata5);

  //magnetometer status
  float *pddata6;

  sprintf(bank_name,"BRM%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata6);

  float mag_x = get_brb_value("mmeter_read_mag_field 0",true);
  float mag_y = get_brb_value("mmeter_read_mag_field 1",true);
  float mag_z = get_brb_value("mmeter_read_mag_field 2",true);
  float mag_tot = sqrt(mag_x*mag_x + mag_y*mag_y + mag_z*mag_z);
  printf("mag status %f %f %f %f\n",mag_x,mag_y,mag_z,mag_tot);

  *pddata6++ = mag_x;
  *pddata6++ = mag_y;
  *pddata6++ = mag_z;
  *pddata6++ = mag_tot;

  bk_close(pevent, pddata6);


  // re-initialize the temp correction for magnetometer
  // Do this about every 60 minutes.
  mag_counter++;
  printf("%i %i\n",mag_counter,mag_counter%500);
  if(mag_counter%1200 == 0){ // 0.2Hz trigger rate * 60 sec * 60 minutes = 18000
    SendBrbCommand("mmeter_get_new_offset \r\n");
    cm_msg(MINFO,"febrb_readout","Updating the magnetometer temperature offsets");
    
  }

  return bk_size(pevent);

}
 
/*-- Event readout -------------------------------------------------*/
INT read_pmt_status(char *pevent, INT off)
{

  bk_init32(pevent);

  return pmts->GetStatus(pevent, off);
  return 0;
}

  


 
