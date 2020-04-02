#include "TBRBWaveform.h"

#include "TBRBRawData.hxx"
#include "TDirectory.h"


/// Reset the histograms for this canvas
TBRBWaveform::TBRBWaveform(){

  SetSubTabName("BRB Waveforms");
  //SetUpdateOnlyWhenPlotted(true);
  
  CreateHistograms();
  FrequencySetting = -1;
}


void TBRBWaveform::CreateHistograms(){

  // check if we already have histogramss.
  char tname[100];
  sprintf(tname,"BRB_%i",0);

  TH1D *tmp = (TH1D*)gDirectory->Get(tname);
  if (tmp) return;

  int fWFLength = 7; // Need a better way of detecting this...
  int numSamples = fWFLength / 1;
  
  // Otherwise make histograms
  clear();
  
  for(int i = 0; i < 20; i++){ // loop over 2 channels
    
    char name[100];
    char title[100];
    sprintf(name,"BRB_%i",i);
    
    sprintf(title,"BRB Waveform for channel=%i",i);	
    
    TH1D *tmp = new TH1D(name, title, 512, -0.5, 511.5);
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
           
      int chan = measurements[i].GetChannel();
      int nsamples = measurements[i].GetNSamples();

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
