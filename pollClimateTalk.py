import socket
import sys
import datetime
import time
import pygame
from pygame.locals import *

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

w, h = 8, 16 
SessionID = [[0 for x in range(w)] for y in range(h)]
MacID = [[0 for x in range(w)] for y in range(h)]

quitapp=0
outdoortemp=0

# Connect the socket to the port where the server is listening
#server_address = ('192.168.1.167', 23)
#server_address = ('50.246.121.173', 8023)
server_address = ('10.88.64.134', 23)
print 'connecting to %s port %s\r\n' % server_address,

pygame.init()
pygame.display.set_mode((100,100))
print 'Press ESC in the black open window form to quit\r\n',

state=0  # 0=sync, else decode
oldstate=state+1

try:
    
  while quitapp==0:
    for event in pygame.event.get():
        if event.type == QUIT: sys.exit()
        if event.type == KEYDOWN and event.dict['key'] == 27:
            quitapp=1
    pygame.event.pump()
    if state==0:
        if oldstate!=state:
            print "Opening Socket\r\n",
            oldstate=state
        try:
            sock.connect(server_address)
            state=1
        except socket.error as msg:
            #time.sleep(10)
            oldstate=state+1  #reprint Opening info
            continue
            
    if state==1:
        if oldstate!=state:
            print "Sync Begin\r\n",
            oldstate=state
            seqcnt = 0   # 0=looking for $A0, 1=looking for $11, 2 thru 2+$11+2=waiting to finish sync
        try:
            # get in sync, look for $A0 then $11, then wait $11+2 bytes
            data = sock.recv(1)
            print data.encode('hex'),
            if seqcnt>=20:
                print '\r\nsync finished\r\n',
                state=2
            else:
                if seqcnt>=2:
                    seqcnt+=1
                else:
                    if seqcnt==1:
                        if data[0]==chr(0x11):
                            seqcnt+=1
                        else:
                            seqcnt=0
                    else:  #if seqcnt==0:
                        if data[0]==chr(0xA0):
                            seqcnt+=1
                        else:
                            seqcnt=0
        except socket.error as msg:
            socket.close()
            state=0

    if state==2:
        if oldstate!=state:
            print "Decode Begin\r\n",
            oldstate=state
            seqcnt=0
            payload=0
            command=bytearray()
            heatstart=heatend=0
            coolstart=coolend=0
            fanstart=fanend=0
            altheatstart=altheatend=0
            heaton=coolon=fanon=altheaton=0
        try:
            data = sock.recv(1)
            #if ord(data[0])!=0xFF or payload==2 or payload==1: # or len(command)>=8:
            if True:
              command.append(data[0])
              if seqcnt>9:
                  seqcnt+=1
                  if payload>0:
                      payload-=1
                  if payload==0:
                      lrc1 = 0xAA
                      lrc2 = 0
                      for b in command:
                          lrc1 += b
                          if lrc1>= 255:
                              lrc1 -= 255
                          lrc2 += lrc1
                          if lrc2>= 255:
                              lrc2 -= 255
                      if lrc1!=0:
                          state=1 # resync
                          print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S "),
                          print '(%02x,%02x) ' % (lrc1,lrc2),
                          print ' '.join('{:02x}'.format(x) for x in command),
                          command[8]=0xFF #dummy command
                          continue
                      if (command[8]&0x80)==0:
                          if command[7]!=0x79: #don't print the NODE queries
                              if len(command)>0:
                                print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S "),
                                print '(%02x,%02x) ' % (lrc1,lrc2),
                                print ' '.join('{:02x}'.format(x) for x in command),
                          #else:
                          #    print "NODE query",
                          if command[7]==0xF9:
                              print "NODEDISCOVERY reply",
                          if command[7]==0x7A:
                              print "SETADDR request",
                          if command[7]==0xFA:
                              print "SETADDR reply",
                          if command[7]==0x7B:
                              print "GETNODEID request",
                          if command[7]==0xFB:
                              print "GETNODEID reply",
                          if command[7]==0x14:
                              print "NODELST request",
                          if command[7]==0x94:
                              print "NODELST reply",
                          if command[7]==0x02:
                              print "GETSTATUS request",
                          if command[7]==0x82:
                              print "GETSTATUS reply",
                          if command[7]==0x01:
                              print "GETCFG request",
                          if command[7]==0x81:
                              print "GETCFG reply",
                          if command[7]==0x05:
                              print "DIAG request",
                          if command[7]==0x85:
                              print "DIAG reply",
                          if command[7]==0x07:
                              print "OAT request",
                          if command[7]==0x87:
                              val=(command[13]&0x3F)*16
                              val+=(command[12]>>4)
                              frac=((command[12]&0x0f)*10)/16
                              outdoortemp=val
                              if (command[13]&0x40)>0:
                                outdoortemp*=-1
                                val*=-1
                              print "OAT reply %d.%dF" % (val,frac),
                          if command[7]==0x03:
                              if command[10]==0x69:
                                  print "AltHeat request ",
                                  val=command[13]/2
                                  if val==0:
                                      if altheaton==1:
                                        altheaton=0
                                        print("OFF,OAT={}F,".format(int(outdoortemp))),
                                        #altheatend = time.time()
                                        #if altheatstart==0: altheatstart=altheatend
                                        #hours, rem = divmod(altheatend-altheatstart, 3600)
                                        #minutes, seconds = divmod(rem, 60)
                                        #if seconds>(60-minutes*2): minutes+=1
                                        #print("OnDuration={:0>2}:{:0>2}".format(int(hours),int(minutes))),
                                  else:
                                      print "%d%%," % val,
                                      if altheaton==0:
                                        altheaton=1
                                        #altheatstart=time.time()
                                        #if altheatend==0: altheatend=altheatstart
                                        #hours, rem = divmod(altheatstart-altheatend, 3600)
                                        #minutes, seconds = divmod(rem, 60)
                                        #if seconds>(60-minutes*2): minutes+=1
                                        #print("OffDuration={:0>2}:{:0>2}".format(int(hours),int(minutes))),
                              if command[10]==0x66:
                                  print "Fan request ",
                                  val=command[14]/2
                                  if val==0:
                                      if fanon==1:
                                        fanon=0
                                        print("OFF,OAT={}F,".format(int(outdoortemp))),
                                        #fanend = time.time()
                                        #if fanstart==0: fanstart=fanend
                                        #hours, rem = divmod(fanend-fanstart, 3600)
                                        #minutes, seconds = divmod(rem, 60)
                                        #if seconds>(60-minutes*2): minutes+=1
                                        #print("OnDuration={:0>2}:{:0>2}".format(int(hours),int(minutes))),
                                  else:
                                      print "%d%%," % val,
                                      if fanon==0:
                                        fanon=1
                                        #fanstart=time.time()
                                        #if fanend==0: fanend=fanstart
                                        #hours, rem = divmod(fanstart-fanend, 3600)
                                        #minutes, seconds = divmod(rem, 60)
                                        #if seconds>(60-minutes*2): minutes+=1
                                        #print("OffDuration={:0>2}:{:0>2}".format(int(hours),int(minutes))),
                              if command[10]==0x65:
                                  print "Cool request",
                                  val=command[13]/2
                                  if val==0:
                                      if coolon==1:
                                        coolon=0
                                        print("OFF,OAT={}F,".format(int(outdoortemp))),
                                        #coolend = time.time()
                                        #if coolstart==0: coolstart=coolend
                                        #hours, rem = divmod(coolend-coolstart, 3600)
                                        #minutes, seconds = divmod(rem, 60)
                                        #if seconds>(60-minutes*2): minutes+=1
                                        #print("OnDuration={:0>2}:{:0>2}".format(int(hours),int(minutes))),
                                  else:
                                      print "%d%%," % val,
                                      if coolon==0:
                                        coolon=1
                                        #coolstart=time.time()
                                        #if coolend==0: coolend=coolstart
                                        #hours, rem = divmod(coolstart-coolend, 3600)
                                        #minutes, seconds = divmod(rem, 60)
                                        #if seconds>(60-minutes*2): minutes+=1
                                        #print("OffDuration={:0>2}:{:0>2}".format(int(hours),int(minutes))),
                              if command[10]==0x64:
                                  print "Heat request",
                                  val=command[13]/2
                                  if val==0:
                                      if heaton==1:
                                        heaton=0
                                        print("OFF,OAT={}F,".format(int(outdoortemp))),
                                        #heatend = time.time()
                                        #if heatstart==0: heatstart=heatend
                                        #hours, rem = divmod(heatend-heatstart, 3600)
                                        #minutes, seconds = divmod(rem, 60)
                                        #if seconds>(60-minutes*2): minutes+=1
                                        #print("OnDuration={:0>2}:{:0>2}".format(int(hours),int(minutes))),
                                  else:
                                      print "%d%%," % val,
                                      if heaton==0:
                                        heaton=1
                                        #heatstart=time.time()
                                        #if heatend==0: heatend=heatstart
                                        #hours, rem = divmod(heatstart-heatend, 3600)
                                        #minutes, seconds = divmod(rem, 60)
                                        #if seconds>(60-minutes*2): minutes+=1
                                        #print("OnDuration={:0>2}:{:0>2}".format(int(hours),int(minutes))),
                          if command[7]==0x83:
                              print "ACK reply",
                          if command[7]!=0x79: #don't print the NODE queries
                              print '\r\n',
                      else:
                          # test
                          #print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S "),
                          #print '(%02x,%02x) ' % (lrc1,lrc2),
                          #print ' '.join('{:02x}'.format(x) for x in command),
                          #print '\r\n',
                          
                          if command[6]<10 :
                            device=command[6]&0x0F  # get deviceID
                            match=1
                            if len(command)>=27 and command[8]==0xA0:
                                for x in range(0,8):
                                    if SessionID[device][x]!=command[19+x]:
                                        match=0
                            if match==0:
                                # test
                                #print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S "),
                                #print '(%02x,%02x) ' % (lrc1,lrc2),
                                #print 'Old SessionID[%d] ' % device,
                                #for x in range(0,8):
                                #    print "%02X" % SessionID[device][x],
                                #print '\r\n',

                                print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S "),
                                print '(%02x,%02x) ' % (lrc1,lrc2),
                                print 'New SessionID[%d] ' % device,
                                for x in range(0,8):
                                    SessionID[device][x]=command[19+x]
                                    print "%02X" % SessionID[device][x],
                                print '\r\n',
                                
                            match=1
                            if len(command)>=27 and command[8]==0xA0:
                                for x in range(0,8):
                                    if MacID[device][x]!=command[11+x]:
                                        match=0
                            if match==0:
                                print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S "),
                                print '(%02x,%02x) ' % (lrc1,lrc2),
                                print 'New MacID[%d] ' % device,
                                for x in range(0,8):
                                    MacID[device][x]=command[11+x]
                                    print "%02X" % MacID[device][x],
                                print '\r\n',
                      #cmdlen=len(command)
                      #for x in range(0,cmdlen):
                      #    command.pop(0)
                      command=bytearray()
                      seqcnt=0
              else:
                  if seqcnt==9:
                      payload=ord(data[0])+2
                      seqcnt+=1
                      #print data.encode('hex'),
                  else:
                      seqcnt+=1
                      #print data.encode('hex'),
        except socket.error as msg:
            socket.close()
            state=0

finally:
    print >>sys.stderr, 'closing socket\r\n',
    sock.close()

