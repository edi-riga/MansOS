#
# MansOS web server - the (optional) database connection codxe
#

from __future__ import print_function
import configuration
from uuid import getnode as get_mac
import datetime
import json
import urllib2
import re

connection = None
engine = None

# Packet
usePacketSeparator = False
packetSeparator = "==="
packet = None
sampleRe = re.compile('^\w+=\d+(\.\d+)?(,[\da-fA-F]{2})?$')

try:
    from sqlalchemy import create_engine
    from sqlalchemy import Table, Column, MetaData
    from sqlalchemy.types import DateTime, Numeric, Integer, String

    # Database
    url = "mysql://%s:%s@%s/" + configuration.c.getCfgValue("dbName")
    username = configuration.c.getCfgValue("dbUsername")
    password = configuration.c.getCfgValue("dbPassword")
    host = configuration.c.getCfgValue("dbHost")
    engine = create_engine(url % (username, password, host))
    metadata = MetaData()
    observations = Table('observations', metadata,
        Column('id', Integer, primary_key = True),
        Column('obs_time', DateTime),
        Column('unit_id', String),
        Column('port', String),
        Column('type', String),
        Column('value', Numeric(20,6))
    )
except ImportError:
    # TODO: do not print this if "silent" mode is configured in config file!
    print("Warning: using a database for data storage is not possible, package dependencies are missing.")
    print("To store to use a database, install: python-sqlalchemy python-mysqldb\n")

def openDBConnection():
    global connection
    if configuration.c.getCfgValueAsBool("saveToDB") and engine != None:
        connection = engine.connect()
 
def closeDBConnection():           
    global connection
    if connection != None:
        connection.close()
        
def maybeAddDataToDatabase(port, newString):
    global usePacketSeparator, packet

    saveToDB = configuration.c.getCfgValueAsBool("saveToDB")
    sendToOpenSense = configuration.c.getCfgValueAsBool("sendToOpenSense")
    if newString == packetSeparator:
        usePacketSeparator = True
        if packet != None:
            if saveToDB:
                # Save data to DB
                saveDataToDB(packet)
            if sendToOpenSense:
                #Send data to sen.se
                sendDataToOpenSense(packet)
            packet.clear()
        else:
            packet = {}
    else:
        s = newString.replace (" ", "")
        if sampleRe.match(s) != None:
            arr = s.split("=")
            if not usePacketSeparator:
                if saveToDB:
                    # Save data to DB
                    saveDataToDB({port+":"+arr[0]: arr[1]})
                if sendToOpenSense:
                    #Send data to sen.se
                    sendDataToOpenSense({port+":"+arr[0]: arr[1]})                               
            else:
                packet[port+":"+arr[0]] = arr[1]
        else:
            print("ERROR: received incorrectly formatted sensor data via serial port!\n")
        
def saveDataToDB(packet):
    global connection
    
    mac = str(get_mac())
    for key in packet.keys():
        arr = key.split(":")
        val = packet[key]
        ins = observations.insert().values(obs_time = datetime.datetime.now(), unit_id = mac, port = arr[0], type = arr[1], value = val)
        connection.execute(ins)

def sendDataToOpenSense(packet):
    url = 'http://api.sen.se/events/'
    header = {
      'sense_key' : configuration.c.getCfgValue("senseApiKey"),
    }
    feeds = configuration.c.getCfgValue("senseApiFeeds")
    type2feedMap = {}
    for feed in feeds:
        arr = feed.split(":")
        sensorType = arr[0]
        feedId = arr[1]
        type2feedMap[sensorType] = feedId   
    data = []
    #mac = str(get_mac())
    for key in packet.keys():
        arr = key.split(":")
        #port = arr[0]
        sensorType = arr[1]
        value = packet[key]
        measurement = {
            "feed_id": type2feedMap[sensorType],
            "value":   value 
        }
        data.append(measurement)
    data_json = json.dumps(data)
    req = urllib2.Request(url, data_json, header)
    response_stream = urllib2.urlopen(req)
    json_response = response_stream.read()
    #print json_response

