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

def scan(category_relationships, redis_conn):
    fpw = open("./scan.log", "w")
    print "startup:%s" % (len(category_relationships))
    for cat in category_relationships:
        _all = redis_conn.zcard("%s" % (cat["id"]))
        all0 = redis_conn.zcard("%s:0" % (cat["id"]))
        all1 = redis_conn.zcard("%s:1" % (cat["id"]))
        fpw.write("%s:%s %s:0 :%s %s:1 :%s\r\n" % (cat["id"], _all, cat["id"], all0, cat["id"], all1))
    print "done."

def scan_modify(redis_conn):
    mongo78_conn = MongoClient("10.5.0.78", 38000)
    doctor = mongo78_conn.doctor
    ask_120_set = doctor.ask_doctor_1204m
    ask_xywy_set = doctor.ask_doctor_xywy4m
    
    analysis = mongo78_conn.analysis_data
    ask_set = analysis.ask_good

    
    fw = codecs.open("./pages.txt", "w", "utf-8")
    fw_log = open("./deleted.txt", "w")

    for catid in xrange(1,576):
        keys = []
        keys.append("%s" % (catid))
        keys.append("%s:0" % (catid))
        keys.append("%s:1" % (catid))
        
        for _key in keys:
            results = redis_conn.zrange(_key, 0, -1)
            records = []
            count = 0
 
            for result in results:
                result = json.loads(result)
                '''ask_res = ask_set.find_one({"_id":int(result["id"])})
                if not ask_res:
                    continue

                the_best_answer = None
                if len(ask_res["answer"]) > 0 : 
                    the_max_score = -1
                    the_best_answer = ask_res["answer"][0]
                    for answer in ask_res["answer"]:
                        if int(answer["score"]) > the_max_score:
                            the_max_score = int(answer["score"])
                            the_best_answer = answer

                    result["answer"]["content"] = the_best_answer["answer_bqfx"]+the_best_answer["answer_zdyj"]+the_best_answer["answer_other"]+the_best_answer["answer_summary"]'''

                '''mongo_res = ask_120_set.find_one({"_id":result["answer"]["id"]})
                if not mongo_res:
                    mongo_res = ask_xywy_set.find_one({"_id":result["answer"]["id"]})
                if mongo_res:
                    result["answer"]["hospital_level"] = mongo_res["hospital_level"]'''

                if result["answer"]["hospital_level"].find(u'未知') != -1:
                    result["answer"]["hospital_level"]=""

                if result["answer"]["hospital_level"].find(u'未评级') != -1:
                    result["answer"]["hospital_level"]=""
                
                
                result["_score"] = time.mktime(time.strptime(result["date"], '%Y-%m-%d %H:%M:%S'))
                records.append(result)

            fw.write("%s###%s\n" % (_key, json.dumps(records, ensure_ascii=False)))
            fw.flush()
            
            redis_conn.delete(_key)
            fw_log.write("%s\r\n" % (_key))
            print "%s deleted" % (_key)
            fw_log.flush()
            for record in records:
                redis_conn.zadd(_key, json.dumps(record, ensure_ascii=False), record["_score"])

            print "%s add page successfull" % (_key)

                   
 

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    args = parser.parse_args()
    redis_conn = redis.Redis(host='10.15.0.18', port=6300, password="dev!1512.kang", db = 0)
    #category_relationships = json.loads(open("catid_relationship.txt", "r").read()) 
    scan_modify(redis_conn)
