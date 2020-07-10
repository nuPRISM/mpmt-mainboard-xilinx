#ifndef PMTControl_hxx_seen
#define PMTControl_hxx_seen

#include "KOsocket.h"
#include "odbxx.h"
#include "midas.h" 

class PMTControl {

 public:
 
  PMTControl(KOsocket *socket); 


  int GetStatus(char *pevent, INT off);  

  // callback function
  void callback(midas::odb &o);

 private:

  // Read PMT value
  float ReadValue(std::string command,int chan);

  bool SetCommand(std::string command, int value);

  // Setup the callback for PMT Settings directory
  midas::odb pmt_watch; 

  // Pointer to socket to BRB
  KOsocket *fSocket;
  

};

#endif
