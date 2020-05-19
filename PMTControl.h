#ifndef PMTControl_hxx_seen
#define PMTControl_hxx_seen

#include "KOsocket.h"
#include "odbxx.hxx"

class PMTControl {

 public:
 
  PMTControl(KOsocket *socket); 

 private:

  // Setup the callback for PMT Settings directory
  midas::odb pmt_watch; 

  // Pointer to socket to BRB
  KOsocket *fSocket;
  

};

#endif
