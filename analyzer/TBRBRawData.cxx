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
  printf("%4i %4i %4i %4i\n",(fData[21]>>4),(fData[22]>>4),(fData[23]>>4),(fData[24]>>4));
  printf("%4i %4i %4i %4i\n",(fData[554]>>4),(fData[555]>>4),(fData[556]>>4),(fData[557]>>4));
  printf("%4i %4i %4i %4i\n",(fData[1087]>>4),(fData[1088]>>4),(fData[1089]>>4),(fData[1090]>>4));
  printf("%4i %4i %4i %4i\n",(fData[1620]>>4),(fData[1621]>>4),(fData[1622]>>4),(fData[1623]>>4));
  
  
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
      for(int i = 0; i < 512; i++){
	int ch = i%4;  // which channel?
	int index = i+21+istart; 
	Samples[ch].push_back((fData[index] >> 4));	
      }


    }

    // Now make the data structures with samples

    for(int ach = 0; ach < 4; ach++){

      int ch = adc*4 + ach;
      
      RawChannelMeasurement meas = RawChannelMeasurement(ch);
      meas.AddSamples(Samples[ach]);
      fMeasurements.push_back(meas);
    }

    if(adc == 0 && 0){
      for(int i = 0; i < Samples[0].size(); i++){
	std::cout << "index = " << i << " Samples (ch0,1,2,3): " << Samples[0][i] << " , " << Samples[1][i] << " , "
		  << Samples[2][i] << " , " << Samples[3][i] << std::endl;
      }


    }


  }
  
  
  
 
}

void TBRBRawData::Print(){

  std::cout << "BRB decoder for bank " << GetName().c_str() << std::endl;


}
