// Example Program for converting MIDAS format to ROOT format.
//
// for mPMT data

#include <stdio.h>
#include <iostream>
#include <time.h>
#include <vector>

#include "TRootanaEventLoop.hxx"
#include "TFile.h"
#include "TTree.h"

#include "TAnaManager.hxx"

#include "TBRBRawData.hxx"
#include <fstream>

const int max_samples = 0x8000;

class Analyzer: public TRootanaEventLoop {

  
public:

  // An analysis manager.  Define and fill histograms in 
  // analysis manager.
  TAnaManager *anaManager;

  // The tree to fill.
  TTree *fTree;

  int timestamp;
  int serialnumber;
  int coarsecounter;
  int brbno; // which BRB is data from?

  double BRB0_waveform[max_samples];
  

  Analyzer() {

  };

  virtual ~Analyzer() {};

  void Initialize(){


  }
  
  
  void BeginRun(int transition,int run,int time){
    
    // Create a TTree
    fTree = new TTree("midas_data","MIDAS data");

    char name[100], descr[100];
    sprintf(name,"BRB_waveform%i",0);
    sprintf(descr,"BRB_waveform%i[%i]/Double_t",0,max_samples);
    fTree->Branch(name,&BRB0_waveform,descr); 

    

    fTree->Branch("timestamp",&timestamp,"timestamp/I");
    fTree->Branch("serialnumber",&serialnumber,"serialnumber/I");
    fTree->Branch("coarsecounter",&coarsecounter,"coarsecounter/I");
    fTree->Branch("brbno",&brbno,"brbno/I");

  }   


  void EndRun(int transition,int run,int time){
        printf("\n");
  }

  
  
  // Main work here; create ttree events for every sequenced event in 
  // Lecroy data packets.
  bool ProcessMidasEvent(TDataContainer& dataContainer){

    serialnumber = dataContainer.GetMidasEvent().GetSerialNumber();
    if(serialnumber%10 == 0) printf(".");
    timestamp = dataContainer.GetMidasEvent().GetTimeStamp();
    
    for(int j = 0; j < 4; j++){  // loop over mPMTs
      
      char name[100];
      sprintf(name,"BRB%i",j);
      
      TBRBRawData *brbdata = dataContainer.GetEventData<TBRBRawData>(name);
      
      if(brbdata){      
	coarsecounter = 0;

	brbno = j;
	std::cout << "Data from " << brbno << std::endl;
	
	std::vector<RawBRBMeasurement> measurements = brbdata->GetMeasurements();
	
	bool changeHistogram = false; 
	for(unsigned int i = 0; i < measurements.size(); i++){
	  
	  int chan = measurements[i].GetChannel();
	  int nsamples = measurements[i].GetNSamples();
	  
	  if(0){if(i==0)	std::cout << "N samples " <<  nsamples << std::endl;}
	  
	  int ichan = j*20 + chan;
	  
	  for(int ib = 0; ib < nsamples; ib++){
	    BRB0_waveform[ib] = measurements[i].GetSample(ib);
	  }

	  

	  
	}
	
      }
    }
    fTree->Fill();
      
    return true;

  }
  
  // Complicated method to set correct filename when dealing with subruns.
  std::string SetFullOutputFileName(int run, std::string midasFilename)
  {
    char buff[128]; 
    Int_t in_num = 0, part = 0;
    Int_t num[2] = { 0, 0 }; // run and subrun values
    // get run/subrun numbers from file name
    for (int i=0; ; ++i) {
      char ch = midasFilename[i];
        if (!ch) break;
        if (ch == '/') {
          // skip numbers in the directory name
          num[0] = num[1] = in_num = part = 0;
        } else if (ch >= '0' && ch <= '9' && part < 2) {
          num[part] = num[part] * 10 + (ch - '0');
          in_num = 1;
        } else if (in_num) {
          in_num = 0;
          ++part;
        }
    }
    if (part == 2) {
      if (run != num[0]) {
        std::cerr << "File name run number (" << num[0]
                  << ") disagrees with MIDAS run (" << run << ")" << std::endl;
        exit(1);
      }
      sprintf(buff,"output_%.6d_%.4d.root", run, num[1]);
      printf("Using filename %s\n",buff);
    } else {
      sprintf(buff,"output_%.6d.root", run);
    }
    return std::string(buff);
  };





}; 


int main(int argc, char *argv[])
{

  Analyzer::CreateSingleton<Analyzer>();
  return Analyzer::Get().ExecuteLoop(argc, argv);

}

