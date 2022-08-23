#ifndef PMTControl_hxx_seen
#define PMTControl_hxx_seen

#include "KOsocket.h"
#include "odbxx.h"
#include "midas.h" 
#include <array>
#include <unistd.h>

class PMTControl {

 public:
 
  PMTControl(KOsocket *socket, int index); 


  int GetStatus(char *pevent, INT off);  

  // callback function
  void callback(midas::odb &o);

  // Check which PMTs are plugged in and responding
  int CheckActivePMTs();

 private:
  
  std::vector<bool> fActivePMTs;

  // Read modbus PMT value
  float ReadModbusValue(std::string command,int chan);

  bool SetCommand(std::string command, float value, int ch = -1);

  bool SetDefaults();

  // This function returns the conversion factor for different modbus registers
  float GetModbusFactor(std::string command);

  // Setup the callback for PMT Settings directory
  midas::odb pmt_watch; 

  // Pointer to socket to BRB
  KOsocket *fSocket;

  int get_frontend_index(){return fe_index;};
  int fe_index;

  // Let's read these variables once, then maybe cache them.
  std::vector<float> ramp_rate_up;
  std::vector<float> ramp_rate_down;
  std::vector<float> trip_time;
  std::vector<float> trip_threshold;

  // Keep track of whether this is first readout event.  Some variables we only read once
  bool fFirstEvent;

};

#endif
