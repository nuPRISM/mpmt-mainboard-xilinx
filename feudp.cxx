//
// feudp.cxx
//
// Frontend for receiving and storing UDP packets as MIDAS data banks.
//

// Remade for mPMT BRB... combine together UDP packets that have the same frameID.
//
//
//

#include <iostream>
#include <stdio.h>
#include <netdb.h> // getnameinfo()
//#include <stdlib.h>
#include <string.h> // memcpy()
#include <errno.h> // errno
//#include <unistd.h>
//#include <time.h>

#include <string>
#include <vector>
#include <map>

#include "midas.h"
#include "mfe.h"

const char *frontend_name = "feudp";                     /* fe MIDAS client name */
const char *frontend_file_name = __FILE__;               /* The frontend file name */

   BOOL frontend_call_loop = TRUE;       /* frontend_loop called periodically TRUE */
   int display_period = 0;               /* status page displayed with this freq[ms] */
   int max_event_size = 1*1024*1024;     /* max event size produced by this frontend */
   int max_event_size_frag = 5 * 1024 * 1024;     /* max for fragmented events */
   int event_buffer_size = 8*1024*1024;           /* buffer size to hold events */

  int interrupt_configure(INT cmd, INT source, PTYPE adr);
  INT poll_event(INT source, INT count, BOOL test);
  int frontend_init();
  int frontend_exit();
  int begin_of_run(int run, char *err);
  int end_of_run(int run, char *err);
  int pause_run(int run, char *err);
  int resume_run(int run, char *err);
  int frontend_loop();
  int read_event(char *pevent, INT off);


// Last frame ID
std::map<std::string, std::vector<uint16_t> > event_datas;
std::map<std::string, uint16_t> fLastFrameIDs;
std::map<std::string, bool> fGotFirstPackets;
std::map<std::string, int> fBadEventCounts;
std::map<std::string, int> nUDPpackets;

// container for storing event packets
std::vector<uint16_t> event_data;
uint16_t fLastFrameID;
bool fGotFirstPacket = false; 
int fBadEvents = 0;
int npackets = 0;

#ifndef EQ_NAME
#define EQ_NAME "UDP"
#endif

#ifndef EQ_EVID
#define EQ_EVID 1
#endif

EQUIPMENT equipment[] = {
   { EQ_NAME,                         /* equipment name */
      {EQ_EVID, 0, "SYSTEM",          /* event ID, trigger mask, Evbuf */
       EQ_MULTITHREAD, 0, "MIDAS",    /* equipment type, EventSource, format */
       TRUE, RO_ALWAYS,               /* enabled?, WhenRead? */
       1, 0, 0, 0,                   /* poll[ms], Evt Lim, SubEvtLim, LogHist */
       "", "", "",}, read_event,      /* readout routine */
   },
   {""}
};
////////////////////////////////////////////////////////////////////////////

#include <sys/time.h>

#if 0
static double GetTimeSec()
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return tv.tv_sec + 0.000001*tv.tv_usec;
}
#endif

struct Source
{
  struct sockaddr addr;
  char bank_name[5];
  std::string host_name;
};

static std::vector<Source> gSrc;

//static HNDLE hDB;
static HNDLE hKeySet; // equipment settings

static int gDataSocket;

static int gUnknownPacketCount = 0;
static bool gSkipUnknownPackets = false;

int open_udp_socket(int server_port)
{
   int status;
   
   int fd = socket(AF_INET, SOCK_DGRAM, 0);
   
   if (fd < 0) {
      cm_msg(MERROR, "open_udp_socket", "socket(AF_INET,SOCK_DGRAM) returned %d, errno %d (%s)", fd, errno, strerror(errno));
      return -1;
   }

   int opt = 1;
   status = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

   if (status == -1) {
      cm_msg(MERROR, "open_udp_socket", "setsockopt(SOL_SOCKET,SO_REUSEADDR) returned %d, errno %d (%s)", status, errno, strerror(errno));
      return -1;
   }

   int bufsize = 8*1024*1024;
   //int bufsize = 20*1024;

   status = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));

   if (status == -1) {
      cm_msg(MERROR, "open_udp_socket", "setsockopt(SOL_SOCKET,SO_RCVBUF) returned %d, errno %d (%s)", status, errno, strerror(errno));
      return -1;
   }

   struct sockaddr_in local_addr;
   memset(&local_addr, 0, sizeof(local_addr));

   local_addr.sin_family = AF_INET;
   local_addr.sin_port = htons(server_port);
   local_addr.sin_addr.s_addr = INADDR_ANY;

   status = bind(fd, (struct sockaddr *)&local_addr, sizeof(local_addr));

   if (status == -1) {
      cm_msg(MERROR, "open_udp_socket", "bind(port=%d) returned %d, errno %d (%s)", server_port, status, errno, strerror(errno));
      return -1;
   }

   int xbufsize = 0;
   unsigned size = sizeof(xbufsize);

   status = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &xbufsize, &size);

   //printf("status %d, xbufsize %d, size %d\n", status, xbufsize, size);

   if (status == -1) {
      cm_msg(MERROR, "open_udp_socket", "getsockopt(SOL_SOCKET,SO_RCVBUF) returned %d, errno %d (%s)", status, errno, strerror(errno));
      return -1;
   }

   cm_msg(MINFO, "open_udp_socket", "UDP port %d socket receive buffer size is %d", server_port, xbufsize);

   return fd;
}

bool addr_match(const Source* s, void *addr, int addr_len)
{
  int v = memcmp(&s->addr, addr, addr_len);
#if 0
  for (int i=0; i<addr_len; i++)
    printf("%3d - 0x%02x 0x%02x\n", i, ((char*)&s->addr)[i], ((char*)addr)[i]);
  printf("match %d, hostname [%s] bank [%s], status %d\n", addr_len, s->host_name.c_str(), s->bank_name, v);
#endif
  return v==0;
}

int wait_udp(int socket, int msec)
{
   int status;
   fd_set fdset;
   struct timeval timeout;

   FD_ZERO(&fdset);
   FD_SET(socket, &fdset);

   timeout.tv_sec = msec/1000;
   timeout.tv_usec = (msec%1000)*1000;

   status = select(socket+1, &fdset, NULL, NULL, &timeout);

#ifdef EINTR
   if (status < 0 && errno == EINTR) {
      return 0; // watchdog interrupt, try again
   }
#endif

   if (status < 0) {
      cm_msg(MERROR, "wait_udp", "select() returned %d, errno %d (%s)", status, errno, strerror(errno));
      return -1;
   }

   if (status == 0) {
      return 0; // timeout
   }

   if (FD_ISSET(socket, &fdset)) {
      return 1; // have data
   }

   // timeout
   return 0;
}

int find_source(Source* src, const sockaddr* paddr, int addr_len)
{
   char host[NI_MAXHOST], service[NI_MAXSERV];
      
   int status = getnameinfo(paddr, addr_len, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);
      
   if (status != 0) {
      cm_msg(MERROR, "read_udp", "getnameinfo() returned %d (%s), errno %d (%s)", status, gai_strerror(status), errno, strerror(errno));
      return -1;
   }

   char bankname[NAME_LENGTH];
   int size = sizeof(bankname);
      
   status = db_get_value(hDB, hKeySet, host, bankname, &size, TID_STRING, FALSE);
   
   if (status == DB_NO_KEY) {
      cm_msg(MERROR, "read_udp", "UDP packet from unknown host \"%s\"", host);
      cm_msg(MINFO, "read_udp", "Register this host by running following commands:");
      cm_msg(MINFO, "read_udp", "odbedit -c \"create STRING /Equipment/%s/Settings/%s\"", EQ_NAME, host);
      cm_msg(MINFO, "read_udp", "odbedit -c \"set /Equipment/%s/Settings/%s AAAA\", where AAAA is the MIDAS bank name for this host", EQ_NAME, host);
      return -1;
   } else if (status != DB_SUCCESS) {
      cm_msg(MERROR, "read_udp", "db_get_value(\"/Equipment/%s/Settings/%s\") status %d", EQ_NAME, host, status);
      return -1;
   }

   if (strlen(bankname) != 4) {
      cm_msg(MERROR, "read_udp", "ODB \"/Equipment/%s/Settings/%s\" should be set to a 4 character MIDAS bank name", EQ_NAME, host);
      cm_msg(MINFO, "read_udp", "Use this command:");
      cm_msg(MINFO, "read_udp", "odbedit -c \"set /Equipment/%s/Settings/%s AAAA\", where AAAA is the MIDAS bank name for this host", EQ_NAME, host);
      return -1;
   }
      
   cm_msg(MINFO, "read_udp", "UDP packets from host \"%s\" will be stored in bank \"%s\"", host, bankname);
      
   src->host_name = host;
   strlcpy(src->bank_name, bankname, 5);
   memcpy(&src->addr, paddr, sizeof(src->addr));
   
   return 0;
}

int read_udp(int socket, char* buf, int bufsize, char* pbankname)
{
   if (wait_udp(socket, 100) < 1)
      return 0;

#if 0
   static int count = 0;
   static double tt = 0;
   double t = GetTimeSec();

   double dt = (t-tt)*1e6;
   count++;
   if (dt > 1000) {
      printf("read_udp: %5d %6.0f usec\n", count, dt);
      count = 0;
   }
   tt = t;
#endif

   struct sockaddr addr;
   socklen_t addr_len = sizeof(addr);
   int rd = recvfrom(socket, buf, bufsize, 0, &addr, &addr_len);
   
   if (rd < 0) {
      cm_msg(MERROR, "read_udp", "recvfrom() returned %d, errno %d (%s)", rd, errno, strerror(errno));
      return -1;
   }

   for (unsigned i=0; i<gSrc.size(); i++) {
      if (addr_match(&gSrc[i], &addr, addr_len)) {
         strlcpy(pbankname, gSrc[i].bank_name, 5);
         //printf("rd %d, bank [%s]\n", rd, pbankname);
         return rd;
      }
   }

   if (gSkipUnknownPackets)
      return -1;

   Source sss;

   int status = find_source(&sss, &addr, addr_len);

   if (status < 0) {

      gUnknownPacketCount++;

      if (gUnknownPacketCount > 10) {
         gSkipUnknownPackets = true;
         cm_msg(MERROR, "read_udp", "further messages are now suppressed...");
         return -1;
      }

      return -1;
   }

   gSrc.push_back(sss);
         
   strlcpy(pbankname, sss.bank_name, 5);
         
   return rd;
}

int interrupt_configure(INT cmd, INT source, PTYPE adr)
{
   return SUCCESS;
}

int frontend_init()
{
   int status;

   status = cm_get_experiment_database(&hDB, NULL);
   if (status != CM_SUCCESS) {
      cm_msg(MERROR, "frontend_init", "Cannot connect to ODB, cm_get_experiment_database() returned %d", status);
      return FE_ERR_ODB;
   }

   std::string path;
   path += "/Equipment";
   path += "/";
   path += EQ_NAME;
   path += "/Settings";

   std::string path1 = path + "/udp_port";

   int udp_port = 50005;
   int size = sizeof(udp_port);
   status = db_get_value(hDB, 0, path1.c_str(), &udp_port, &size, TID_INT, TRUE);
   
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "frontend_init", "Cannot find \"%s\", db_get_value() returned %d", path1.c_str(), status);
      return FE_ERR_ODB;
   }
   
   status = db_find_key(hDB, 0, path.c_str(), &hKeySet);
   
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "frontend_init", "Cannot find \"%s\", db_find_key() returned %d", path.c_str(), status);
      return FE_ERR_ODB;
   }
   
   gDataSocket = open_udp_socket(udp_port);
   
   if (gDataSocket < 0) {
      printf("frontend_init: cannot open udp socket\n");
      cm_msg(MERROR, "frontend_init", "Cannot open UDP socket for port %d", udp_port);
      return FE_ERR_HW;
   }

   // Initialize the counters and structure needed for combining data packets.
   // Assume we have only two boards for now.
   event_datas["BRB0"] = std::vector<uint16_t>();
   event_datas["BRB1"] = std::vector<uint16_t>();
  
   fLastFrameIDs["BRB0"] = 0;
   fLastFrameIDs["BRB1"] = 0;

   fGotFirstPackets["BRB0"] = false;
   fGotFirstPackets["BRB1"] = false;

   fBadEventCounts["BRB0"] = 0;
   fBadEventCounts["BRB1"] = 0;

   nUDPpackets["BRB0"] = 0;
   nUDPpackets["BRB1"] = 0;

   cm_msg(MINFO, "frontend_init", "Frontend equipment \"%s\" is ready, listening on UDP port %d", EQ_NAME, udp_port);
   return SUCCESS;
}

int frontend_loop()
{
   ss_sleep(10);
   return SUCCESS;
}

int begin_of_run(int run_number, char *error)
{
   gUnknownPacketCount = 0;
   gSkipUnknownPackets = false;
   fBadEvents = 0;

   event_data.clear();
   fLastFrameID = 0;
   fGotFirstPacket = false; 

   // Initialize the counters and structure needed for combining data packets.
   // Assume we have only two boards for now.
   event_datas["BRB0"].clear();
   event_datas["BRB1"].clear();
  
   fLastFrameIDs["BRB0"] = 0;
   fLastFrameIDs["BRB1"] = 0;

   fGotFirstPackets["BRB0"] = false;
   fGotFirstPackets["BRB1"] = false;

   fBadEventCounts["BRB0"] = 0;
   fBadEventCounts["BRB1"] = 0;

   nUDPpackets["BRB0"] = 0;
   nUDPpackets["BRB1"] = 0;

   return SUCCESS;
}

int end_of_run(int run_number, char *error)
{

   if(fBadEventCounts["BRB0"] > 0){
      cm_msg(MERROR, "end_of_run", "There was at least %i suppressed events with missing UDP packets for BRB0 in this run.",fBadEventCounts["BRB0"]);
   }
   if(fBadEventCounts["BRB1"] > 0){
      cm_msg(MERROR, "end_of_run", "There was at least %i suppressed events with missing UDP packets for BRB1 in this run.",fBadEventCounts["BRB1"]);
   }
   fBadEventCounts["BRB0"] = 0;
   fBadEventCounts["BRB1"] = 0;

   event_data.clear();
   fLastFrameID = 0;
   fGotFirstPacket = false; 

   // Initialize the counters and structure needed for combining data packets.
   // Assume we have only two boards for now.
   event_datas["BRB0"].clear();
   event_datas["BRB1"].clear();
  
   fLastFrameIDs["BRB0"] = 0;
   fLastFrameIDs["BRB1"] = 0;

   fGotFirstPackets["BRB0"] = false;
   fGotFirstPackets["BRB1"] = false;

   fBadEventCounts["BRB0"] = 0;
   fBadEventCounts["BRB1"] = 0;


   return SUCCESS;
}

int pause_run(INT run_number, char *error)
{
   return SUCCESS;
}

int resume_run(INT run_number, char *error)
{
   return SUCCESS;
}

int frontend_exit()
{
   return SUCCESS;
}

INT poll_event(INT source, INT count, BOOL test)
{
   //printf("poll_event: source %d, count %d, test %d\n", source, count, test);

   if (test) {
      for (int i=0; i<count; i++)
         ss_sleep(10);
      return 1;
   }

   return 1;
}

#define MAX_UDP_SIZE (0x10000)

int read_event(char *pevent, int off)
{
   char buf[MAX_UDP_SIZE];
   char bankname[5];
   
   int length = read_udp(gDataSocket, buf, MAX_UDP_SIZE, bankname);
   if (length <= 0)
      return 0;

   std::string bname(bankname);

   // Check the frame ID

   if(length < 50) std::cerr << "Error packet too short!!! " << length << std::endl;
   uint16_t *data = (uint16_t*)buf;
   std::cout <<"_________________________________" << std::endl;
   for(int i = 0; i < 21; i++){
      uint16_t tmp = (((data[i] & 0xff00)>>8) | ((data[i] & 0xff)<<8));
      
      std::cout << tmp << " ";
   }
   std::cout << std::endl;



      if(0)   std::cout << std::hex 
             << data[0] << " " << data[1] << " "
             << data[2] << " " << data[3] << " "
             << data[4] << " " << data[5] << " "
             << std::dec << std::endl;

   int packetID = (((data[2] & 0xff00)>>8) | ((data[2] & 0xff)<<8));
   int frameID = (((data[4] & 0xff00)>>8) | ((data[4] & 0xff)<<8));
   std::cout << "frameID: " << frameID << " packetID: " << packetID << " with length: " << length << std::endl;
   for(int i = 0; i < 40; i++){
      int tttt = (((data[i] & 0xff00)>>8) | ((data[i] & 0xff)<<8));
      unsigned int ttt = (tttt >> 4);
      //printf("%4i ",ttt);
      //if(i%4==3) std::cout << std::endl;
   }


   bool saveEvent = false;
   
   //   std::cout << "FrameIds out (" << bname << "): " << frameID << " " << fLastFrameIDs[std::string("BRB0")] << " " << fLastFrameIDs[std::string("BRB1")] << std::endl;

   if(frameID != fLastFrameIDs[bname] && fGotFirstPackets[bname]){
      if(1)      std::cout << "Frame IDs differ ("<<frameID <<"/" << fLastFrameIDs[bname] 
                           << "): saving last event." << std::endl;

      if(nUDPpackets[bname]%4 != 1){ // The number of packets should be 1 + multiple of 4.  Don't save if not the right number of packets
         std::cout << "Failure! Number of packets: " << nUDPpackets[bname] << " for bank " << bname << std::endl;
         
         std::cout << "npackets: " << nUDPpackets[std::string("BRB0")] << " " << nUDPpackets[std::string("BRB1")] << std::endl;
         std::cout << "frameIDs: " << frameID << " " << fLastFrameIDs[std::string("BRB0")] << " " << fLastFrameIDs[std::string("BRB1")] << std::endl;
         nUDPpackets[bname] = 0;
         event_datas[bname].clear();
         fBadEventCounts[bname]++;
      }else{

         nUDPpackets[bname] = 0;
         
         if(0)std::cout << "Saving " << event_datas[bname].size() << " words." << std::endl;
         //printf("%4i %4i %4i %4i\n",(event_data[21]>>4),(event_data[22]>>4),(event_data[23]>>4),(event_data[24]>>4));
         //printf("%4i %4i %4i %4i\n",(event_data[554]>>4),(event_data[555]>>4),(event_data[556]>>4),(event_data[557]>>4));
         //printf("%4i %4i %4i %4i\n",(event_data[1087]>>4),(event_data[1088]>>4),(event_data[1089]>>4),(event_data[1090]>>4));
         //printf("%4i %4i %4i %4i\n",(event_data[1620]>>4),(event_data[1621]>>4),(event_data[1622]>>4),(event_data[1623]>>4));
         
         saveEvent = true;
         
         bk_init32(pevent);
         uint16_t* pdata;
         bk_create(pevent, bankname, TID_WORD, (void**)&pdata);
         for(int i = 0; i < event_datas[bname].size(); i++) *pdata++ = event_datas[bname][i];
         bk_close(pevent, pdata);
         
         
         event_datas[bname].clear();
      }

   }

   // save the data in overall packet.  Endian flip
   for(int i  = 0; i < length/2; i++){
      uint16_t tmp = (((data[i] & 0xff00)>>8) | ((data[i] & 0xff)<<8));
      
      event_datas[bname].push_back(tmp);
   }
   nUDPpackets[bname]++;

   //   std::cout << "FrameIds out1 (" << bname << "): " << frameID << " " << fLastFrameIDs[std::string("BRB0")] << " " << fLastFrameIDs[std::string("BRB1")] << std::endl;
   fLastFrameIDs[bname] = frameID;
   //std::cout << "FrameIds out2 (" << bname << "): " << frameID << " " << fLastFrameIDs[std::string("BRB0")] << " " << fLastFrameIDs[std::string("BRB1")] << std::endl;
   fGotFirstPackets[bname] = true; 

   if(!saveEvent) return 0;


   
   return bk_size(pevent); 
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
