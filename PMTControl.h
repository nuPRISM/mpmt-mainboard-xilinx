#ifndef PMTControl_hxx_seen
#define PMTControl_hxx_seen

#include "KOsocket.h"
#include "odbxx.hxx"
#include "midas.h" 

class PMTControl {

 public:
 
  PMTControl(KOsocket *socket); 


  int GetStatus(char *pevent, INT off);  

 private:

  // Setup the callback for PMT Settings directory
  midas::odb pmt_watch; 

  // Pointer to socket to BRB
  KOsocket *fSocket;
  

};

#endif
