
#include "PMTControl.h"

PMTControl::PMTControl(KOsocket *socket){

  // Save connection to socket.
  fSocket = socket;

  midas::odb::set_debug(true); 
  
  //Get ODB values (new C++ ODB!)                                                                                                                             
  midas::odb pmts = {
    {"HVmax", std::array<double, 20>{} },
    {"HVenable", std::array<bool, 20>{} },
    {"HVset", std::array<double, 20>{} }

  };

  std::cout << "PMT connect" << std::endl;
  pmts.connect("/Equipment/BRB/Settings/PMTS");
  std::cout << "PMT enable : " << pmts["HVenable"][4] << std::endl;
  std::cout << "PMT max : " << pmts["HVmax"][4] << std::endl;

  // Setup the DB watch for the PMT settings
  pmt_watch.connect("/Equipment/BRB/Settings/PMTS");
  pmt_watch.watch([](midas::odb &o) {
      if(o.get_full_path().find("HVset") != std::string::npos){
	std::cout << "HV set points changed:  \"" + o.get_full_path() + "\" changed to " << o << std::endl;
	// Change the HV set point
      }else if(o.get_full_path().find("HVenable") != std::string::npos){
	std::cout << "HV enable changed:  \"" + o.get_full_path() + "\" changed to " << o << std::endl;
	// Change the enable and HV set point
      };

    });

  // Need to add functionality to check if board was turned off... if board was turned off then find that
  // the enable don't match the board, then enables should be turned off in ODB...


}




int PMTControl::GetStatus(char *pevent, INT off)
{


  float *pddata;

  bk_create(pevent, "PMT9", TID_FLOAT, (void**)&pddata);
  *pddata++ = 0.0;
  *pddata++ = 1.0;
  *pddata++ = -1.0;
  *pddata++ = 2.0;

  bk_close(pevent, pddata);

  return bk_size(pevent);
}
