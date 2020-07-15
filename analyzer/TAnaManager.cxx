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
      int baseline = 2045;
      if(chan == 1) baseline = 2054;
      if(chan == 2) baseline = 2050;
      if(chan == 3) baseline = 2051;
      if(chan == 4) baseline = 2028;
      if(chan == 6) baseline = 2046;
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


