# -*- coding: utf-8 -*-
__author__ = "chenzhengqiang"
__date__ = "2017-10-09"


import sys
sys.path.append("..")

import os
import argparse
import json
import socket
import struct
import redis
import codecs

THE_REDIS_IPS_KEY = "jk_ip2city"

def obtain_ips(ip1, ip2):
    # ip1:0.0.0.0
    # ip2:0.255.255.255
    ips = []
    try:
        beg = struct.unpack("!L", socket.inet_aton(ip1))[0]
        end = struct.unpack("!L", socket.inet_aton(ip2))[0]
        for ip in range(beg+1, end):
            ip_str = socket.inet_ntoa(struct.pack('!L', ip))
            if ip_str not in ips:
                ips.append(ip_str)
    except:
        pass
 
    return ips

def ip_to_score(ip_addr):
    '''
    :param ip_addr:e.g:1.57.63.0
    :return:the ip address's score
    '''

    score = 0
    for v in ip_addr.split('.'):
        score = score*256 + int(v,10)
    return score


def load_ips(input_file, redis_conn):
    '''
    :param input_file:the file of ip to city information
    :param redis_conn: the redis connection
    :return: void
    '''
    print "start"
    count = 0
    fpw = codecs.open(input_file, "r", "utf-8")
    for line in fpw:
        if not line or line.strip()=='':
            continue
        data = line.split()
        if len(data) <=2:
            continue
        score = ip_to_score(data[0])
        city_id = data.pop(2)+":"
        city_id += ":".join(data)
        redis_conn.zadd(THE_REDIS_IPS_KEY, city_id, score)
        count += 1
        print count  

 
def find_city_by_ip(ip_addr, redis_conn):
    '''
    :param ip_addr: ip address
    :param redis_conn: the redis connection handler u know
    :return:the area code of the specific ip address
    '''
    score = ip_to_score(ip_addr)
    city_id = redis_conn.zrevrangebyscore(THE_REDIS_IPS_KEY, score, 0, start=0, num = 1)
    if not city_id:
        return None
    return city_id[0].partition(':')[0]

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-H", "--redis_host")
    parser.add_argument("-p", "--redis_port")
    parser.add_argument("-i", "--input_file")
    #parser.add_argument("-q", "--query")
    args = parser.parse_args()
    redis_conn = redis.Redis(host=args.redis_host, port=int(args.redis_port), db = 3)
    load_ips(args.input_file, redis_conn)
    #print find_city_by_ip(args.query, redis_conn)
