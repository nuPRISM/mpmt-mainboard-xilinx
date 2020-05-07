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

  if(npackets != 21) std::cout << "Error in decoding; packets not " << npackets << " = 21," << std::endl;

  
  for(int adc =0; adc < 5; adc++){ // loop over ADCs

    std::vector<std::vector<uint32_t> >Samples;
    for(int ch = 0; ch < 4; ch++){ Samples.push_back(std::vector<uint32_t>());}

    for(int p = 0; p < 4; p++){ // loop over packets

      int counter = adc*4 + p;
      int istart = counter*533;

      int frameid = fData[istart + 4];
      int packetid = fData[istart + 2];
      int cadc = fData[istart + 19];
      std::cout << adc << " " << p << " istart=" << istart << " frame id " << frameid
		<< "packet id " << packetid
		<< " channel=0x" << std::hex << cadc << std::dec  << std::endl;


      // Save data samples; data for 4 channels is interleaved
      for(int i = 21 + istart; i < 533 + istart; i++){
	int ch = (i-21)%4;  // which channel?
	Samples[ch].push_back((fData[i] >> 4));	
      }
      if(cadc == 0){
	for(int i = 0; i < 24; i++) std::cout << Samples[0][i] << ", ";
	std::cout << std::endl;
      }
      
    }

    // Now make the data structures with samples
    for(int ach = 0; ach < 4; ach++){
      
      int ch = adc*4 + ach;
      
      RawChannelMeasurement meas = RawChannelMeasurement(ch);
      meas.AddSamples(Samples[ach]);
      fMeasurements.push_back(meas);
    }


  }
  
  
  
 
}

void TBRBRawData::Print(){

  std::cout << "BRB decoder for bank " << GetName().c_str() << std::endl;


}
