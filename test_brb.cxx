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

  for(int i = 0; i < 200; i++){ buffer[i] = 0;}
  for(int i = 0; i < 500; i++){ bigbuffer[i] = 0;}

  int size2 = sizeof(bigbuffer);

  sprintf(buffer,"%s",command.c_str());
  int size=sizeof(buffer);
  size = command.size();
  
  //  std::cout << "size: " << size << " , command:[" << buffer << "]"<< " " << command.size() << std::endl;
  gSocket->write(buffer,size);
  usleep(100000);
  int val = gSocket->read(bigbuffer,size2);
  usleep(150000);

  //std::cout << "Return value : " << val << std::endl;
  for(int i = 0; i < 9;i++){
    int value = (int)bigbuffer[i];
    //std::cout << value << " " ;
  }
  //std::cout << std::endl;
  
  // Try rereading if val = 2
  if(val == 2 && 0){
    int val = gSocket->read(bigbuffer,size2);

    //std::cout << "Return value second time: " << val << std::endl;
    for(int i = 0; i < 20;i++){
      int value = (int)bigbuffer[i];
      std::cout << value << " " ;
    }
    std::cout << std::endl;

  }

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

  //  std::cout << "Return value : " << val << std::endl;
  //  for(int i = 0; i < 9; i++){
  //int value = (int)bigbuffer[i];
  // std::cout << value << " " ;
  // }
  //std::cout << std::endl;

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
  
  // Get data from mainboard with single socket connection

  std::cout << "Getting data from Nuprism board with single socket connection" << std::endl;

  if(1){
    
    std::cout << "Start socket connection " << std::endl;
    KOsocket *Socket = new KOsocket(argv[1], 40);
    if(Socket->getErrorCode() != 0){
      std::cout << "Failed to connect to host" << std::endl;
      exit(0);
    }
    std::cout << "Making request" << std::endl;
    for(int i = 0 ; i < 5000;++i){
      usleep(1000000);
      
      std::string temp = ReadBrbCommand("custom_command get_pressure_sensor_temp\n", Socket);
      printf("Pressure sensor temperature (%i) =  %s\n",i,temp.c_str());
    }
    
    usleep(100000);
    std::string pressure = ReadBrbCommand("custom_command get_pressure\n", Socket);
    std::cout << "Pressure = " << pressure << std::endl;
    
    //std::string version = ReadBrbCommand("get_sw_version\n", Socket);
    //std::cout << "Software Version = " << version << std::endl;
    
    Socket->shutdown();
  }

  if(0){

    
    std::cout << "\n\nGetting data from Nuprism board with single multiple connections" << std::endl;
    
    for(int i = 0 ; i < 1000;++i){
      usleep(1000000);
      
      std::string temp2 = ReadBrbCommand2("custom_command get_clnr_status_pins\n", argv[1]);
      printf("get_clnr_status_pins (%i) =  %s\n",i,temp2.c_str());
    }
    
    usleep(100000);
    std::string pressure2 = ReadBrbCommand2("custom_command get_pressure\n", argv[1]);
    std::cout << "Pressure = " << pressure2 << std::endl;
    
    //std::string version2 = ReadBrbCommand2("get_sw_version\n", argv[1]);
    /// std::cout << "Software Version = " << version2 << std::endl;
  }

}

