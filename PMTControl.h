#ifndef PMTControl_hxx_seen
#define PMTControl_hxx_seen

#include "KOsocket.h"
#include "odbxx.h"
#include "midas.h" 

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

  // Read PMT value
  float ReadValue(std::string command,int chan);

  // Read modbus PMT value
  float ReadModbusValue(std::string command,int chan);

  bool SetCommand(std::string command, int value, int ch = -1);

  // Setup the callback for PMT Settings directory
  midas::odb pmt_watch; 

  // Pointer to socket to BRB
  KOsocket *fSocket;

  int get_frontend_index(){return fe_index;};
  int fe_index;

};

#endif
