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


// Function for reading from mainboard with single socket
std::string ReadBrbCommand(std::string command, KOsocket *gSocket){

  char buffer[200];
  char bigbuffer[500];
  int size=sizeof(buffer);
  int size2 = sizeof(bigbuffer);

  sprintf(buffer,"%s",command.c_str());
  gSocket->write(buffer,size);
  usleep(100000);
  int val = gSocket->read(bigbuffer,size2);
  usleep(150000);

  // Strip the \r off end
  std::string rstring(bigbuffer);
  std::size_t current;
  current = rstring.find("\r");
  std::string rstring2 = rstring.substr(0, current);

  return rstring2;


}

// Function for reading from mainboard with different socket each time
std::string ReadBrbCommand2(std::string command, std::string ip){

  KOsocket *gSocket = new KOsocket(ip, 40);

  char buffer[200];
  char bigbuffer[500];
  int size=sizeof(buffer);
  int size2 = sizeof(bigbuffer);

  sprintf(buffer,"%s",command.c_str());
  gSocket->write(buffer,size);
  usleep(100000);
  int val = gSocket->read(bigbuffer,size2);
  usleep(150000);

  // Strip the \r off end
  std::string rstring(bigbuffer);
  std::size_t current;
  current = rstring.find("\r");
  std::string rstring2 = rstring.substr(0, current);

  gSocket->shutdown();

  return rstring2;


}


int main (int argc, char *argv[])
{
  
  // Get data from mainboard with single socket connection

  std::cout << "Getting data from Nuprism board with single socket connection" << std::endl;

  KOsocket *Socket = new KOsocket(argv[1], 40);
  if(Socket->getErrorCode() != 0){
    std::cout << "Failed to connect to host" << std::endl;
    exit(0);
  }

  for(int i = 0 ; i < 4;++i){
    sleep(1);

    std::string temp = ReadBrbCommand("custom_command get_pressure_sensor_temp\n", Socket);
    printf("Pressure sensor temperature =  %s\n",temp.c_str());
  }

  std::string pressure = ReadBrbCommand("custom_command get_pressure\n", Socket);
  std::cout << "Pressure = " << pressure << std::endl;

  std::string version = ReadBrbCommand("get_sw_version\n", Socket);
  std::cout << "Software Version = " << version << std::endl;
 
  Socket->shutdown();

  std::cout << "\n\nGetting data from Nuprism board with single multiple connections" << std::endl;

  for(int i = 0 ; i < 4;++i){
    sleep(1);

    std::string temp2 = ReadBrbCommand2("custom_command get_pressure_sensor_temp\n", argv[1]);
    printf("Pressure sensor temperature =  %s\n",temp2.c_str());
  }

  std::string pressure2 = ReadBrbCommand2("custom_command get_pressure\n", argv[1]);
  std::cout << "Pressure = " << pressure2 << std::endl;

  std::string version2 = ReadBrbCommand2("get_sw_version\n", argv[1]);
  std::cout << "Software Version = " << version2 << std::endl;


}

