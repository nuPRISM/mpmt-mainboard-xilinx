#include "TBRBRawData.hxx"

#include <iostream>

#define R__bswap_constant_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |               \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))


#define R__bswap_constant_16(x) \
  ((((x) & 0xff00) >>  8) | (((x) & 0x00ff) << 8))



TBRBRawData::TBRBRawData(int bklen, int bktype, const char* name, void *pdata):
    TGenericData(bklen, bktype, name, pdata)
{

  uint16_t* fData = reinterpret_cast<uint16_t*>(pdata);
  
  // check for correct endian-ness
  for(int i = 0; i < bklen/2; i++){    
     fData[i] = R__bswap_constant_16(fData[i]);    
  }

  
  for(int i = 0; i < 50; i++){
    printf("%i 0x%04x\n",i,fData[i]);
  }
  std::cout << "_________________________________________" << std::endl;

  
  std::vector<uint32_t> Samples;
  for(int i = 21; i < 121; i++){
    Samples.push_back((fData[i] >> 4));
  }

  RawChannelMeasurement meas = RawChannelMeasurement(0);
  meas.AddSamples(Samples);
  fMeasurements.push_back(meas);


  
  
 
  //RawChannelMeasurement meas0 = RawChannelMeasurement(gr*2);
  //    meas0.AddSamples(Samples0);
  


}

void TBRBRawData::Print(){

  std::cout << "BRB decoder for bank " << GetName().c_str() << std::endl;


}
