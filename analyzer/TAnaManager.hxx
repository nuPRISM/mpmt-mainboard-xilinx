#ifndef TAnaManager_h
#define TAnaManager_h

#include "TBRBWaveform.h"
#include "TBaselineSing.hxx"
#include "TTree.h"

/// This is an example of how to organize a set of different histograms
/// so that we can access the same information in a display or a batch
/// analyzer.
/// Change the set of ifdef's above to define which equipment to use.
///
/// We simplify a lot of code by mostly using an array of THistogramArrayBase classes
/// for storing different histograms.
class TAnaManager  {
public:
  TAnaManager(bool isoffline);
  virtual ~TAnaManager(){
    //for(unsigned int i = 0; i < fHistos.size(); i++){
    //delete fHistos[i];
    //}
  };

  /// Processes the midas event, fills histograms, etc.
  int ProcessMidasEvent(TDataContainer& dataContainer);

  /// Update those histograms that only need to be updated when we are plotting.
  void UpdateForPlotting();

  void Initialize();

  bool CheckOption(std::string option);
    
  void BeginRun(int transition,int run,int time) {
    number_dark_pulses.clear();
    number_samples.clear();

    for(int i = 0; i < 20; i++){
      number_dark_pulses.push_back(0.0);
      number_samples.push_back(0.0);
    }

    BSingleton::GetInstance()->UpdateBaselines();
  };
  void EndRun(int transition,int run,int time) {};

  // Add a THistogramArrayBase object to the list
  void AddHistogram(THistogramArrayBase* histo);

  // Little trick; we only fill the transient histograms here (not cumulative), since we don't want
  // to fill histograms for events that we are not displaying.
  // It is pretty slow to fill histograms for each event.
  void UpdateTransientPlots(TDataContainer& dataContainer);
  
  // Get the histograms
  std::vector<THistogramArrayBase*> GetHistograms() {
    return fHistos;
  }  

private:

  
  std::vector<THistogramArrayBase*> fHistos;

  std::vector<double> number_dark_pulses;
  std::vector<double> number_dark_pulses_single;
  std::vector<double> number_dark_pulses_long;
  std::vector<double> number_dark_pulses_single_long;
  std::vector<double> number_samples;
  std::vector<double> number_samples_long;
  bool fIsOffline;

  TTree *fTree; // tree just to save HV settings
  
};



#endif


