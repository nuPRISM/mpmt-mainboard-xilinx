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

  for(int j = 0; j < 4; j++){ // loop over 3 BRBS
    for(int i = 0; i < 20; i++){ // loop over 20 channels
    
      char name[100];
      char title[100];
      sprintf(name,"BRB_%i_%i",j,i);
      //TH1D *tmp2 = (TH1D*)gDirectory->Get(tname);
      //    if (tmp2){
      //std::cout << "Found " << tname << " and will delete !!! " << std::endl;
      // delete tmp2;
      //}
      std::cout << "Creating " << name << std::endl;
      sprintf(title,"BRB Waveform for BRB=%i channel=%i",j,i);	

      
      TH1D *tmp = new TH1D(name, title, 1024, -0.5*8, 1023.5*8);
      tmp->SetXTitle("ns");
      tmp->SetYTitle("ADC value");
      
      push_back(tmp);
    }
  }
  std::cout << "TBRBWaveform done init...... " << std::endl;
  FrequencySetting = -1;
}

uint64_t last_timestamp2 =0;
void TBRBWaveform::UpdateHistograms(TDataContainer& dataContainer){
  
  int eventid = dataContainer.GetMidasData().GetEventId();
  
  
  for(int j = 0; j < 4; j++){  // loop over mPMTs

    char name[100];
    sprintf(name,"BRB%i",j);
    
    TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>(name);
    
    if(dt743){      

      if(j==0){
	int timestamp = dt743->GetTimestamp();
	double tdiff = (double)(timestamp - last_timestamp2) * 8.0 / 1000000.0;
	std::cout << "Timestamp since last event: " << tdiff << "ms. "
		  << " difference from expected 10ms " << (tdiff - 10.0)*1000.0 << "us"
		  << std::endl;
	last_timestamp2 = timestamp;
      }
      
      std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();

      bool changeHistogram = false; 
      for(unsigned int i = 0; i < measurements.size(); i++){
	
	int chan = measurements[i].GetChannel();
	int nsamples = measurements[i].GetNSamples();

	if(0){if(i==0)	std::cout << "N samples " <<  nsamples << std::endl;}

	int ichan = j*20 + chan;

	if(chan < 20){
	  for(int ib = 0; ib < nsamples; ib++){
	    GetHistogram(ichan)->SetBinContent(ib+1, measurements[i].GetSample(ib));
	  }
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
  sprintf(tname,"BRB_Hit_0_%i",0);

  int fWFLength = 7; // Need a better way of detecting this...
  int numSamples = fWFLength / 1;
  
  // Otherwise make histograms
  clear();
  
  for(int j = 0; j < 4; j++){ // loop over 3 BRBS
    for(int i = 0; i < 20; i++){ // loop over 20 channels
    
      char name[100];
      char title[100];
      sprintf(name,"BRB_Hit_%i_%i",j,i);
      sprintf(title,"BRB Waveform for BRB=%i channel=%i (hit)",j,i);	

      TH1D *tmp = new TH1D(name, title, 1024, -0.5*8, 1023.5*8);
      tmp->SetXTitle("ns");
      tmp->SetYTitle("ADC value");
      
      push_back(tmp);
    }
  }
  std::cout << "TBRBWaveform  hit done init...... " << std::endl;
  FrequencySetting = -1;
}


void TBRBWaveformHit::UpdateHistograms(TDataContainer& dataContainer){
  
  int eventid = dataContainer.GetMidasData().GetEventId();
  int timestamp = dataContainer.GetMidasData().GetTimeStamp();

  for(int j = 0; j < 4; j++){  // loop over mPMTs

    char name[100];
    sprintf(name,"BRB%i",j);
    
    TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>(name);
    
    if(dt743){      
      
      //      std::cout << "updating for hit " << j << std::endl;
      
      std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();
      
      bool changeHistogram = false; 
      for(unsigned int i = 0; i < measurements.size(); i++){
	
	int chan = measurements[i].GetChannel();
	int nsamples = measurements[i].GetNSamples();

	int ichan = j*20 + chan;


	      
	// Check for hits in this waveform
	
	int baseline = (int)BSingleton::GetInstance()->GetBaseline(ichan);
	int threshold = baseline - 5;
	int threshold2 = baseline - 10;
	bool found_hit = false;
	int pulse_height = 0;
	int pulse_time = 0;
	for(int ib = 0; ib < nsamples; ib++){
	  int sample = measurements[i].GetSample(ib);
	  if(sample < threshold){ // found a pulse                                                                                
	    
	    int ph = baseline-sample;
	    if(ph > pulse_height) pulse_height = ph;
	    found_hit = true;
	    pulse_time = ib;
	  }
	}
	
	
	if(found_hit){
	  int nsamples = measurements[i].GetNSamples();
	  for(int ib = 0; ib < nsamples; ib++){
	    GetHistogram(ichan)->SetBinContent(ib+1, measurements[i].GetSample(ib));
	  }
	}      
      }
    }
  }
  
}


TBRBWaveformCorrupt::TBRBWaveformCorrupt(){

  SetSubTabName("BRB Waveforms Corrupt");
  
  CreateHistograms();
  FrequencySetting = -1;
}


void TBRBWaveformCorrupt::CreateHistograms(){


  // check if we already have histogramss.
  char tname[100];
  sprintf(tname,"BRB_Corrupt_%i",0);


  int fWFLength = 7; // Need a better way of detecting this...
  int numSamples = fWFLength / 1;
  
  // Otherwise make histograms
  clear();
  
  for(int i = 0; i < 20; i++){ // loop over 2 channels
    
    char name[100];
    char title[100];
    sprintf(name,"BRB_Corrupt_%i",i);
    sprintf(title,"BRB Waveform for channel=%i (corrupt)",i);	

    
    TH1D *tmp = new TH1D(name, title, 1024, -0.5*8, 1023.5*8);
    tmp->SetXTitle("ns");
    tmp->SetYTitle("ADC value");
    
    push_back(tmp);
  }
  std::cout << "TBRBWaveform corrupt done init...... " << std::endl;
  FrequencySetting = -1;
}


int total_checked=0;
int total_corrupt=0;

void TBRBWaveformCorrupt::UpdateHistograms(TDataContainer& dataContainer){
  
  int eventid = dataContainer.GetMidasData().GetEventId();
  int timestamp = dataContainer.GetMidasData().GetTimeStamp();
  int serno = dataContainer.GetMidasData().GetSerialNumber();
  TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>("BRB0");
  
  if(dt743){      
    
    
    std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();




    for(unsigned int i = 0; i < measurements.size(); i++){
      

      int chan = measurements[i].GetChannel();

      char title[100];
      sprintf(title,"BRB Waveform ch=%i (corrupt) evt=%i",chan,serno);	
      
      // Check for hits in this waveform
      
      int nsamples = 1000;
      int baseline = (int)BSingleton::GetInstance()->GetBaseline(chan);
      int threshold = baseline - 3;
      int threshold2 = baseline - 10;
      bool found_hit = false;
      int pulse_height = 0;
      int pulse_time = 0;
      for(int ib = 0; ib < nsamples; ib++){
	int sample = measurements[i].GetSample(ib);
	if(sample < threshold){ // found a pulse                                                                                

	  int ph = baseline-sample;
	  if(ph > pulse_height) pulse_height = ph;
	  found_hit = true;
	  pulse_time = ib;
	}
      }
      
      if(chan==0) total_checked++;

      
      if(found_hit){
	if(chan==0 && 0){ total_corrupt++;
	  std::cout << "Corrupt data = " << total_corrupt << " out of total " << total_checked 
		    << " ratio  = " << ((float)total_corrupt/(float)total_checked) << std::endl;
	}
	//	std::cout << "Pulse height: " << pulse_height << std::endl;
	if(chan == 10) std::cout << "ch 8 : Pulse hit: " << " " << pulse_height << " " 
				<< pulse_time << std::endl;
	int nsamples = measurements[i].GetNSamples();
	for(int ib = 0; ib < nsamples; ib++){
	  GetHistogram(chan)->SetBinContent(ib+1, measurements[i].GetSample(ib));
	}
	GetHistogram(chan)->SetTitle(title);
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
  sprintf(tname,"BRB_Baseline_0_%i",0);

  TH1D *tmp = (TH1D*)gDirectory->Get(tname);
  if (tmp) return;

  // Otherwise make histograms
  clear();


  for(int j = 0; j < 4; j++){ // loop over 3 BRBS
    for(int i = 0; i < 20; i++){ // loop over 20 channels
    
      char name[100];
      char title[100];
      sprintf(name,"BRB_Baseline_%i_%i",j,i);
      sprintf(title,"BRB Baseline for BRB=%i channel=%i (hit)",j,i);	
      
      TH1D *tmp = new TH1D(name, title, 1000, 1990, 2100);
      tmp->SetXTitle("Average Baseline");
      push_back(tmp);
    }
  }

  std::cout << "Baseline histo init complete... " << std::endl;
}


void TBRBBaseline::UpdateHistograms(TDataContainer& dataContainer){
  
  for(int j = 0; j < 4; j++){  // loop over mPMTs

    char name[100];
    sprintf(name,"BRB%i",j);
    
    TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>(name);
    
    if(dt743){      
      //      std::cout << "updating for baseline " << j << std::endl;

      std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();
      
      for(int i = 0; i < measurements.size(); i++){
	
	int chan = measurements[i].GetChannel();
	int nsamples = measurements[i].GetNSamples();	
	int ichan = j*20 + chan;

	// Use the first 30 samples for baseline
	double avg = 0.0;
	for(int ib = 0; ib < 30; ib++){ // <<<<--------
	  avg += measurements[i].GetSample(ib);
	}
	avg /= 30.0;
	
	GetHistogram(ichan)->Fill(avg);   
      }
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
  sprintf(tname,"BRB_PH_0_%i",0);

  TH1D *tmp = (TH1D*)gDirectory->Get(tname);
  if (tmp) return;

  // Otherwise make histograms
  clear();

  
  for(int j = 0; j < 4; j++){ // loop over 3 BRBS
    for(int i = 0; i < 20; i++){ // loop over 20 channels
      
      char name[100];
      char title[100];
      sprintf(name,"BRB_PH_%i_%i",j,i);

      sprintf(title,"BRB Pulse Height for BRB=%i, channel=%i",j,i);	
      
      TH1D *tmp= new TH1D(name, title, 120, 0.5, 120.5);
      tmp->SetXTitle("Pulse Height");
      push_back(tmp);
    }
  }
}


void TBRBPH::UpdateHistograms(TDataContainer& dataContainer){
  
  for(int j = 0; j < 4; j++){  // loop over mPMTs

    char name[100];
    sprintf(name,"BRB%i",j);
    
    TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>(name);
    
    if(dt743){      
      
      std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();
      for(int i = 0; i < measurements.size(); i++){
	
	int chan = measurements[i].GetChannel();
	int ichan = j*20 + chan;
	int nsamples = measurements[i].GetNSamples();
	int min_value = 4096;
	//      int max_value = 0;
	//      for(int j = 262; j < 280; j++){
	int min_bin = 0;
	for(int j = 0; j < 1000; j++){
	  if(measurements[i].GetSample(j) < min_value  & (j > 1 && j < 1000) ){
	    min_value = measurements[i].GetSample(j);
	    min_bin = j;
	  }
	}
	int baseline = (int)BSingleton::GetInstance()->GetBaseline(chan);
	int pulse_height = baseline - min_value;
	if(pulse_height > 3){
	  GetHistogram(ichan)->Fill(pulse_height);
	  if(chan == 10) std::cout << "Pulse height: " << chan << " " << pulse_height << " " << baseline << " " << min_value << " " << min_bin << std::endl;
	}
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
  sprintf(tname,"BRB_PH_Big_0_%i",0);

  TH1D *tmp = (TH1D*)gDirectory->Get(tname);
  if (tmp) return;

  // Otherwise make histograms
  clear();
  
  for(int j = 0; j < 4; j++){ // loop over 3 BRBS
    for(int i = 0; i < 20; i++){ // loop over 20 channels
      
      char name[100];
      char title[100];
      sprintf(name,"BRB_PH_Big_%i_%i",j,i);
    
      sprintf(title,"BRB Pulse Height (Big) for BRB=%i channel=%i",j,i);	

      TH1D *tmp= new TH1D(name, title, 600, 0.5, 2500.5);
      
      tmp->SetXTitle("Pulse Height");
      push_back(tmp);
    }
  }
}


void TBRBPHBig::UpdateHistograms(TDataContainer& dataContainer){
  
  for(int j = 0; j < 4; j++){  // loop over mPMTs

    char name[100];
    sprintf(name,"BRB%i",j);
    
    TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>(name);
    if(dt743){      
      
      std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();
      for(int i = 0; i < measurements.size(); i++){
	
	int chan = measurements[i].GetChannel();
	int ichan = j*20 + chan;
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
	  GetHistogram(ichan)->Fill(pulse_height);
	//	if(chan == 1) std::cout << "Pulse height: " << chan << " " << pulse_height << " " << baseline << " " << min_value << " " << min_bin << std::endl;
	}
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
 
  for(int j = 0; j < 4; j++){ // loop over 3 BRBS
    for(int i = 0; i < 20; i++){ // loop over 20 channels
      
      char name[100];
      char title[100];
      sprintf(name,"BRB_Time_%i_%i",j,i);

      sprintf(title,"BRB Pulse Time for BRB=%i channel=%i",j,i);	


      TH1D *tmp = new TH1D(name, title, 1024, -(0.5 + time_offset)*8, (1023.5-time_offset)*8);
      tmp->SetXTitle("Pulse Time (ns)");
      push_back(tmp);
    }
  }
}


void TBRB_Time::UpdateHistograms(TDataContainer& dataContainer){
  
 for(int j = 0; j < 4; j++){  // loop over mPMTs

    char name[100];
    sprintf(name,"BRB%i",j);
    
    TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>(name);
      
    if(dt743){      
      
      std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();
      for(int i = 0; i < measurements.size(); i++){
	
	int chan = measurements[i].GetChannel();
	int ichan = chan + j*20;
	int nsamples = measurements[i].GetNSamples();
	
	int baseline = (int)BSingleton::GetInstance()->GetBaseline(chan);
	int threshold = baseline - 8;
	
	bool in_pulse = false; // are we currently in a pulse?
	std::vector<int> pulse_times;
	for(int ib = 0; ib < nsamples; ib++){
	  int sample = measurements[i].GetSample(ib);
	  
	  if(sample < threshold && !in_pulse){ // found a pulse
	    in_pulse = true;
	    
	    int pulse_time = 8*(ib - time_offset);
	    pulse_times.push_back(pulse_time);
	  }
	  
	  if(sample >= threshold && in_pulse){ /// finished this pulse
	    in_pulse = false;
	  }
	  
	}
	
	for(int j = 0; j < pulse_times.size(); j++){
	  int pulse_time = pulse_times[j];
	  GetHistogram(ichan)->Fill(pulse_time);
	}
	
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









