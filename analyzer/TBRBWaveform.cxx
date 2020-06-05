#include "TBRBWaveform.h"

#include "TBRBRawData.hxx"
#include "TDirectory.h"


/// Reset the histograms for this canvas
TBRBWaveform::TBRBWaveform(){

  SetSubTabName("BRB Waveforms");
  
  CreateHistograms();
  FrequencySetting = -1;
}


void TBRBWaveform::CreateHistograms(){

  std::cout << "Creating waveforms!!! " << std::endl;

  // check if we already have histogramss.
  char tname[100];
  sprintf(tname,"BRB_%i",0);


  int fWFLength = 7; // Need a better way of detecting this...
  int numSamples = fWFLength / 1;
  
  // Otherwise make histograms
  clear();
  
  for(int i = 0; i < 20; i++){ // loop over 2 channels
    
    char name[100];
    char title[100];
    sprintf(name,"BRB_%i",i);
    //TH1D *tmp2 = (TH1D*)gDirectory->Get(tname);
    //    if (tmp2){
    //std::cout << "Found " << tname << " and will delete !!! " << std::endl;
    // delete tmp2;
    //}
    std::cout << "Creating " << name << std::endl;
    sprintf(title,"BRB Waveform for channel=%i",i);	

    
    TH1D *tmp = new TH1D(name, title, 1024, -0.5*8, 1023.5*8);
    tmp->SetXTitle("ns");
    tmp->SetYTitle("ADC value");
    
    push_back(tmp);
  }
  std::cout << "TBRBWaveform done init...... " << std::endl;
  FrequencySetting = -1;
}


void TBRBWaveform::UpdateHistograms(TDataContainer& dataContainer){
  
  int eventid = dataContainer.GetMidasData().GetEventId();
  int timestamp = dataContainer.GetMidasData().GetTimeStamp();
  
  TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>("BRB0");
  
  if(dt743){      
    
    
    std::vector<RawChannelMeasurement> measurements = dt743->GetMeasurements();

    bool changeHistogram = false; 
    for(int i = 0; i < measurements.size(); i++){

      std::cout << "Measurement: " << i << std::endl;
      int chan = measurements[i].GetChannel();
      int nsamples = measurements[i].GetNSamples();
      std::cout << "Measurement: " << i << " " << chan << std::endl;
      //std::cout << "Nsamples " <<  measurements[i].GetNSamples() << std::endl;
      for(int ib = 0; ib < nsamples; ib++){
	GetHistogram(chan)->SetBinContent(ib+1, measurements[i].GetSample(ib));
      }
      
    }
  }
  
}



void TBRBWaveform::Reset(){
  
  
  for(int i = 0; i < 8; i++){ // loop over channels
    int index =  i;
    
    // Reset the histogram...
    for(int ib = 0; ib < GetHistogram(index)->GetNbinsX(); ib++) {
      GetHistogram(index)->SetBinContent(ib, 0);
    }
    
    GetHistogram(index)->Reset();
    
  }
}





/// Reset the histograms for this canvas
TBRBBaseline::TBRBBaseline(){
  SetSubTabName("BRB Baseline");  
  CreateHistograms();
}


void TBRBBaseline::CreateHistograms(){

  // check if we already have histogramss.
  char tname[100];
  sprintf(tname,"BRB_Baseline_%i",0);

  TH1D *tmp = (TH1D*)gDirectory->Get(tname);
  if (tmp) return;

  // Otherwise make histograms
  clear();
  
  for(int i = 0; i < 20; i++){ // loop over 2 channels
    
    char name[100];
    char title[100];
    sprintf(name,"BRB_Baseline_%i",i);
    
    sprintf(title,"BRB Baseline for channel=%i",i);	
    
    TH1D *tmp = new TH1D(name, title, 1000, 1990, 2070);
    tmp->SetXTitle("Average Baseline");
    push_back(tmp);
  }
}


void TBRBBaseline::UpdateHistograms(TDataContainer& dataContainer){
  
  TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>("BRB0");
  
  if(dt743){      
     
    std::vector<RawChannelMeasurement> measurements = dt743->GetMeasurements();

     for(int i = 0; i < measurements.size(); i++){
           
      int chan = measurements[i].GetChannel();
      int nsamples = measurements[i].GetNSamples();

      // Use the first 100 samples for baseline
      double avg = 0.0;
      for(int ib = 0; ib < 100; ib++){
	avg += measurements[i].GetSample(ib);
      }
      avg /= 100.0;
      
      GetHistogram(chan)->Fill(avg);   
    }
  }
  
}




/// Reset the histograms for this canvas
TBRBRMS::TBRBRMS(){
  SetSubTabName("BRB RMS");  
  CreateHistograms();
}


void TBRBRMS::CreateHistograms(){

  // check if we already have histogramss.
  char tname[100];
  sprintf(tname,"BRB_RMS_%i",0);

  TH1D *tmp = (TH1D*)gDirectory->Get(tname);
  if (tmp) return;

  // Otherwise make histograms
  clear();
  
  for(int i = 0; i < 20; i++){ // loop over 2 channels
    
    char name[100];
    char title[100];
    sprintf(name,"BRB_RMS_%i",i);
    
    sprintf(title,"BRB RMS for channel=%i",i);	
    
    TH1D *tmp = new TH1D(name, title, 200, 0, 1);
    tmp->SetXTitle("Average RMS");
    push_back(tmp);
  }
}


void TBRBRMS::UpdateHistograms(TDataContainer& dataContainer){
  
  TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>("BRB0");
  
  if(dt743){      
     
    std::vector<RawChannelMeasurement> measurements = dt743->GetMeasurements();

     for(int i = 0; i < measurements.size(); i++){
           
      int chan = measurements[i].GetChannel();
      int nsamples = measurements[i].GetNSamples();

      /*
      // Use the first 100 samples for RMS
      double sum = 0;
      for(int ib = 0; ib < 100; ib++){
        sum += pow(measurements[i].GetSample(ib),2);
      }

      avg /= 100.0;


      for (int i = 0; i < n; i++)
	sum += pow(x[i], 2);

      return sqrt(sum / n);
      */
      float sum = 0.0, mean, SD = 0.0;
      for (int ib = 0; ib < 100; ++ib) {
        sum += measurements[i].GetSample(ib);
      }
      mean = sum / 100;

      for (int ib = 0; ib < 100; ++ib)
        SD += pow(measurements[i].GetSample(ib) - mean, 2);
      GetHistogram(chan)->Fill(sqrt(SD / 100));
	//      GetHistogram(chan)->Fill(avg);   
    }
  }
  
}




