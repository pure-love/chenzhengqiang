#!/usr/bin/env python
# -*- coding:utf-8 -*-  

import redis
import struct,socket  
import hashlib  
import threading,random  
import time  
import struct  
from base64 import b64encode, b64decode
  
  
#[SERVER]
BIND_ADDRESS="0.0.0.0"
BIND_PORT=8000
LISTENQ=20
MUTEX=threading.Lock()
#[REDIS]
REDIS_ADDRESS="127.0.0.1"
REDIS_PORT=6379
REDIS_DB=2
REDIS_HANDLER=None

CHANNELS = {}  
MESSAGE_LENGTH = 0  
HEADER_LENGTH = 0
MAX_MESSAGE_LENGTH=140  
  

def getRedisHandler(redisHost,redisPort,redisDb):
	try:
		handler = redis.Redis(host=redisHost,port=redisPort,db=redisDb)
		return handler
	except:
		return False
		
			  
def hex2Dec(string_num):  
    return str(int(string_num.upper(), 16))  
  
    
def getDataLength(msg):     
    MESSAGE_LENGTH = ord(msg[1]) & 127  
    received_length = 0;  
    if MESSAGE_LENGTH == 126:   
        MESSAGE_LENGTH = struct.unpack('>H', str(msg[2:4]))[0]  
        HEADER_LENGTH = 8  
    elif MESSAGE_LENGTH == 127:  
        MESSAGE_LENGTH = struct.unpack('>Q', str(msg[2:10]))[0]  
        HEADER_LENGTH = 14  
    else:  
        HEADER_LENGTH = 6  
    MESSAGE_LENGTH = int(MESSAGE_LENGTH)  
    return MESSAGE_LENGTH  
          
def parseData(msg):  
    MESSAGE_LENGTH = ord(msg[1]) & 127  
    received_length = 0;  
    if MESSAGE_LENGTH == 126:  
        MESSAGE_LENGTH = struct.unpack('>H', str(msg[2:4]))[0]  
        masks = msg[4:8]  
        data = msg[8:]  
    elif MESSAGE_LENGTH == 127:  
        MESSAGE_LENGTH = struct.unpack('>Q', str(msg[2:10]))[0]  
        masks = msg[10:14]  
        data = msg[14:]  
    else:  
        masks = msg[2:6]  
        data = msg[6:]  
    i = 0  
    raw_str = ''  
    for d in data:  
        raw_str += chr(ord(d) ^ ord(masks[i%4]))  
        i += 1        
    return raw_str    
  
  
def sendMessage(message,channel=""):  
      
    message_utf_8 = message.encode('utf-8')
    for key,connections in CHANNELS.items():
    	if key == channel:
    		for connection in connections:
    			back_str = []
    			back_str.append('\x81')  
    			data_length = len(message_utf_8)
    			if data_length <= 125:  
    				back_str.append(chr(data_length))  
    			elif data_length <= 65535 :
    				back_str.append(struct.pack('b', 126))
    				back_str.append(struct.pack('>h', data_length))
    			elif data_length <= (2^64-1):
    				back_str.append(struct.pack('b', 127))
    				back_str.append(struct.pack('>q', data_length))
    			else :
    				print (u' message too long')
    			msg = ''
    			for c in back_str:
    				msg += c;
    			back_str = str(msg)   + message_utf_8#.encode('utf-8')
    			if back_str != None and len(back_str) > 0:
    				try:
    					connection.send(back_str)
    				except:
    					delSpeaker(channel, connection)
    					connection.close()	  
    			
    	 
def delSpeaker(channel, conn):  
    CHANNELS[channel].remove(conn)
    		  
  
def getChannelAndName(request_line):
	channel=""
	name=""
	args=request_line.split('?')[-1]
	for item in args.split('&'):
		key=item.split('=')[0]
		value=item.split('=')[1]
		if key == "channel":
			channel=value.split(' ')[0]
		if key == "name":
			name=value.split(' ')[0]	
	return channel,name
	  
class WebSocket(threading.Thread):
    GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"  
    def __init__(self,conn,remote, redis,path="/"):  
        threading.Thread.__init__(self)
        self.conn = conn    
        self.speaker=""
        self.channel="" 
        self.remote = remote  
        self.path = path  
        self.buffer = ""
        self.message=""  
        self.buffer_utf8 = ""  
        self.length_buffer = 0
        self.redis=None
        self.byebye=False 
    def run(self):
    	logTime = time.strftime(u'%Y/%m/%d %H:%M:%S',time.localtime(time.time()))	 
        print('%s client %s connected!' % (logTime, str(self.remote)))  
        headers = {}  
        self.handshaken = False  
        while True:  
            if self.handshaken == False:
            	logTime = time.strftime(u'%Y/%m/%d %H:%M:%S',time.localtime(time.time()))
            	print ('%s start shake hand with %s!' % (logTime, str(self.remote)))  
                self.buffer += bytes.decode(self.conn.recv(2048))  
                if self.buffer.find('\r\n\r\n') != -1:  
                    header, data = self.buffer.split('\r\n\r\n', 1)
                    #request_line = header.split("\r\n")[0]
                    for line in header.split("\r\n")[1:]:  
                        key, value = line.split(": ", 1)  
                        headers[key] = value  
                    #the ws headers    
                    headers["Location"] = ("ws://%s%s" %(headers["Host"], self.path))
                    #obtain the websocket's key  
                    key = headers['Sec-WebSocket-Key']
                    #compute the token by base64 encode and sha1 encode  
                    token = b64encode(hashlib.sha1(str.encode(str(key + self.GUID))).digest())  
                    handshake="HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "+bytes.decode(token)+"\r\nWebSocket-Origin: "+str(headers["Origin"])+"\r\nWebSocket-Location: "+str(headers["Location"])+"\r\n\r\n"  
                    self.conn.send(str.encode(str(handshake)))  
                    self.handshaken = True
                    logTime = time.strftime(u'%Y/%m/%d %H:%M:%S',time.localtime(time.time()))    
                    print ('%s shakehand with client %s ok!' %(logTime, str(self.remote)))     
                    self.buffer_utf8 = ""  
                    MESSAGE_LENGTH = 0                      
            else:  
                message=self.conn.recv(MAX_MESSAGE_LENGTH)  
                if len(message) <= 0:  
                    continue  
                if MESSAGE_LENGTH == 0:  
                    getDataLength(message)  
                self.length_buffer = self.length_buffer + len(message)  
                self.buffer = message
                if self.length_buffer - HEADER_LENGTH < MESSAGE_LENGTH :  
                    continue  
                else :
                    self.buffer_utf8 = parseData(self.buffer) #utf8                  
                    msg_unicode = str(self.buffer_utf8).decode('utf-8', 'ignore') #unicode
                    curTimestamp = time.time()
                    nowTime =  time.strftime(u'%Y/%m/%d %H:%M:%S',time.localtime(curTimestamp)) 
                    if msg_unicode == u'quit':
                        print (u'%s client %s quit!' % (nowTime, str(self.remote)))
                        self.message=  u"%s %s %s" % (nowTime, self.speaker, unicode("已退出聊天室","utf-8"))
                        self.byebye=True
                    elif ord(msg_unicode[0]) == 3:
                    	print (u'%s client %s close the browser!' % (nowTime, str(self.remote)))
                        self.message=  u"%s %s %s" % (nowTime, self.speaker, unicode("关闭了浏览器","utf-8"))
                        self.byebye=True  
                    else:
                    	if not self.speaker:
                    		self.channel,self.speaker=getChannelAndName(msg_unicode)
                    		if not self.channel or not self.speaker:
                    			self.conn.close()
                    			break
                    		MUTEX.acquire()	
                    		if not CHANNELS.has_key(self.channel):
                    			conn_list=list()
                    			conn_list.append(self.conn)
                    			CHANNELS[self.channel]=conn_list
                    		else:
                    			CHANNELS[self.channel].append(self.conn)
                    		MUTEX.release()
                    		self.message=u"%s %s %s" % (nowTime, self.speaker, unicode("加入聊天室","utf-8"))				
                     	else:
                     		self.message=u'%s %s %s: %s' % (nowTime, self.speaker, unicode("说","utf-8"),msg_unicode)
                    sendMessage(self.message,self.channel)
                    if self.redis is not None:
                    	self.redis.hsetnx(self.channel, curTimestamp, self.message)
                    if self.byebye:
                    	delSpeaker(self.channel, self.conn)
                        self.conn.close()
                        break   		  
                    self.buffer_utf8 = ""  
                    self.buffer = ""  
                    MESSAGE_LENGTH = 0  
                    self.length_buffer = 0  
            self.buffer = ""  
  
  
class WCServer(object):  
    def __init__(self):  
        self.socket = None  
    def serveForever(self):  
        print( 'Web Chat Server For Live\nAuthor:chenzhengqiang\nStart Date:2016/3/16\n-------------------------------------\n')  
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  
        self.socket.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)  
        self.socket.bind((BIND_ADDRESS,BIND_PORT))  
        self.socket.listen(LISTENQ)   
  	#REDIS_HANDLER =getRedisHandler(REDIS_ADDRESS, REDIS_PORT, REDIS_DB)
	REDIS_HANDLER = None	
  	if REDIS_HANDLER is None:
  		 print( 'Connect To Redis Server Failed\n')  
        while True:  
            sock, address = self.socket.accept()     
            newSocket = WebSocket(sock,address,REDIS_HANDLER)  
            newSocket.start()
  
  
if __name__ == "__main__":  
    wcs =WCServer()  
    wcs.serveForever()
