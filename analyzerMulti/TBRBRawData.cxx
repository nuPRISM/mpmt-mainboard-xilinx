#include "TBRBRawData.hxx"

#include <iostream>

#define R__bswap_constant_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |               \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))


#define R__bswap_constant_16(x) \
  ((((x) & 0xff00) >>  8) | (((x) & 0x00ff) << 8))

uint64_t last_timestamp = 0;

TBRBRawData::TBRBRawData(int bklen, int bktype, const char* name, void *pdata):
    TGenericData(bklen, bktype, name, pdata)
{

  uint16_t* fData = reinterpret_cast<uint16_t*>(pdata);

  int nwords = bklen;
  int npackets = (bklen) / 533; // Number of full packets, discard tail packet with just 29 words
  //int npackets = (bklen) / 533; // Number of full packets, 
  //int npackets = (bklen) / 221; // Number of full packets, 
  if(0)std::cout << "Number of words: " << nwords
            << ", number of packets : " << npackets << std::endl;
  
  // check for correct endian-ness
  //  for(int i = 0; i < bklen/2; i++){    
  // fData[i] = R__bswap_constant_16(fData[i]);    
  //}


  
  int nadcs = ((npackets))/(8);
  if(0)  std::cout << "Number of words: " << nwords
	    << ", number of packets : " << npackets 
	    << " nadcs=" << nadcs 
	    << " bklen=" << bklen << std::endl;

  // Let's get coarse time from FPGA trailer word
  int end_adc_data = nadcs * 8 * 533;

  uint16_t data = fData[end_adc_data + 21 + 2];
  uint16_t tmp = (((data & 0xff00)>>8) | ((data & 0xff)<<8)); 
  uint64_t time0 = tmp;

  data = fData[end_adc_data + 21 + 3];
  tmp = (((data & 0xff00)>>8) | ((data & 0xff)<<8)); 
  uint64_t time1 = tmp;
  
  data = fData[end_adc_data + 21 + 4];
  tmp = (((data & 0xff00)>>8) | ((data & 0xff)<<8)); 
  uint64_t time2 = tmp;

  data = fData[end_adc_data + 21 + 5];
  tmp = (((data & 0xff00)>>8) | ((data & 0xff)<<8)); 
  uint64_t time3 = tmp;

  
  uint64_t timestamp = (time3 << 48) + (time2 << 32) + (time1 << 16) + time0;

  double tdiff = (double)(timestamp - last_timestamp) * 8.0 / 1000000.0;

  if(0){
    for(int i = 0; i<29;i++){
      uint16_t data = fData[end_adc_data +i];
      uint16_t tmp = (((data & 0xff00)>>8) | ((data & 0xff)<<8)); 
      //std::cout << fData[end_adc_data +i] << " ";
      std::cout << tmp << " ";
    }
    std::cout << std::endl;
  }
  if(1){ std::cout << " " << time0 << " "
		    << time1 << " "
		    << time2 << " "
		    << time3 << " " 
		    << timestamp << " "
		    << timestamp - last_timestamp << " "
		    << tdiff << "ms "
		    << std::endl;
    
    last_timestamp = timestamp;
    fTimestamp = timestamp;
  }

  
  // Now get the ADC data   
  for(int adc =0; adc < nadcs; adc++){ // loop over ADCs

    // save ADC number
    int sadc=-1;

    // Create the something to save samples...
    std::vector<std::vector<uint32_t> >Samples;
    for(int ch = 0; ch < 4; ch++){ Samples.push_back(std::vector<uint32_t>());}

    for(int p = 0; p < 8; p++){ // loop over packets// now 1024 samples hopefully
      //for(int p = 0; p < 4; p++){ // loop over packets

      int counter = adc*(8) + p;
      int istart = counter*533; // 29 to handle the trailer
      if(0)std::cout << "adc " << adc << " istart " << istart << std::endl;

      int frameid = fData[istart + 4];
      int packetid = fData[istart + 2];

      uint64_t time0 = fData[istart + 5];
      uint64_t time1 = fData[istart + 6];
      uint64_t time2 = fData[istart + 7];
      uint64_t time3 = fData[istart + 8];

      uint64_t timestamp = (time0 << 48) + (time1 << 32) + (time2 << 16) + time3;
      
      uint32_t cadc0 = fData[istart + 19];

      uint32_t cadc1 = fData[istart + 20];
      uint32_t cadc = (cadc0 << 16) + cadc1;
      if(sadc == -1) sadc = fData[istart + 19] >> 8;

      if(0)std::cout << "sadc " << sadc << std::endl;

	
      if(0 and p==0){std::cout << adc << " " << p << " istart=" << istart << " frame id " << frameid
		<< "packet id " << packetid
			       << " channel=0x" << std::hex << cadc0 << " " << cadc1 << " 0x" << cadc << std::dec  << std::endl;
	std::cout << "ADC:" 
		  <<"  0x" << std::hex << sadc << std::dec
		  << std::endl;
      }

      
      // Save data samples; data for 4 channels is interleaved
      // need to convert between ADC channel number and connector channel number
      int chan_map[4] = {2,3,0,1};
      for(int i = 0; i < 512; i++){
	//	int ch = chan_map[i%4];  // which channel?
	int ch = i%4;  // which channel?
	int index = i+21+istart; 
	//	uint32_t data = (fData[index] >> 4);
	uint32_t tmp1 = (((fData[index] & 0xff00)>>8) | ((fData[index] & 0xff)<<8)); // endian flip
	//uint32_t tmp1 = tmp1; //(((fData[index] & 0xff00)>>8) | ((fData[index] & 0xff)<<8)); // endian flip
	uint32_t tmp2 = (((tmp1 & 0xfff0)>>4) & 0xfff); // shift right four bits

	uint32_t data;
	if((tmp2 & 0x800) == 0x800){ // fix 2s complement encoding 
	  data = tmp2 - 2048;  
	}else{
	  data = tmp2 + 2048;  
	}
	if(0 and i < 50){
	  std::cout << "before data("<<i<<")="
		    << tmp1 << " "<< tmp2 << " " << data << " ( " << fData[index] << "/"
		    << std::hex << fData[index] << std::dec <<std::endl; 
	}

	if(ch == 0 || ch == 2){ data = 4096 - data;}  // Swap polarity of data for channels 1 and 3.

	if(0 and i < 50 and p == 0 and ch==0){
	  std::cout << "after data("<<i<<")="
		    << tmp1 << " "<< tmp2 << " " << data << " ( " << fData[index] << "/"
		    << std::hex << fData[index] << std::dec <<std::endl; 
	}
	//	if(adc == 4 && ch == 1){ data = 4096 - data;} // swap polarity back again    
	//if(adc == 4 && ch == 3){ data = 4096 - data;} // swap polarity back again    

	// Swap the channel so that channel 0 is as marked on board as J0
	//ch = 3 - ch;
	Samples[ch].push_back(data);	
	if(0 and i < 50 and p == 0 and ch==0){
	  std::cout << "after after data("<<i<<")="
		    << tmp1 << " "<< tmp2 << " " << data << " ( " << fData[index] << "/"
		    << std::hex << fData[index] << std::dec <<std::endl; 
	}
      }


    }

    // Now make the data structures with samples

    for(int ach = 0; ach < 4; ach++){

      int ch = sadc*4 + ach;
      
      RawBRBMeasurement meas = RawBRBMeasurement(ch);
      meas.AddSamples(Samples[ach]);
      fMeasurements.push_back(meas);
      if(ach ==0 && 0){
	for(int i = 0; i < 40; i++){
	  std::cout <<std::hex << Samples[0][i] << " ";
	}
	std::cout << std::dec << std::endl;

      }
    }

    if(adc == 0){

      bool bad = false;
      for(int i = 0; i < Samples[0].size(); i++){
	// check for bad samples
	if(Samples[0][i] < 2052) bad = true;
	
	//std::cout << "index = " << i << " Samples (ch0,1,2,3): " << Samples[0][i] << " , " << Samples[1][i] << " , "
	//	  << Samples[2][i] << " , " << Samples[3][i] << std::endl;
      }

      if(bad && 0){

	std::cout << "Bad event! Corrupt 0/1/129/130 " << Samples[0][0] << " " 
		  << Samples[0][1] << " " 
		  << Samples[0][128] << " " 
		  << Samples[0][129] << " " 
		  << Samples[0][128*2] << " " 
		  << Samples[0][138*2+1] << " " 
		  << Samples[0][128*3] << " " 
		  << Samples[0][138*3+1] << " " 
		  << Samples[0][128*4] << " " 
		  << Samples[0][138*4+1] << " " 
		  << Samples[0][128*5] << " " 
		  << Samples[0][138*5+1] << " nadcs=" 
		  << nadcs << " npackets" << npackets
		  << std::endl;



	//	std::cout << "FrameID/packetID/ ADC: ";
	for(int p = 0; p < 8; p++){ // loop over packets// now 1024 samples hopefully                           
	  //for(int p = 0; p < 4; p++){ // loop over packets                                                    
	  
	  int counter = adc*8 + p;
	  int istart = counter*533;

	  int frameid = fData[istart + 4];
	  int packetid = fData[istart + 2];
	  int triggerCount = fData[istart + 10];
	  uint32_t cadc0 = fData[istart + 19];
	  uint32_t cadc1 = fData[istart + 20];
	  uint32_t cadc = (cadc0 << 16) + cadc1;
	  cadc = (cadc >> 24);

	  std::cout << "frame=" << frameid << " packet=" << packetid << " trigger=" << triggerCount << " ADC=" << cadc << " " << std::endl;

	  //int frameID = (((data[4] & 0xff00)>>8) | ((data[4] & 0xff)<<8));
	  //int triggerCount = (((data[10] & 0xff00)>>8) | ((data[10] & 0xff)<<8));
	  //int adc = (((data[19] & 0xff00)>>8) | ((data[19] & 0xff)<<8));
	  //adc = (adc>>8);
	// Temp hack for lack of consistent frameIDs                                                             
	  //	packetID = packetID + (adc*8);
	}

      }

    }


  }
  
  
  
 
}

void TBRBRawData::Print(){

  std::cout << "BRB decoder for bank " << GetName().c_str() << std::endl;


}
