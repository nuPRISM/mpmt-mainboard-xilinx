#include <stdio.h>
#include <stdlib.h>
#include "iostream"
#include "KOsocket.h"

#include "odbxx.h"
#include "midas.h"
#include "mfe.h"
#include "unistd.h"
#include "time.h"
#include "sys/time.h"
#include <stdint.h>



// Function for reading from mainboard with different socket each time
std::string ReadBrbCommand2(std::string command, std::string ip){

  KOsocket *gSocket = new KOsocket(ip, 40);

  usleep(100000);
  usleep(100);
  char buffer[200];
  char bigbuffer[500];
  int size=sizeof(buffer);
  int size2 = sizeof(bigbuffer);

  sprintf(buffer,"%s",command.c_str());
  gSocket->write(buffer,size);
  usleep(50000);
  int val = gSocket->read(bigbuffer,size2);
  usleep(150000);

  // Strip the \r off end
  std::string rstring(bigbuffer);
  std::size_t current;
  current = rstring.find("\r");
  std::string rstring2 = rstring.substr(0, current);

  gSocket->shutdown();
  usleep(150000);
  return rstring2;


}


int main (int argc, char *argv[])
{
  
  std::cout << "Getting currents from Nuprism board" << std::endl;


  float power[8];
  for(int i = 0 ; i < 8;++i){
    usleep(100000);
    
    char command[100];
    sprintf(command,"custom_command ldo_get_shunt_voltage %i\n",i+1);

    std::string temp2 = ReadBrbCommand2(command, argv[1]);

    float shunt_voltage = strtof (temp2.c_str(), NULL) * 1000.0; // in mV

    // Shunt resistor values 
    double resistor = 1.0; // three of the 
    if(i==0){ resistor = 0.1; }
    if(i==2){ resistor =200; }
    if(i==5){ resistor =0.1; }
    if(i==6){ resistor =0.05; }
    if(i==7){ resistor =0.05;}

    // Current
    float current = shunt_voltage/resistor; // in mA

    // Fix a couple currents;
    if(i==2) current = current/0.0004;
    if(i==6) current = -current;

    // Voltages in V
    double voltage;
    if(i==0) voltage = 6;
    if(i==1) voltage = 5;
    if(i==2) voltage = 1.8;
    if(i==3) voltage = 5;
    if(i==4) voltage = 3.3;
    if(i==5) voltage = 4;
    if(i==6) voltage = 12;
    if(i==7) voltage = 3.3;

    power[i] = voltage * current / 1000.0; // in W
    
    char sensor[100];
    if(i==0) sprintf(sensor,"+6V Amplifier");
    if(i==1) sprintf(sensor,"-5V PMT      ");
    if(i==2) sprintf(sensor,"+1.8V ADC    ");
    if(i==3) sprintf(sensor,"+5V PMT      ");
    if(i==4) sprintf(sensor,"+3.3V PMT    ");
    if(i==5) sprintf(sensor,"-4V Amplifier");
    if(i==6) sprintf(sensor,"+12V POE     ");
    if(i==7) sprintf(sensor,"+3.3V non-SOM");

    printf("%s (LDO%i): shunt voltage = %6.2fmV, current = %7.2fmA, power=%6.2fW\n",sensor,i+1, shunt_voltage, current,power[i]);

  }
  

  // Now calculate total power for SoM:

  double power_som = power[6] - power[0] - power[1] - power[2] - power[5] - power[7];
  printf("Total power being used on SoM is %5.2fW  / %5.2fmA\n",power_som,power_som/12.0*1000.0);


}

