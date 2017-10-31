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
from tasks import add_page
import threadpool
import copy

THE_REDIS_KEY = "jk_listpages"


def date_to_score(ip_addr):
    '''
    :param ip_addr:e.g:1.57.63.0
    :return:the ip address's score
    '''

    score = 0
    for v in ip_addr.split('.'):
        score = score*256 + int(v,10)
    return score


def get_catids(mongo_conn):
    catids = []
    jiexi_120 = mongo_conn.jiexi_120
    catset = jiexi_120.ask_category
    for res in catset.find():
        if res["id"] not in catids:
            catids.append(res["id"])
     
    return catids


def get_category_relationships(mongo_conn):
    '''

    :param mongo_conn:
    :return:
    '''

    jiexi_120 = mongo_conn.jiexi_120
    catset = jiexi_120.ask_category

    category_relationships = []
    index = 0
    
    
    for res in catset.find().sort([("id",1)]):
        print res["id"]
        category_relationships.append({"id":res["id"],"child_ids":[res["id"]]})
        data = copy.deepcopy(res)
           
        parent_ids = [data["id"]]
        count = 0
        while True:
            new_ids = []
            for pid in parent_ids:
                print "pid:", pid
                for _res in catset.find({"pid": pid}):
                    count += 1
                    category_relationships[index]["child_ids"].append(_res["id"])
                    new_ids.append(_res["id"])

            if len(new_ids) == 0:
                break

            parent_ids = new_ids

        index += 1

    return category_relationships


def do_list_pages(category_relationship):
    '''
    :param category_relationship: ip address
    :return:
    '''

    mongo_conn = MongoClient('10.15.0.14', 38000)
    jk_ask = mongo_conn.jk_ask
    ask_set = jk_ask.ask_question
    redis_conn = redis.Redis(host="10.15.0.18", port=6300, db=0)

    zset_key = "%s" % (str(category_relationship["id"]))
    for catid in category_relationship["child_ids"]:
        for res in ask_set.find({"catid":catid}).sort([("_id",1)]):
            try:
                score = time.mktime(time.strptime(res["date"], '%Y-%m-%d %H:%M:%S'))
            except:
                continue

            page = redis_conn.zrange(zset_key, 0, 0, False, True)
            try:
                page_key = json.loads(page[0][0])
            except Exception, e:
                page_key = None
                pass

            if not page_key or res["_id"] > page_key["id"]:
                add_page.delay(zset_key, res, score)
            else:
                break
    
def del_redis_keys(category_relationships, redis_conn):
    catids = []
    for cat in category_relationships:
        if cat["id"] not in catids:
            catids.append("%s" % cat["id"])
            catids.append("%s:0" % cat["id"])
            catids.append("%s:1" % cat["id"])

    catids.append("jk_ip2city")
    for _key in redis_conn.keys():
        if _key not in catids:
            redis_conn.delete(_key)
            print "%s deleted" % (_key) 

def cat_redis_zset(category_relationships, redis_conn):
    fpw = open("./zset.log", "w")
    print "startup"
    for cat in category_relationships:
        _all = redis_conn.zcard("%s" % (cat["id"]))
        all0 = redis_conn.zcard("%s:0" % (cat["id"]))
        all1 = redis_conn.zcard("%s:1" % (cat["id"]))
        fpw.write("%s:%s %s:0 :%s %s:1 :%s\r\n" % (cat["id"], _all, cat["id"], all0, cat["id"], all1))
    print "done."

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    args = parser.parse_args()

    redis_conn = redis.Redis(host='10.15.0.13', port=6300, db = 0)
    category_relationships = json.loads(open("catid_relationship.txt", "r").read()) 
    category_relationships.reverse()
    #acreate_redis_zset(category_relationships, redis_conn)
    #del_redis_zset(category_relationships, redis_conn)
    del_redis_keys(category_relationships, redis_conn)
    
