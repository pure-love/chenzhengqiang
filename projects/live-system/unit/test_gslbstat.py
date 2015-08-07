#!/usr/bin/env python

import json
import requests
import socket

url="http://192.168.1.11:8090/daemon/gslbstat.do"
header = {'content-type': 'application/json;charset=UTF-8'}

'''IP="192.168.1.11"
PORT=8090
SOCK=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
SOCK.connect((IP,PORT))
request_headers="POST /daemon/create_live_telecast.do HTTP/1.1\r\n\r\n"
'''
request_body = {
           'version':"1.7.1",
           'concurrency':65535,
           'areaID':400012, 
           'uptime':123,
           'machineroom':"guangzhou",
           'streamips':["192.168.1.11","192.168.1.211"]
         }

request_json=json.dumps(request_body)
r=requests.post(url,data=request_json,headers=header)
print(r.text)
