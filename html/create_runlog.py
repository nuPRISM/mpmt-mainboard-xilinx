#!/usr/bin/python
import sys
import json
import time
import os
import math
import fileinput

"""
This class deals with populating the runlog.txt file with a summary of each run.

T. Lindner
Aug 2017

Script adapted from examples from Lars Martin and Tom Feusels
"""


class runlog:



  def initialize_files(self, htmlfile,txtfile):
    htmlfile.write(
        "<!DOCTYPE html>\n "
        " <html>\n "
        "\n"
        "<head>\n"
        "<link rel='stylesheet' href='midas.css'>"
        "<script src='controls.js'></script>\n"
        "<script src='midas.js'></script>\n"
        "<script src='mhttpd.js'></script>\n"
	"<meta http-equiv='refresh' content='60'>\n "
        "<style>\n"
        "table {\n"
        "    width:100%;\n"
        "}\n"
        "table, th, td {\n"
        "    border: 1px solid black;\n"
        "    border-collapse: collapse;\n"
        "}\n"
        " th, td {\n"
        "    padding: 5px;\n"
        "    text-align: left;\n"
        "}\n"
        "table#nicetable tr:nth-child(even) {\n"
        "    background-color: #eee;\n"
        "}\n"
        "table#t01 tr:nth-child(odd) {\n"
        "   background-color:#fff;\n"
        "}\n"
        "table#nicetable th{\n"
        "    background-color: black;\n"
        "    color: white;\n"
        "}\n"
        "</style>\n"
        "</head>\n"
        "\n"
        '<body class="mcss" onload="mhttpd_init(\'Runlog\');">\n'
        "<div id='mheader'></div>\n"
        "<div id='msidenav'></div>\n"
        "<div id='mmain'>\n"


        )

    htmlfile.write('<table id="nicetable" class="mtable">\n')
    htmlfile.write('<tr> <td> Run # </td>  <td> Start </td> <td> End Time </td>\n')
    #htmlfile.write('<tr> <td> Run </td>  <td> Start </td>\n')
    htmlfile.write('<td> <pre> # BRB \n events</pre> </td> \n')
    htmlfile.write('<td> full mPMT </td> \n')
    htmlfile.write('<td> mPMT ID </td> \n')
    htmlfile.write('<td> Comments </td>\n')
    htmlfile.write('<td> Who? </td> \n')
    htmlfile.write('<td> Log data? </td> </tr>\n')
    
    txtfile.write("Run number, Start Time, Beamline enabled?, Beam on time, UCN Valve Open, ")
    txtfile.write("He-3 events, Li-6 events, UCN Experiment, Shifter, Comments")
    return

  def writecolumn(self,htmlfile,txtfile,value):

    htmlfile.write("<td>" + value + "</td>");
    txtfile.write(value + ", ");

  def odb2html(self, odb, htmlfile,txtfile):
    # Run number / data logged flag / Start time / EBuilder events / Fragment list / comments
    run_number = odb['Runinfo']['Run number']
    start_time = odb['Runinfo']['Start time']
    stop_time = odb['Runinfo']['Stop time']
    beamline_enabled = "no"


    beamon_time = 0
    brb_events = 0
    if "UDP" in odb['Equipment']:
      brb_events = odb['Equipment']['UDP']['Statistics']['Events sent']

    hv_set = 0
    if "PMTS06" in odb['Equipment']:
      if "Settings" in odb['Equipment']['PMTS06']:
        hv_set = odb['Equipment']['PMTS06']['Settings']['HVset'][0]
    #if "PMTS" in odb['Equipment']:
    #  hv_set = odb['Equipment']['PMTS06']['Settings']['HVset'][0]
      
    comment = "*NO COMMENT FIELD*"
    laser = False
    who = ""
    full_mpmt = False
    mpmt_id = ""
    if "Edit on start" in odb['Experiment']:
      comment = odb['Experiment']['Edit on start']['Run Description']
      if "Data taker" in odb['Experiment']['Edit on start']:
        who = odb['Experiment']['Edit on start']['Data taker']
      if "mPMT ID" in odb['Experiment']['Edit on start']:
        mpmt_id = odb['Experiment']['Edit on start']["mPMT ID"]
        full_mpmt = odb['Experiment']['Edit on start']["Full mPMT?"]

    laser = odb['Logger']['Channels']['0']['Settings']['Active']

    

    htmlfile.write("<tr>")
    self.writecolumn(htmlfile,txtfile,str(run_number))
    self.writecolumn(htmlfile,txtfile,str(start_time))
    self.writecolumn(htmlfile,txtfile,str(stop_time))
    self.writecolumn(htmlfile,txtfile,str(brb_events))
    self.writecolumn(htmlfile,txtfile,str(full_mpmt))
    self.writecolumn(htmlfile,txtfile,mpmt_id)
    self.writecolumn(htmlfile,txtfile,comment)
    self.writecolumn(htmlfile,txtfile,who)
    self.writecolumn(htmlfile,txtfile,str(laser))
    htmlfile.write("</tr>\n")

    txtfile.write("\n")


    

    return

  def odbFromJson(self, filename):
    with open(filename, "r") as file:
      data = file.read()
      odb = json.loads(data, "ISO-8859-1")
      return odb


  def bulkupload(self):

    htmlfile = open('/home/mpmttest/online/custom/runlist.html','w')
    txtfile = open('/home/mpmttest/online/custom/runlist.txt','w')
    self.initialize_files(htmlfile,txtfile)


    seen_datadirs = []
    data_dir = "/data/mpmttest/data/"
    print "done"
    os.chdir(data_dir)
    print "done2"
    for filename in reversed(sorted(os.listdir(data_dir))):
      if (filename.find("json") == -1):
        continue

      odb = self.odbFromJson(filename)
      print odb['Runinfo']['Run number']
      print filename
      

      self.odb2html(odb,htmlfile,txtfile)

    htmlfile.write('</table>\n')
    htmlfile.write("</div> </body>\n"
                  "</html>\n")


  def dummy(self):
    print "Dummy function"

if __name__ == "__main__":
  runlog().bulkupload()
