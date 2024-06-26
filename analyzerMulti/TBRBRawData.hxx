#ifndef TBRBRawData_hxx_seen
#define TBRBRawData_hxx_seen

#include <vector>

#include "TGenericData.hxx"

/// Class for each channel measurement
class RawBRBMeasurement {

  friend class TBRBRawData;
  
public:
  
  int GetNSamples(){
    return  fSamples.size();
  }
  
  int GetChannel(){ return fChan;}
  
  /// Get data... 
  int GetSample(int i){
    if(i >= 0 && i < (int)fSamples.size()){
      //uint16_t data = fSamples[i];
      int conv_data = (int)fSamples[i];
      //if(data >= 2048) conv_data -= 4096;// this is for twos complement encoding, which is now disabled.	
      return conv_data;
    }
    return 9999999;
  }
  
  void AddSamples(std::vector<uint32_t> Samples){
    fSamples = Samples;
  }

  
  // DT5743 frequency: 
  // 0 => 3.2GHz
  // 1 => 1.6GHz
  // 2 => 0.8GHz
  // 3 => 0.4GHz
  void SetFrequency(int freq){fFreq = freq;}
  int  GetFrequency(){ return fFreq;}

  void SetHitCounter(int counter){fHitCounter = counter;}
  int  GetHitCounter(){return fHitCounter;}
  
  void SetTimeCounter(int counter){fTimeCounter = counter;}
  int  GetTimeCounter(){return fTimeCounter;}  
  
  
private:
  
  int fChan; // channel number
  
  /// Constructor; need to pass in header and measurement.
  RawBRBMeasurement(int chan){
    fChan = chan;
  }
  
  std::vector<uint32_t> fSamples;
  int fFreq;
  int fHitCounter;
  int fTimeCounter;
  

};


/// Class to store raw data from CAEN 100MHz BRB (for raw readout, no-DPP).
class TBRBRawData: public TGenericData {

public:

  /// Constructor
  TBRBRawData(int bklen, int bktype, const char* name, void *pdata);


  uint64_t GetTimestamp(){return fTimestamp;}
  
  void Print();

  /// Get the Vector of TDC Measurements.
  std::vector<RawBRBMeasurement>& GetMeasurements() {return fMeasurements;}



private:
  
  // We have vectors of the headers/trailers/etc, since there can be 
  // multiple events in a bank.

  uint64_t fTimestamp;

  /// Vector of BRB Measurements.
  std::vector<RawBRBMeasurement> fMeasurements;

};

#endif
