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
    
    
    std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();

    bool changeHistogram = false; 
    for(unsigned int i = 0; i < measurements.size(); i++){

      int chan = measurements[i].GetChannel();
      int nsamples = measurements[i].GetNSamples();
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
     
    std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();

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
     
    std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();

     for(int i = 0; i < measurements.size(); i++){
           
      int chan = measurements[i].GetChannel();
      int nsamples = measurements[i].GetNSamples();

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







/// Reset the histograms for this canvas
TBRBPH::TBRBPH(){
  SetSubTabName("BRB Pulse Height");  
  CreateHistograms();
}


void TBRBPH::CreateHistograms(){

  // check if we already have histogramss.
  char tname[100];
  sprintf(tname,"BRB_PH_%i",0);

  TH1D *tmp = (TH1D*)gDirectory->Get(tname);
  if (tmp) return;

  // Otherwise make histograms
  clear();
  
  for(int i = 0; i < 20; i++){ // loop over 2 channels
    
    char name[100];
    char title[100];
    sprintf(name,"BRB_PH_%i",i);
    
    sprintf(title,"BRB Pulse Heigh for channel=%i",i);	

    TH1D *tmp = new TH1D(name, title, 160, -50, 270);
    tmp->SetXTitle("Pulse Height");
    push_back(tmp);
  }
}


void TBRBPH::UpdateHistograms(TDataContainer& dataContainer){
  
  TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>("BRB0");
  
  if(dt743){      
     
    std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();
    for(int i = 0; i < measurements.size(); i++){
      
      int chan = measurements[i].GetChannel();
      int nsamples = measurements[i].GetNSamples();
      int min_value = 4096;
      //      int max_value = 0;
      //      for(int j = 262; j < 280; j++){
      for(int j = 0; j < 512; j++){
	if(measurements[i].GetSample(j) < min_value){
	  min_value = measurements[i].GetSample(j);
	}
      }
      int baseline = 2045;
      if(chan == 1) baseline = 2054;
      if(chan == 2) baseline = 2050;
      if(chan == 3) baseline = 2051;
      if(chan == 4) baseline = 2028;
      if(chan == 6) baseline = 2046;

      int pulse_height = baseline - min_value;
      //std::cout << "Pulse height: " << chan << " " << pulse_height << std::endl;
      if(pulse_height > 1 || pulse_height < -1)
	GetHistogram(chan)->Fill(pulse_height);
      
    }  
  }
  
}




/// Reset the histograms for this canvas
TBRB_Time::TBRB_Time(){
  SetSubTabName("BRB Pulse Times");  
  CreateHistograms();

  nhits = std::vector<int>(20,0);
  total_events = 0;


}

const double time_offset = 270;
void TBRB_Time::CreateHistograms(){

  // check if we already have histogramss.
  char tname[100];
  sprintf(tname,"BRB_Time_%i",0);

  TH1D *tmp = (TH1D*)gDirectory->Get(tname);
  if (tmp) return;

  // Otherwise make histograms
  clear();
  
  for(int i = 0; i < 20; i++){ // loop over 2 channels
    
    char name[100];
    char title[100];
    sprintf(name,"BRB_Time_%i",i);
    
    sprintf(title,"BRB Pulse Time for channel=%i",i);	


    TH1D *tmp = new TH1D(name, title, 1024, -(0.5 + time_offset)*8, (1023.5-time_offset)*8);
    tmp->SetXTitle("Pulse Time (ns)");
    push_back(tmp);
  }
}


void TBRB_Time::UpdateHistograms(TDataContainer& dataContainer){
  
  TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>("BRB0");
  
  if(dt743){      
     
    std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();
    for(int i = 0; i < measurements.size(); i++){
      
      int chan = measurements[i].GetChannel();
      int nsamples = measurements[i].GetNSamples();

      int baseline = 2045;
      if(chan == 1) baseline = 2054;
      if(chan == 2) baseline = 2050;
      if(chan == 3) baseline = 2051;
      if(chan == 4) baseline = 2028;
      if(chan == 6) baseline = 2046;
      int threshold = baseline - 12;

      bool in_pulse = false; // are we currently in a pulse?
      bool laser_pulse = false;
      std::vector<int> pulse_times;
      for(int ib = 0; ib < nsamples; ib++){
	int sample = measurements[i].GetSample(ib);

	if(sample < threshold && !in_pulse){ // found a pulse
	  in_pulse = true;
	  
	  int pulse_time = 8*(ib - time_offset);
	  pulse_times.push_back(pulse_time);
	  if(ib > 262 and ib < 280){ // found a laser pulse
	    total_events += 1;
	    laser_pulse = true;
	  }
	}

	if(sample >= threshold && in_pulse){ /// finished this pulse
	  in_pulse = false;
	}

      }

      if(laser_pulse){
	for(int j = 0; j < pulse_times.size(); j++){
	  int pulse_time = pulse_times[j];
	  GetHistogram(chan)->Fill(pulse_time);
	}
      }

      int serial = dataContainer.GetMidasData().GetSerialNumber();
      if(chan == 0 and serial%2000 == 0){
	double integral = GetHistogram(chan)->Integral(290,1000);
	std::cout << "Afterpulse fraction: " << integral/((double)(total_events)) << " "
		  << integral << " " << total_events << std::endl;
      }  
    }
  }
  
}







