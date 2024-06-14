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

const int max_samples = 1024;

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

  double BRB_waveform[20][max_samples];
  double BRB_waveform_ch0[max_samples];
  double BRB_waveform_ch1[max_samples];
  double BRB_waveform_ch2[max_samples];
  double BRB_waveform_ch3[max_samples];
  double BRB_waveform_ch4[max_samples];
  double BRB_waveform_ch5[max_samples];
  double BRB_waveform_ch6[max_samples];
  double BRB_waveform_ch7[max_samples];
  double BRB_waveform_ch8[max_samples];
  double BRB_waveform_ch9[max_samples];
  double BRB_waveform_ch10[max_samples];
  double BRB_waveform_ch11[max_samples];
  double BRB_waveform_ch12[max_samples];
  double BRB_waveform_ch13[max_samples];
  double BRB_waveform_ch14[max_samples];
  double BRB_waveform_ch15[max_samples];
  double BRB_waveform_ch16[max_samples];
  double BRB_waveform_ch17[max_samples];
  double BRB_waveform_ch18[max_samples];
  double BRB_waveform_ch19[max_samples];
  

  Analyzer() {

  };

  virtual ~Analyzer() {};

  void Initialize(){


  }
  
  
  void BeginRun(int transition,int run,int time){
    
    // Create a TTree
    fTree = new TTree("midas_data","MIDAS data");

    char name[100], descr[100];
    sprintf(name,"BRB_waveform");
    sprintf(descr,"BRB_waveform[20][%i]/Double_t",max_samples);
    fTree->Branch(name,&BRB_waveform,descr); 

    
    for(int i = 0; i < 20; i++){
      sprintf(descr,"BRB_waveform_ch%i[%i]/Double_t",i,max_samples);
      sprintf(name,"BRB_waveform_ch%i",i);

      if(i==0)fTree->Branch(name,&BRB_waveform_ch0,descr); 
      if(i==1)fTree->Branch(name,&BRB_waveform_ch1,descr); 
      if(i==2)fTree->Branch(name,&BRB_waveform_ch2,descr); 
      if(i==3)fTree->Branch(name,&BRB_waveform_ch3,descr); 
      if(i==4)fTree->Branch(name,&BRB_waveform_ch4,descr); 
      if(i==5)fTree->Branch(name,&BRB_waveform_ch5,descr); 
      if(i==6)fTree->Branch(name,&BRB_waveform_ch6,descr); 
      if(i==7)fTree->Branch(name,&BRB_waveform_ch7,descr); 
      if(i==8)fTree->Branch(name,&BRB_waveform_ch8,descr); 
      if(i==9)fTree->Branch(name,&BRB_waveform_ch9,descr); 
      if(i==10)fTree->Branch(name,&BRB_waveform_ch10,descr); 
      if(i==11)fTree->Branch(name,&BRB_waveform_ch11,descr); 
      if(i==12)fTree->Branch(name,&BRB_waveform_ch12,descr); 
      if(i==13)fTree->Branch(name,&BRB_waveform_ch13,descr); 
      if(i==14)fTree->Branch(name,&BRB_waveform_ch14,descr); 
      if(i==15)fTree->Branch(name,&BRB_waveform_ch15,descr); 
      if(i==16)fTree->Branch(name,&BRB_waveform_ch16,descr); 
      if(i==17)fTree->Branch(name,&BRB_waveform_ch17,descr); 
      if(i==18)fTree->Branch(name,&BRB_waveform_ch18,descr); 
      if(i==19)fTree->Branch(name,&BRB_waveform_ch19,descr); 

    }

  

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
	coarsecounter = brbdata->GetTimestamp();;

	brbno = j;
	std::cout << "Data from " << brbno << std::endl;
	
	std::vector<RawBRBMeasurement> measurements = brbdata->GetMeasurements();
	
	bool changeHistogram = false; 
	for(unsigned int i = 0; i < measurements.size(); i++){
	  
	  int chan = measurements[i].GetChannel();
	  int nsamples = measurements[i].GetNSamples();
	  
	  if(1){if(i==0)	std::cout << "N samples " <<  nsamples << std::endl;}

	  if(chan < 0 || chan >= 20)continue;
	  
	  for(int ib = 0; ib < nsamples; ib++){
	    BRB_waveform[chan][ib] = measurements[i].GetSample(ib);
	    if(chan == 0){BRB_waveform_ch0[ib] = measurements[i].GetSample(ib);}
	    if(chan == 1){BRB_waveform_ch1[ib] = measurements[i].GetSample(ib);}
	    if(chan == 2){BRB_waveform_ch2[ib] = measurements[i].GetSample(ib);}
	    if(chan == 3){BRB_waveform_ch3[ib] = measurements[i].GetSample(ib);}
	    if(chan == 4){BRB_waveform_ch4[ib] = measurements[i].GetSample(ib);}
	    if(chan == 5){BRB_waveform_ch5[ib] = measurements[i].GetSample(ib);}
	    if(chan == 6){BRB_waveform_ch6[ib] = measurements[i].GetSample(ib);}
	    if(chan == 7){BRB_waveform_ch7[ib] = measurements[i].GetSample(ib);}
	    if(chan == 8){BRB_waveform_ch8[ib] = measurements[i].GetSample(ib);}
	    if(chan == 9){BRB_waveform_ch9[ib] = measurements[i].GetSample(ib);}
	    if(chan == 10){BRB_waveform_ch10[ib] = measurements[i].GetSample(ib);}
	    if(chan == 11){BRB_waveform_ch11[ib] = measurements[i].GetSample(ib);}
	    if(chan == 12){BRB_waveform_ch12[ib] = measurements[i].GetSample(ib);}
	    if(chan == 13){BRB_waveform_ch13[ib] = measurements[i].GetSample(ib);}
	    if(chan == 14){BRB_waveform_ch14[ib] = measurements[i].GetSample(ib);}
	    if(chan == 15){BRB_waveform_ch15[ib] = measurements[i].GetSample(ib);}
	    if(chan == 16){BRB_waveform_ch16[ib] = measurements[i].GetSample(ib);}
	    if(chan == 17){BRB_waveform_ch17[ib] = measurements[i].GetSample(ib);}
	    if(chan == 18){BRB_waveform_ch18[ib] = measurements[i].GetSample(ib);}
	    if(chan == 19){BRB_waveform_ch19[ib] = measurements[i].GetSample(ib);}
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

