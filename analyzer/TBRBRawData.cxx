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

  int nwords = bklen;
  int npackets = bklen / 533;
  std::cout << "Number of words: " << nwords
            << ", number of packets : " << npackets << std::endl;
  
  // check for correct endian-ness
  //  for(int i = 0; i < bklen/2; i++){    
  // fData[i] = R__bswap_constant_16(fData[i]);    
  //}

  
  std::cout << "_________________________________________" << std::endl;


  // loop over the packets
  for(int p = 0; p < npackets; p++){

    int istart = p*533;

    int frameid = fData[istart + 4];
    int packetid = fData[istart + 2];
    int channel = fData[istart + 19];
    std::cout << "p=" << p << " istart=" << istart << " frame id " << frameid
              << "packet id " << packetid
              << " channel=" << channel << std::endl;

    // ignore packetid %2 == 1... these are trailer packets...
    if(packetid % 2 == 1) continue;

    
    
    for(int i = istart; i < 35+istart; i++){
      //printf("%i 0x%04x\n",i,fData[i]);
    }
    
    std::vector<uint32_t> Samples;
    for(int i = 21 + istart; i < 533 + istart; i++){
      Samples.push_back((fData[i] >> 4));
    }
    
  
    RawChannelMeasurement meas = RawChannelMeasurement(channel);
    meas.AddSamples(Samples);
    fMeasurements.push_back(meas);

  }

  
  
 
  //RawChannelMeasurement meas0 = RawChannelMeasurement(gr*2);
  //    meas0.AddSamples(Samples0);
  


}

void TBRBRawData::Print(){

  std::cout << "BRB decoder for bank " << GetName().c_str() << std::endl;


}
