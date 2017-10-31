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
#from list_tasks import add_page
import threadpool
import copy
import random
from mongo_config import *

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
    
    for res in catset.find().sort([("id",1)]):
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

def add_page(redis_conn, zset_key, res, score):

    count = redis_conn.zcard(zset_key)
    content =""
    can_insert = True
    
    if count >= 4000:
        '''the_first = redis_conn.zrange(zset_key, 0, 0, False, True)
        try:
            if score > float(the_first[0][1]):
                redis_conn.zremrangebyrank(zset_key, 0, 0)
            else:
                can_insert = False
        except:
            can_insert = False'''
        can_insert = False 


    if can_insert is True:
        
        the_best_answer = None
        if len(res["answer"]) > 0 :
            the_max_score = -1
            the_best_answer = res["answer"][0]

            for answer in res["answer"]:
                if int(answer["score"]) > the_max_score:
                    the_max_score = int(answer["score"])
                    the_best_answer = answer
                
            if answer["answer_bqfx"].strip() != '':
                content = the_best_answer["answer_bqfx"]

            if not content:
                if answer["answer_zdyj"].strip() != '':
                    content = the_best_answer["answer_zdyj"]

            if not content:
                if answer["answer_other"].strip() != '':
                    content = the_best_answer["answer_other"]


        page_key = {
                    "id": res["_id"], "title": res["title"], "catid": res["catid"],
                    "answer_num": res["ans_num"] if "ans_num" in res else 0,
                    "answer_accept": res["answer_accept"] if "answer_accept" in res else 0,
                    "date": res["date"] if "date" in res else "",
                    "answer": {"img_name": the_best_answer["img_name"]  if the_best_answer is not None  else "", "content":content }
                   }


        redis_conn.zadd(zset_key, json.dumps(page_key, ensure_ascii=False), score)

def do_list_pages(cats):
    '''
    :param category_relationship: ip address
    :return:
    '''
    count = 0
    redis_conn = redis.Redis(host="10.15.0.18", port=6300, password="dev!1512.kang", db=0)
    for conf in mongo_78config:
        mongo_conn = MongoClient("10.5.0.78", 38000)
        for db_set in conf["db_set"]:
            jk_ask = mongo_conn[db_set["db"]]
            ask_set = jk_ask[db_set["set"]]
            for res in ask_set.find().sort([("date",-1)]):

                if int(res["catid"]) == 0:
                    continue
                try:
                    score = time.mktime(time.strptime(res["date"], '%Y-%m-%d %H:%M:%S'))
                except:
                    continue
                try:  
                    for catid in cats[int(res["catid"])]:
                        zset_key = "%s" % (catid)                       
                        add_page(redis_conn, zset_key, res, score)
                        zset_key = "%s:%s" % (catid, res["answer_accept"])
                        add_page(redis_conn, zset_key, res, score) 
                    count += 1
                    print count
                except:
                    pass  



def del_redis_zset(category_relationships, redis_conn):
    print "delete started"
    for cat in category_relationships:
        redis_conn.delete("%s" % (cat["id"]))
        redis_conn.delete("%s:0" % (cat["id"]))
        redis_conn.delete("%s:1" % (cat["id"]))
        print "%s %s:0 %s:1 deleted" % (cat["id"], cat["id"], cat["id"])

    print "all deleted"

def cat_redis_zset(category_relationships, redis_conn):
    fpw = open("./zset.log", "w")
    print "startup"
    for cat in category_relationships:
        _all = redis_conn.zcard("%s" % (cat["id"]))
        all0 = redis_conn.zcard("%s:0" % (cat["id"]))
        all1 = redis_conn.zcard("%s:1" % (cat["id"]))
        fpw.write("%s:%s %s:0 :%s %s:1 :%s\r\n" % (cat["id"], _all, cat["id"], all0, cat["id"], all1))
    print "done."

def get_cat_parents(category_relationships):
    cats = {}
    for cat in category_relationships:
        parent_id = int(cat["id"]) 
        if parent_id not in cats:
            cats[parent_id] = []

        for _id in cat["child_ids"]:
            if int(_id) in cats:
                cats[int(_id)].append(parent_id)
            else:
                cats[int(_id)] = [parent_id]
    return cats
 

if __name__ == "__main__":
    '''parser = argparse.ArgumentParser()
    parser.add_argument("-X", "--threads")
    args = parser.parse_args()'''

    redis_conn = redis.Redis(host='10.15.0.18', port=6300, password="dev!1512.kang", db = 0)
    #mongo_conn = MongoClient(args.mongo_host, int(args.mongo_port))
    #category_relationships = get_category_relationships(mongo_conn)
    category_relationships = json.loads(open("catid_relationship.txt", "r").read())
    cats = get_cat_parents(category_relationships)
    do_list_pages(cats)
    #random.shuffle(category_relationships)
    #acreate_redis_zset(category_relationships, redis_conn)
    #del_redis_zset(category_relationships, redis_conn)
    #cat_redis_zset(category_relationships, redis_conn)
    
    '''pool = threadpool.ThreadPool(int(args.threads))
    requests = threadpool.makeRequests(do_list_pages, category_relationships)
    [pool.putRequest(req) for req in requests]
    pool.wait()'''
