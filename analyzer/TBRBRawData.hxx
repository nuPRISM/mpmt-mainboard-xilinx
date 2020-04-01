#ifndef TBRBRawData_hxx_seen
#define TBRBRawData_hxx_seen

#include <vector>

#include "TGenericData.hxx"

/// Class for each channel measurement
/// For the definition of obscure variables see the CAEN BRB manual.
class RawChannelMeasurement {

  friend class TBRBRawData;
  
public:
  
  int GetNSamples(){
    return  fSamples.size();
  }
  
  int GetChannel(){ return fChan;}
  
  /// Get Errors
  uint32_t GetSample(int i){
    if(i >= 0 && i < (int)fSamples.size())
      return fSamples[i];
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
  RawChannelMeasurement(int chan){
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


  /// Get Event Counter
  uint32_t GetEventCounter() const {return (fGlobalHeader[2] & 0xffffff);};

  /// Get Event Counter
  uint32_t GetEventSize() const {return (fGlobalHeader[0] & 0xfffffff);};

  /// Get Geographical Address
  uint32_t GetGeoAddress() const {return (fGlobalHeader[1] & 0xf8000000) >> 27 ;};

  /// Get the extended trigger time tag
  uint32_t GetTriggerTimeTag() const {return fGlobalHeader[3];};

  /// Get channel mask
  uint32_t GetChMask(){return (fGlobalHeader[1] & 0xff) + ((fGlobalHeader[2] & 0xff000000) >> 16);};

  void Print();

  /// Get the Vector of TDC Measurements.
  std::vector<RawChannelMeasurement>& GetMeasurements() {return fMeasurements;}



private:
  
  // We have vectors of the headers/trailers/etc, since there can be 
  // multiple events in a bank.

  /// The overall global header
  std::vector<uint32_t> fGlobalHeader;  

  /// Vector of BRB Measurements.
  std::vector<RawChannelMeasurement> fMeasurements;

};

#endif
