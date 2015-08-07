#!/usr/bin/env python

import json
import requests
import socket

url="http://192.168.1.11:8090/daemon/create_live_telecast.do"
header = {'content-type': 'application/json;charset=UTF-8'}

'''IP="192.168.1.11"
PORT=8090
SOCK=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
SOCK.connect((IP,PORT))
request_headers="POST /daemon/create_live_telecast.do HTTP/1.1\r\n\r\n"
'''
request_body = {
           'uid':55, 
           'title': 'bad boy',
           'name':'backer',
           'introduce':'so sex so handsome',
           'live_name':'what',
           'live_desc':'this is the channel balabalabala',
           'start_time':23456782,
           'category':0,
           'department':8,
           'public':0,
           'invitees':[1,2,3,4,5,6,7]
         }
request_json=json.dumps(request_body)
r=requests.post(url,data=request_json,headers=header)
print(r.text)
