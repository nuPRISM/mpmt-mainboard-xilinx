#include "TAnaManager.hxx"
#include "TV1720RawData.h"
#include "TRB3Decoder.hxx"
#include "TBRBRawData.hxx"

#include "odbxx.h"

TAnaManager::TAnaManager(bool isoffline){

  AddHistogram(new TBRBWaveform());
  AddHistogram(new TBRBBaseline());
  AddHistogram(new TBRBRMS());
  AddHistogram(new TBRBPH());
  AddHistogram(new TBRBPHBig());
  AddHistogram(new TBRB_Time());
  AddHistogram(new TBRBWaveformHit());
  AddHistogram(new TBRBWaveformCorrupt());
  rate_histos = new TBRB_Rates();
  AddHistogram(rate_histos);
  rate_single_histos = new TBRB_Rates_Single();
  AddHistogram(rate_single_histos);

  for(int i = 0; i < 20; i++){
    number_dark_pulses.push_back(0.0);
    number_dark_pulses_single.push_back(0.0);
    number_dark_pulses_long.push_back(0.0);
    number_dark_pulses_single_long.push_back(0.0);
    number_samples.push_back(0.0);
    number_samples_long.push_back(0.0);
  }

  fIsOffline = isoffline;

  BSingleton::GetInstance()->UpdateBaselines();

  fTree = new TTree("settings","settings");
  int hv_set[20];
  int hv_enable[20];
  fTree->Branch("hv_set",&hv_set,"hv_set[20]/Int_t");
  fTree->Branch("hv_enable",&hv_enable,"hv_enable[20]/Int_t");

  midas::odb o2 = {
    {"HVEnable", std::array<bool, 20>{} },
    {"HVSet", std::array<double, 20>{} }
  };
  o2.connect("/Equipment/PMTS07/Settings");
  for(int i = 0; i < 20; i++){
    hv_set[i] = o2["HVset"][i];
    hv_enable[i] = o2["HVenable"][i];
  }
  std::cout << "Get HV from ODB: First 8 channel: "
	    << hv_set[0] << " " << hv_enable[0] << "\n " 
	    << hv_set[1] << " " << hv_enable[1] << "\n " 
	    << hv_set[2] << " " << hv_enable[2] << "\n " 
	    << hv_set[3] << " " << hv_enable[3] << "\n " 
	    << hv_set[4] << " " << hv_enable[4] << "\n " 
	    << hv_set[5] << " " << hv_enable[5] << "\n " 
	    << hv_set[6] << " " << hv_enable[6] << "\n " 
	    << hv_set[7] << " " << hv_enable[7] << "\n " 
	    << hv_set[8] << " " << hv_enable[8] << "\n " 
	    << std::endl;
  
  fTree->Fill();

};


void TAnaManager::AddHistogram(THistogramArrayBase* histo) {
  histo->DisableAutoUpdate();

  //histo->CreateHistograms();
  fHistos.push_back(histo);

}


int TAnaManager::ProcessMidasEvent(TDataContainer& dataContainer){

  //std::cout << "Bank list " << dataContainer.GetMidasEvent().GetBankList() << std::endl;
  
  // Fill all the  histograms
  for (unsigned int i = 0; i < fHistos.size(); i++) {
    // Some histograms are very time-consuming to draw, so we can
    // delay their update until the canvas is actually drawn.
    if (!fHistos[i]->IsUpdateWhenPlotted()) {
      fHistos[i]->UpdateHistograms(dataContainer);
    }
  }

  // Do pulse finding on first 1us to calculate dark noise rate
  TBRBRawData *dt743 = dataContainer.GetEventData<TBRBRawData>("BRB0");

  if(dt743){

    std::vector<RawBRBMeasurement> measurements = dt743->GetMeasurements();

    for(int i = 0; i < measurements.size(); i++){
      

      int chan = measurements[i].GetChannel();
      int nsamples = measurements[i].GetNSamples();
      bool in_pulse = false; // are we currently in a pulse?
      

      //nsamples = 240;
      nsamples = 1000;
      bool found_pulse = false;
      int baseline = (int)BSingleton::GetInstance()->GetBaseline(chan);
      int threshold = baseline - 5;
      for(int ib = 0; ib < nsamples; ib++){
        int sample = measurements[i].GetSample(ib);

        if(sample < threshold && !in_pulse){ // found a pulse
          in_pulse = true;
	  number_dark_pulses[chan] += 1.0;
	  number_dark_pulses_long[chan] += 1.0;
	  //  if(chan == 8) std::cout << "Found dark noise pulse: " << ib << " " << sample << " " << threshold <<  std::endl;
	  found_pulse = true;
        }

        if(sample >= threshold && in_pulse){ /// finished this pulse
          in_pulse = false;
        }

      }
      number_samples[chan] += (double)nsamples;
      number_samples_long[chan] += (double)nsamples;

      if(found_pulse){
	number_dark_pulses_single[chan] += 1.0; // keep separate track of number of uncorrelated dark noise.
	number_dark_pulses_single_long[chan] += 1.0; // keep separate track of number of uncorrelated dark noise.
      }
    }
  }


  // Save the measured dark rate on a short and long timescale; long timescale gives better statistics

  double time0 = ((number_samples[0]*8.0)/1000000000.0);
  double min_time = 0.4;// seconds
  
  if(time0 > min_time and !fIsOffline){
    
    midas::odb o = {
      {"Dark Rate", std::array<double, 20>{} },
      {"Dark Rate Single", std::array<double, 20>{} }
    };
    
    o.connect("/Analyzer/DarkNoiseRate");
    
    for(int chan = 0; chan < 20; chan++){      
      double time = ((number_samples[chan]*8.0)/1000000000.0);
      double rate = 0;
      double rate_single = 0;
      if(time > 0){
	rate = number_dark_pulses[chan]/time;            
	rate_single = number_dark_pulses_single[chan]/time;            
      }
      // Save rate in ODB
      o["Dark Rate"][chan] = rate;
      o["Dark Rate Single"][chan] = rate_single;

      // Fill histograms with rate as well.
      rate_histos->GetHistogram(chan)->Fill(rate);
      rate_single_histos->GetHistogram(chan)->Fill(rate_single);
      
      if (chan == 8) std::cout << "Rate: " << chan << " " << rate << std::endl;
      number_dark_pulses[chan] = 0.0;
      number_dark_pulses_single[chan] = 0.0;
      number_samples[chan] = 0.0;      
    }

    // Use opportunity to also calculate the baseline and store in ODB
    // For the moment just use maximum bin

    midas::odb o2 = {
      {"Baseline", std::array<double, 20>{} }
    };    
    o2.connect("/Analyzer/Baselines");
    
    for(int chan = 0; chan < 20; chan++){            
      // Baseline histogram is histogram array number 1.
      TH1 *baseh = ((TH1*)(fHistos[1]->GetHistogram(chan)));
      std::cout << "Baseline: " << chan << baseh->GetEntries() <<  " "<< baseh->GetBinCenter(baseh->GetMaximumBin()) << std::endl;
      if(baseh->GetEntries() > 500){
	o2["Baseline"][chan] = baseh->GetBinCenter(baseh->GetMaximumBin());      
	baseh->Reset();
      }
    }

  }


  // Recalculate dark rate with longer timescale
  time0 = ((number_samples_long[0]*8.0)/1000000000.0);
  min_time = 5.0; // seconds
  if(time0 > min_time and !fIsOffline){
    
    midas::odb o = {
      {"Dark Rate Long", std::array<double, 20>{} },
      {"Dark Rate Single Long", std::array<double, 20>{} }
    };
    
    o.connect("/Analyzer/DarkNoiseRatePrecise");
    
    for(int chan = 0; chan < 20; chan++){      
      double time = ((number_samples_long[chan]*8.0)/1000000000.0);
      double rate = 0;
      double rate_single = 0;
      if(time > 0){
	rate = number_dark_pulses_long[chan]/time;            
	rate_single = number_dark_pulses_single_long[chan]/time;            
      }
      std::cout << "Precise rate: " << chan << " " << number_dark_pulses_single_long[chan] 
		<< " " << time << " " << rate_single << std::endl;
      o["Dark Rate Long"][chan] = rate;
      o["Dark Rate Single Long"][chan] = rate_single;
      number_dark_pulses_long[chan] = 0.0;
      number_dark_pulses_single_long[chan] = 0.0;
      number_samples_long[chan] = 0.0;      
    }

  }  


  return 1;
}


// Little trick; we only fill the transient histograms here (not cumulative), since we don't want
// to fill histograms for events that we are not displaying.
// It is pretty slow to fill histograms for each event.
void TAnaManager::UpdateTransientPlots(TDataContainer& dataContainer){
  std::vector<THistogramArrayBase*> histos = GetHistograms();
  
  for (unsigned int i = 0; i < histos.size(); i++) {
    if (histos[i]->IsUpdateWhenPlotted()) {
      histos[i]->UpdateHistograms(dataContainer);
    }
  }
}


