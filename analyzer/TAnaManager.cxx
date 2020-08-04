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
  AddHistogram(new TBRB_Time());

  for(int i = 0; i < 20; i++){
    number_dark_pulses.push_back(0.0);
    number_samples.push_back(0.0);
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
  o2.connect("/Equipment/PMTS/Settings");
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
      int baseline = (int)BSingleton::GetInstance()->GetBaseline(chan);
      int threshold = baseline - 10;
      for(int ib = 0; ib < nsamples; ib++){
        int sample = measurements[i].GetSample(ib);

        if(sample < threshold && !in_pulse){ // found a pulse
          in_pulse = true;
	  number_dark_pulses[chan] += 1.0;
        }

        if(sample >= threshold && in_pulse){ /// finished this pulse
          in_pulse = false;
        }

      }
      number_samples[chan] += (double)nsamples;

    }
  }

  bool do_update =false;
  double time0 = ((number_samples[0]*8.0)/1000000000.0);
  double rate0 = number_dark_pulses[0]/time0;
  int nnn = dataContainer.GetMidasData().GetSerialNumber();
  if(nnn%20000 == 0)
    std::cout << "dark noise pulse = " << number_dark_pulses[0] << " total_time " 
	      << time0*1000.0 << "ms" <<  " rate = " 
	      << rate0 << std::endl;
  double min_time = 0.4;// seconds
  
  if(time0 > min_time and !fIsOffline){
    
    midas::odb o = {
      {"Dark Rate", std::array<double, 20>{} }
    };
    
    o.connect("/Analyzer/DarkNoiseRate");
    
    for(int chan = 0; chan < 20; chan++){      
      double time = ((number_samples[chan]*8.0)/1000000000.0);
      double rate = 0;
      if(time > 0)
	rate = number_dark_pulses[chan]/time;            
      o["Dark Rate"][chan] = rate;
      if (chan < 8) std::cout << "Rate: " << chan << " " << rate << std::endl;
      number_dark_pulses[chan] = 0.0;
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
      if(baseh->GetEntries() > 5000){

	
	std::cout << "Number of baseline entries ("<<chan<<"): " << baseh->GetEntries() << " " 
		  << baseh->GetMaximum() << " " << baseh->GetMaximumBin() 
		  << " baseline=" << baseh->GetBinCenter(baseh->GetMaximumBin())
		  << std::endl;
	o2["Baseline"][chan] = baseh->GetBinCenter(baseh->GetMaximumBin());
      
	baseh->Reset();
      }
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


