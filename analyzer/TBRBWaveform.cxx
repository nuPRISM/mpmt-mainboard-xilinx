#include "TBRBWaveform.h"

#include "TBRBRawData.hxx"
#include "TDirectory.h"
#include "TBaselineSing.hxx"

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
      int baseline = (int)BSingleton::GetInstance()->GetBaseline(chan);
      for(int ib = 0; ib < nsamples; ib++){
	if(i != 1 || 1){
	  GetHistogram(chan)->SetBinContent(ib+1, measurements[i].GetSample(ib));
	}else{ // scale up channel 1
	  int sample = measurements[i].GetSample(ib);
	  sample = ((sample-baseline) * 60) + baseline; 
	  GetHistogram(chan)->SetBinContent(ib+1, sample);
	}
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


TBRBWaveformHit::TBRBWaveformHit(){

  SetSubTabName("BRB Waveforms Hit");
  
  CreateHistograms();
  FrequencySetting = -1;
}


void TBRBWaveformHit::CreateHistograms(){


  // check if we already have histogramss.
  char tname[100];
  sprintf(tname,"BRB_Hit_%i",0);


  int fWFLength = 7; // Need a better way of detecting this...
  int numSamples = fWFLength / 1;
  
  // Otherwise make histograms
  clear();
  
  for(int i = 0; i < 20; i++){ // loop over 2 channels
    
    char name[100];
    char title[100];
    sprintf(name,"BRB_Hit_%i",i);
    sprintf(title,"BRB Waveform for channel=%i (hit)",i);	

    
    TH1D *tmp = new TH1D(name, title, 1024, -0.5*8, 1023.5*8);
    tmp->SetXTitle("ns");
    tmp->SetYTitle("ADC value");
    
    push_back(tmp);
  }
  std::cout << "TBRBWaveform done init...... " << std::endl;
  FrequencySetting = -1;
}


void TBRBWaveformHit::UpdateHistograms(TDataContainer& dataContainer){
  
  int eventid = dataContainer.GetMidasData().GetEventId();
  int timestamp = dataContainer.GetMidasData().GetTimeStamp();
  
  TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>("BRB0");
  
  if(dt743){      
    
    
    std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();




    for(unsigned int i = 0; i < measurements.size(); i++){
      
      int chan = measurements[i].GetChannel();
      
      // Check for hits in this waveform
      
      int nsamples = 1000;
      int baseline = (int)BSingleton::GetInstance()->GetBaseline(chan);
      int threshold = baseline - 5;
      int threshold2 = baseline - 10;
      bool found_hit = false;
      int pulse_height = 0;
      for(int ib = 0; ib < nsamples; ib++){
	int sample = measurements[i].GetSample(ib);
	if(sample < threshold){ // found a pulse                                                                                

	  int ph = baseline-sample;
	  if(ph > pulse_height) pulse_height = ph;
	  found_hit = true;
	}
      }
      
      
      if(found_hit){
	//	std::cout << "Pulse height: " << pulse_height << std::endl;
	int nsamples = measurements[i].GetNSamples();
	for(int ib = 0; ib < nsamples; ib++){
	  GetHistogram(chan)->SetBinContent(ib+1, measurements[i].GetSample(ib));
	}
      }      
    }
    
  }
  
}



void TBRBWaveformHit::Reset(){
  
  
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
      for(int ib = 0; ib < 100; ib++){ // <<<<--------
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
    
    sprintf(title,"BRB Pulse Height for channel=%i",i);	

    TH1D *tmp= new TH1D(name, title, 50, 0.5, 50.5);
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
      int min_bin = 0;
      for(int j = 0; j < 1000; j++){
	if(measurements[i].GetSample(j) < min_value){
	  min_value = measurements[i].GetSample(j);
	  min_bin = j;
	}
      }
      int baseline = (int)BSingleton::GetInstance()->GetBaseline(chan);
      int pulse_height = baseline - min_value;
      if(pulse_height > 5){
	GetHistogram(chan)->Fill(pulse_height);
	//	if(chan == 1) std::cout << "Pulse height: " << chan << " " << pulse_height << " " << baseline << " " << min_value << " " << min_bin << std::endl;
      }
      
    }  
  }
  
}



/// Reset the histograms for this canvas
TBRBPHBig::TBRBPHBig(){
  SetSubTabName("BRB Pulse Height Big");  
  CreateHistograms();
}


void TBRBPHBig::CreateHistograms(){

  // check if we already have histogramss.
  char tname[100];
  sprintf(tname,"BRB_PH_Big_%i",0);

  TH1D *tmp = (TH1D*)gDirectory->Get(tname);
  if (tmp) return;

  // Otherwise make histograms
  clear();
  
  for(int i = 0; i < 20; i++){ // loop over 2 channels
    
    char name[100];
    char title[100];
    sprintf(name,"BRB_PH_Big_%i",i);
    
    sprintf(title,"BRB Pulse Height (Big) for channel=%i",i);	

    TH1D *tmp= new TH1D(name, title, 500, 0.5, 2000.5);

    tmp->SetXTitle("Pulse Height");
    push_back(tmp);
  }
}


void TBRBPHBig::UpdateHistograms(TDataContainer& dataContainer){
  
  TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>("BRB0");
  
  if(dt743){      
     
    std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();
    for(int i = 0; i < measurements.size(); i++){
      
      int chan = measurements[i].GetChannel();
      int nsamples = measurements[i].GetNSamples();
      int min_value = 4096;
      //      int max_value = 0;
      //      for(int j = 262; j < 280; j++){
      int min_bin = 0;
      for(int j = 0; j < 1000; j++){
	if(measurements[i].GetSample(j) < min_value){
	  min_value = measurements[i].GetSample(j);
	  min_bin = j;
	}
      }
      int baseline = (int)BSingleton::GetInstance()->GetBaseline(chan);
      int pulse_height = baseline - min_value;
      if(pulse_height > 5){
	GetHistogram(chan)->Fill(pulse_height);
	//	if(chan == 1) std::cout << "Pulse height: " << chan << " " << pulse_height << " " << baseline << " " << min_value << " " << min_bin << std::endl;
      }
      
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

      int baseline = (int)BSingleton::GetInstance()->GetBaseline(chan);
      int threshold = baseline - 8;

      bool in_pulse = false; // are we currently in a pulse?
      bool laser_pulse = true; // always save hits
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
	if(0)std::cout << "Afterpulse fraction: " << integral/((double)(total_events)) << " "
		  << integral << " " << total_events << std::endl;
      }  
    }
  }
  
}





/// Reset the histograms for this canvas
TBRB_Rates::TBRB_Rates(){
  SetSubTabName("BRB Dark Rates");  
  CreateHistograms();

}

void TBRB_Rates::CreateHistograms(){

  // check if we already have histogramss.
  char tname[100];
  sprintf(tname,"BRB_Rates_%i",0);

  TH1D *tmp = (TH1D*)gDirectory->Get(tname);
  if (tmp) return;

  // Otherwise make histograms
  clear();
  
  for(int i = 0; i < 20; i++){ // loop over 2 channels
    
    char name[100];
    char title[100];
    sprintf(name,"BRB_Rates_%i",i);
    
    sprintf(title,"PMT Dark Noise Rate for channel=%i",i);	


    TH1D *tmp = new TH1D(name, title, 400,0,2000);
    tmp->SetXTitle("Pulse Time (ns)");
    push_back(tmp);
  }
}




/// Reset the histograms for this canvas
TBRB_Rates_Single::TBRB_Rates_Single(){
  SetSubTabName("BRB Dark Rates");  
  CreateHistograms();

}

void TBRB_Rates_Single::CreateHistograms(){

  // check if we already have histogramss.
  char tname[100];
  sprintf(tname,"BRB_Rates_Single_%i",0);

  TH1D *tmp = (TH1D*)gDirectory->Get(tname);
  if (tmp) return;

  // Otherwise make histograms
  clear();
  
  for(int i = 0; i < 20; i++){ // loop over 2 channels
    
    char name[100];
    char title[100];
    sprintf(name,"BRB_Rates_Single_%i",i);
    
    sprintf(title,"PMT Dark Noise Rate (uncorrelated) for channel=%i",i);	


    TH1D *tmp = new TH1D(name, title, 400,0,2000);
    tmp->SetXTitle("Pulse Time (ns)");
    push_back(tmp);
  }
}









