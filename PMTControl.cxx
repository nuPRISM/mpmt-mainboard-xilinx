
#include "PMTControl.h"

PMTControl::PMTControl(KOsocket *socket){

  // Save connection to socket.
  //  fSocket = socket;

  //  pmt_settings;

  midas::odb::set_debug(true); 

  //Get ODB values (new C++ ODB!)                                                                                                                             
  midas::odb pmts = {
    {"HVset", std::array<double, 20>{} }
  };

  pmts.connect("/Equipment/BRB/Settings/PMTS");

  // Setup the DB watch for the PMT settings
  pmt_watch.connect("/Equipment/BRB/Settings/PMTS");
  pmt_watch.watch([](midas::odb &o) {
      std::cout << "Value of key \"" + o.get_full_path() + "\" changed to " << o << std::endl;
    });


}
