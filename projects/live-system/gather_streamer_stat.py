#!/usr/bin/env python
#author:chenzhengqiang
#start date:2015/7/28
#modified date:
#desc:gather the streamer's stat 

import json,requests,time
import redis

##############GLOBAL CONFIG##############
STREAMER_CONF="streamsrv.conf"
STREAMER_STAT_URL="/streamer/stat.do"
STREAMER_KEY="streamsrvlist"
STREAMER_STAT_PORT=9090
IP_KEY="IP"

REDIS_IP="192.168.1.11"
REDIS_PORT=6379
REDIS_DB=2
##################END####################


def get_streamer_conf_json(config_file):
    fstreamsrv_handler=open(config_file)
    return json.load(fstreamsrv_handler)

def generate_request_url(IP):
    url="http://"+IP+":9090"+STREAMER_STAT_URL
    return url

def get_redis_handler(redis_ip,redis_port,redis_db):
    try:
        redis_handler=redis.Redis(host=redis_ip,port=redis_port)
        return redis_handler
    except:
        return False     

def gather_streamer_stat_now(streamer_conf_json,redis_handler):    
    for single_streamer in streamer_conf_json[STREAMER_KEY]:
        try:
            IP=single_streamer[IP_KEY]
            reply=requests.get(generate_request_url(IP))
            try:
                reply_dict=json.loads(reply.text)
                if reply_dict.has_key('code'):
                    continue
                redis_handler.hset(STREAMER_KEY,IP,reply.text)
            except:
                pass
        except requests.exceptions.ConnectionError:
            pass

if __name__=='__main__':
    redis_handler=get_redis_handler(REDIS_IP,REDIS_PORT,REDIS_DB)
    if redis_handler:               
        while True:
            streamer_conf_json=get_streamer_conf_json(STREAMER_CONF)
            gather_streamer_stat_now(streamer_conf_json,redis_handler) 
            time.sleep(5)
    else:
        print("connect to redis error,just try again\n")
  
