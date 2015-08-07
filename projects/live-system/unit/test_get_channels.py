#!/usr/bin/env python

import json
import requests
import socket

url="http://192.168.1.11:8090/daemon/get_channel_info.do?uid=1&invitee=1"
header = {'content-type': 'application/json;charset=UTF-8'}

'''IP="192.168.1.11"
PORT=8090
SOCK=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
SOCK.connect((IP,PORT))
request_headers="POST /daemon/create_live_telecast.do HTTP/1.1\r\n\r\n"
'''
request_body = {
           'index':0, 
           'count': 10,
           #'sort_type':1,
           #'status':0,
           'category':0,
           'department':8
         }
request_json=json.dumps(request_body)
r=requests.get(url,data=request_json,headers=header)
print(r.text)
