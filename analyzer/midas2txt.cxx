// Example Program for converting MIDAS format to ROOT format.
//
// T. Lindner (Jan 2016) 
//
// Example is for the CAEN V792 ADC module

#include <stdio.h>
#include <iostream>
#include <time.h>
#include <vector>

#include "TRootanaEventLoop.hxx"
#include "TFile.h"
#include "TTree.h"

#include "TAnaManager.hxx"

#ifdef USE_V792
#include "TV792Data.hxx"
#endif
#include "TBRBRawData.hxx"
#include <fstream>
#include <ostream>

class Analyzer: public TRootanaEventLoop {

public:

  std::ofstream myfile; // output text file

  Analyzer() {
    
    printf("Constructor\n");
    UseBatchMode();
    DisableRootOutput();
  };

  virtual ~Analyzer() {};

  void Initialize(){

    int runno = GetCurrentRunNumber();
    printf("Run number : %i\n",runno);
    char filename[100];
    sprintf(filename,"mpmt_data_run%i.txt",runno);
    myfile.open(filename);

  }
  
  
  void BeginRun(int transition,int run,int time){
    

  }   


  void EndRun(int transition,int run,int time){
        printf("\n");
	
	myfile.close();
  }

  
  
  // Main work here; create ttree events for every sequenced event in 
  // Lecroy data packets.
  bool ProcessMidasEvent(TDataContainer& dataContainer){

    int serialnumber = dataContainer.GetMidasEvent().GetSerialNumber();
    if(serialnumber%1000 == 0) printf(".");


    TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>("BRB0");

    if(dt743){



      std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();
      
      bool changeHistogram = false;
      for(unsigned int i = 0; i < measurements.size(); i++){

	int chan = measurements[i].GetChannel();
	if(!(i==1 || i==18)) continue; // just look at channel 1 (PMT) and 18 (injected)
	
	myfile << i << ", ";

	int nsamples = measurements[i].GetNSamples();
	for(int ib = 0; ib < nsamples; ib++){

	  // only save samples between 200 and 300, since that's where pulse is
	  if(ib >= 200 && ib < 300){
	    int sample = measurements[i].GetSample(ib);
	    

	    myfile << sample << ", " ;
	  }
	}
	myfile << "\n";
      }
    }


    return true;

  };
  


}; 


int main(int argc, char *argv[])
{

  Analyzer::CreateSingleton<Analyzer>();
  return Analyzer::Get().ExecuteLoop(argc, argv);

}

