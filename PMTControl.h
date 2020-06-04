#ifndef PMTControl_hxx_seen
#define PMTControl_hxx_seen

#include "KOsocket.h"
#include "odbxx.h"
#include "midas.h" 

class PMTControl {

 public:
 
  PMTControl(KOsocket *socket); 


  int GetStatus(char *pevent, INT off);  

 private:

  // Read PMT value
  float ReadValue(std::string command,int chan);

  // Setup the callback for PMT Settings directory
  midas::odb pmt_watch; 

  // Pointer to socket to BRB
  KOsocket *fSocket;
  

};

#endif
