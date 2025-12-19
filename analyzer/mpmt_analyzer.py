import midas.client
import midas.file_reader
import midas.event
import json
import datetime
import argparse
import math

try:
    import scipy
except:
    pass

try:
    import numpy as np
except:
    pass


class FersAnalyzer:
    """
    """
    def __init__(self):
        self.client = None


    def extract_data_from_event(self, ev):
        """
        Extract the necessary information from a midas event, and add it to
        `self.data`.
        
        Args:
            
        * ev (`midas.event.MidasEvent`)
        """
        fe_idx = 0
        bank_name = "BRB0"

        w, h = 8, 5
        Matrix = [[] for y in range(20)] 

        if not ev.bank_exists(bank_name):
            return
        
        bank = ev.banks[bank_name].data
        offset = 0

        word0 = bank[0 + offset]
        word1 = bank[1 + offset]
        word2 = bank[2 + offset]
        word3 = bank[3 + offset]
        word4 = bank[4 + offset]
        word5 = bank[5 + offset]
        word6 = bank[6 + offset]
        word7 = bank[7 + offset]
        #word2 = bank[2 + offset] & 0xFFFFFFFF
        #word3 = bank[2 + offset] >> 32

        bklen = len(bank)
        nwords = bklen
        npackets = int((bklen) / 533)

        
        nadcs = int(npackets/8)
        print("Number of words: ",str(nwords),", number of packets : ",str(npackets)," nadcs=",str(nadcs))

        
        print(word0,word1,word2,word3,word4,word5,word6,word7)

        for adc in range(nadcs):
         
            samples = [[] for y in range(4)]
            sadc=-1

            for p in range(8): # // loop over packets// now 1024 samples hopefully                                          
                counter = adc*8 + p;
                istart = counter*533;

                frameid = bank[istart + 4];
                packetid = bank[istart + 2];
                
                if sadc == -1:
                    sadc = bank[istart + 19] >> 8
                    
                #print(frameid,packetid)
                for  i in range(512):
                    ch = i%4 #which channel?              
                    index = i+21+istart;
                    tmp1 = (((bank[index] & 0xff00)>>8) | ((bank[index] & 0xff)<<8)) # endian flip      
                    tmp2 = (((tmp1 & 0xfff0)>>4) & 0xfff) # shift right four bits           

                    data = 0
                    if (tmp2 & 0x800) == 0x800: # fix 2s complement encoding                                                         
                        data = tmp2 - 2048;
                    else:
                        data = tmp2 + 2048;
                        
                    if ch == 0 or ch == 2:
                        data = 4096 - data

                    samples[ch].append(data)
                    ich = sadc*4 + ch;
                    Matrix[ich].append(data)

            #print(samples[0])

        #print(Matrix)
        self.waveforms = Matrix
            

    

    def rpc_handler(self, client, cmd, args, max_len):
        """
        JRPC handler we register with midas. This will generally be called when
        a user clicks a button on the associated webpage.
        
        Args:
            
        * client (`midas.client.MidasClient`)
        * cmd (str) - The command the user wants us to do.
        * args (str) - Stringified JSON of any arguments for this command.
        * max_len (int) - Maximum length of response the user will accept.
        
        Returns:
            2-tuple of (int, str) for (status code, message)
            
        Accepted commands:
           
        * get_plot_data
        * get_plot_names
        * reset_plots
        """
        ret_int = midas.status_codes["SUCCESS"]
        ret_str = ""
        
        if cmd == "get_plot_data":
            jargs = json.loads(args)
            plot_names = jargs.get("plot_names", None)

            waveform_samples = self.waveforms[0]

            ret_str = json.dumps({"data": waveform_samples, 
                                  "good_data": "yes"})

            if len(ret_str) > max_len - 1000:
                ret_str = json.dumps({"increase_max_len": True})
        else:
            raise ValueError("Unknown command '%s'" % cmd)
        
        return (ret_int, ret_str)

    def handle_new_run(self, new_odb=None):
        if new_odb is None:
            new_odb = self.client.odb_get("/")

    def run_live(self):
        """
        Main function to run in a loop forever, reading events from the SYSTEM
        buffer and responding to JRPC requests for plot creation.
        """
        print("Running")

        self.client = midas.client.MidasClient("mpmt_analyzer_py")
        
        system_buffer = self.client.open_event_buffer("SYSTEM")
        self.client.register_event_request(system_buffer, sampling_type=midas.GET_RECENT)
        self.client.register_jrpc_callback(self.rpc_handler, True)
        curr_run = None
        
        while True:

            # Handle any events
            system_event = self.client.receive_event(system_buffer, True)
            
            if system_event:
                self.extract_data_from_event(system_event)
                
            self.client.communicate(1)

            
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Low-level FERS Analyzer")
    parser.add_argument("--file", help="Load and serve a midas file, rather than connecting to live experiment")
    args = parser.parse_args()
    
    ana = FersAnalyzer()
    
    if args.file is not None:
        ana.run_file(args.file)
    else:
        ana.run_live()
