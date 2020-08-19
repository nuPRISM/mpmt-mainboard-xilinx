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
      TRUE,                   /* enabled */
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

  char buffer[200];
  char bigbuffer[500];
  int size=sizeof(buffer);
  int size2 = sizeof(bigbuffer);

  sprintf(buffer,"%s",command.c_str());
  gSocket->write(buffer,size);
  int val = gSocket->read(bigbuffer,size2);
  std::cout << command << " (" << val << ") : " << bigbuffer ;
  usleep(150000);

}

/*-- Frontend Init -------------------------------------------------*/
INT frontend_init()
{


  // Setup the socket connection
  // using new C++ ODB!
  
  midas::odb::set_debug(true);
  midas::odb o = {
    {"host", "brb00"},
    {"port", 40},
  };


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

  SendBrbCommand("custom_command SET_PMT_UART_TIMEOUT_MS 50\r\n");


  // Setup control of PMTs
  pmts = new PMTControl(gSocket, get_frontend_index());



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
    {"channel mask", 0x1f }
  };
  
  //  o.connect("/Equipment/BRB/Settings");
  char eq_dir[200];
  sprintf(eq_dir,"/Equipment/BRB%02i/Settings",get_frontend_index());
  o.connect(eq_dir);

  // Use the test pattern, if requested
  BOOL testPattern = o["testPatternAdc"];
  
  // Do settings for each ADC
  for(int i = 61; i < 66; i++){
    
    // Use offset binary encoding (rathers than twos complement)
    sprintf(buffer,"custom_command set_adc_data_format %i 1\r\n",i);
    SendBrbCommand(buffer);

    if(testPattern){
      cm_msg(MINFO,"BOR","Using test pattern for ADC %i",i);
      sprintf(buffer,"custom_command enable_adc_test_signals %i\r\n",i);
      SendBrbCommand(buffer);
      for(int j = 0; j < 5; j++){
	sprintf(buffer,"custom_command set_adc_test_signal_type %i %i 9 \r\n",i,j);
	SendBrbCommand(buffer);
      }
    }else{
      std::cout << "Not using test pattern " << std::endl;
      sprintf(buffer,"custom_command disable_adc_test_signals %i\r\n",i);
      SendBrbCommand(buffer);
      for(int j = 0; j < 5; j++){
	sprintf(buffer,"custom_command set_adc_test_signal_type %i %i 0 \r\n",i,j);
	SendBrbCommand(buffer);
      }
    }
  }


  // Set the trigger rate as per ODB
  sprintf(buffer,"custom_command SET_EMULATED_TRIGGER_SPEED %f\r\n",(float)(o["soft trigger rate"]));
  SendBrbCommand(buffer);

  // Set the channel mask
  unsigned int mask = (unsigned int)(o["channel mask"]) & 0x1ff;
  sprintf(buffer,"custom_command set_acquired_masks  0 0x%x \r\n",mask);
  SendBrbCommand(buffer);

  // Set the Number samples
  //SendBrbCommand("custom_command SELECT_NUM_SAMPLES_TO_SEND_TO_UDP 512\r\n");
  SendBrbCommand("custom_command SELECT_NUM_SAMPLES_TO_SEND_TO_UDP 1024\r\n");
  usleep(200000);
  SendBrbCommand("custom_command CHANGE_STREAMING_PARAMS \r\n");

  usleep(200000);
  SendBrbCommand("custom_command SET_PMT_UART_TIMEOUT_MS 50\r\n");


  // Start the events
  usleep(20000);
  BOOL software_trigger = (bool)(o["enableSoftwareTrigger"]);
  if(software_trigger){
      cm_msg(MINFO,"BOR","Enabling software trigger");
      SendBrbCommand("uart_regfile_ctrl_write 0 1 1 0\r\n");
  }else{
      cm_msg(MINFO,"BOR","Disabling software trigger");
    SendBrbCommand("uart_regfile_ctrl_write 0 1 0 0\r\n");
  }
  usleep(200000);
  SendBrbCommand("custom_command enable_dsp_processing \n");
  usleep(1000000);
  SendBrbCommand("udp_stream_start 0 192.168.0.253 1500\r\n");
  usleep(200000);

  // Check which PMTs are active.
  pmts->CheckActivePMTs();


  //------ FINAL ACTIONS before BOR -----------
  printf("End of BOR\n");

  return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{


  // Stop the events                                                                                                                                                      
  SendBrbCommand("custom_command disable_dsp_processing \r\n");
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
float get_brb_value(std::string command){

  // Read temperature                                                                                                                                                                               
  char buffer[200];
  sprintf(buffer,"%s\r\n",command.c_str());
  int size=sizeof(buffer);
  gSocket->write(buffer,size);

  char bigbuffer[500];
  size = sizeof(bigbuffer);
  gSocket->read(bigbuffer,size);

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

  if(0)  std::cout << "get brb value: " << command << " " << buffer 
	    << " " << readback << " | " << value << std::endl;
  
  return value;


}

/*-- Event readout -------------------------------------------------*/
INT read_slow_control(char *pevent, INT off)
{


  bk_init32(pevent);

  float *pddata;
  char command[200];
  
  // Bank names must be exactly four char
  char bank_name[20];
  sprintf(bank_name,"BRV%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata);

  for(int j = 0; j < 8; j++){

    double resistor = 1.0;
    if(j==0){ resistor = 0.1; }
    if(j==2){ resistor =200; }
    if(j==5){ resistor =0.1; }
    if(j==6){ resistor =0.05; }
    if(j==7){ resistor =0.05;}


    struct timeval t1;  
    gettimeofday(&t1, NULL);

    // Read voltage
    sprintf(command,"custom_command ldo_get_voltage %i",j+1);
    double voltage = get_brb_value(command);
    sprintf(command,"custom_command ldo_get_power %i",j+1);
    double shunt_voltage = get_brb_value(command);
    
    float shunt_current = 1000.0*shunt_voltage/resistor;
    if(j==2){
      shunt_current = 1000.0*(shunt_voltage/resistor)/0.0004;
    }
    *pddata++ = shunt_current;
    
    *pddata++ = voltage;


  }
    

  bk_close(pevent, pddata);	
  
  // Get temperatures

  float *pddata2;
  
  sprintf(bank_name,"BRT%i",get_frontend_index());
  bk_create(pevent, bank_name, TID_FLOAT, (void**)&pddata2);

  std::cout << "Temp: ";
  for(int j = 1; j < 4; j++){

    // Read temperature
    sprintf(command,"custom_command get_temp %i",j);
    float temperature = get_brb_value(command);
    std::cout << temperature << " " ; 
    *pddata2++ = temperature;

  }
  std::cout << std::endl;

  bk_close(pevent, pddata2);	

  


  return bk_size(pevent);

}
 
/*-- Event readout -------------------------------------------------*/
INT read_pmt_status(char *pevent, INT off)
{

  bk_init32(pevent);

  return pmts->GetStatus(pevent, off);

}

  


 
