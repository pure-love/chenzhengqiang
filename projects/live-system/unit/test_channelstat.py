#!/usr/bin/env python

import json
import requests
import socket

url="http://192.168.1.11:8090/daemon/channel_stat.do?channel=EFEFEFEFEFEF_1234567890&status=1"

'''IP="192.168.1.11"
PORT=8090
SOCK=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
SOCK.connect((IP,PORT))
request_headers="POST /daemon/create_live_telecast.do HTTP/1.1\r\n\r\n"
'''
r=requests.post(url)
print(r.text)
