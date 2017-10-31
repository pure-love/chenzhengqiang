# -*- coding: utf-8 -*-
__author__ = "chenzhengqiang"
__date__ = "2017-10-17"


import sys
sys.path.append("..")

import os
import argparse
import json
import socket
import struct
import redis
import codecs
from pymongo import MongoClient
from multiprocessing import Pool
import time
import threadpool
import copy
import random
from mongo_config import *

THE_REDIS_KEY = "jk_recent_50"


def do_recent_50(redis_conn):
    '''
    :param:redis_conn the redis connection u know
    :return:
    '''
    print "obtail the recent 50 news"
    for conf in mongo_78config:
        mongo_conn = MongoClient(conf["mongo_host"], conf["mongo_port"])
        for db_set in conf["db_set"]:
            jk_ask = mongo_conn[db_set["db"]]
            ask_set = jk_ask[db_set["set"]]
            for res in ask_set.find().sort([("date",-1)]).limit(50):
                try:
                    score = time.mktime(time.strptime(res["date"], '%Y-%m-%d %H:%M:%S'))
                except:
                    continue
                record ={
                           "id": res["_id"], 
                           "title": res["title"], 
                           "catid": res["catid"],
                           "date": res["date"] if "date" in res else ""
                         }

                redis_conn.zadd(THE_REDIS_KEY, json.dumps(record, ensure_ascii=False), score)

    print "all done."

    

if __name__ == "__main__":
    
    parser = argparse.ArgumentParser()
    parser.add_argument("-H", "--redis_host")
    parser.add_argument("-P", "--redis_port")
    parser.add_argument("-D", "--redis_database")

    args = parser.parse_args()
    redis_conn = redis.Redis(host=args.redis_host, port=int(args.redis_port), db = int(args.redis_database), password="dev!1512.kang")
    do_recent_50(redis_conn)
