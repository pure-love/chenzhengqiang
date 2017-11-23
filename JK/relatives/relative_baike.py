# -*- coding: utf-8 -*-
# __author__ = chenzhengqiang
# __date__ = 2017-08-29
# __desc__ = anchor text function

import sys  
reload(sys)  
sys.setdefaultencoding('utf8') 
sys.path.append("..")

import os
import jieba
import jieba.posseg as pseg
import jieba.analyse
from elasticsearch import Elasticsearch
import argparse
import csv
import json
import xlrd
import multiprocessing
import codecs
import math
import copy
from pymongo import MongoClient
from multiprocessing import Pool
from datetime import datetime
import time
import socket
import requests
import time


BAIKE_QUERY={
 "query":
        {
            "bool": {
                "must": [{
                            "multi_match":
                                {
                                    "query": "",
                                    "fields": ["title"],
                                    "minimum_should_match":"1<20%"
                                }
                        }]
                }
            },
    "_source":["title", "name", "image", "description"],  
    "from":0,
    "size":1
 }


SYMPTOM_QUERY={
 "query":
        {
            "bool": {
                "must": [{
                            "multi_match":
                                {
                                    "query":"",
                                    "fields": ["description"],
                                    "minimum_should_match":"3<60%"
                                }
                        }]
                }
            },
    "_source":["title", "name", "image","description"],
    "from":0,
    "size":3
}

HOSPITAL_QUERY={
 "query":
        {
            "bool": {
                "must": [{
                            "multi_match":
                                {
                                    "query":"",
                                    "fields": ["doctors.advantage"]
                                }
                        }]
                }
            }
}

NEWS_QUERY={
 "query":
        {
            "bool": {
                "must": [{
                            "multi_match":
                                {
                                    "query":"",
                                    "fields": ["title"]
                                }
                        }]
                }
            }
}

def do_relatives(es_host, es_port):
    es = Elasticsearch([{'host':es_host, 'port':es_port}])
    mongo_conn = MongoClient("10.5.0.78", 38000)
    analysis = mongo_conn.analysis_data
    ask_good = analysis.ask_good

    for ask_res in ask_good.find():
        print "QUERY:",ask_res["title"]
        BAIKE_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = ask_res["title"]

        es_res = es.search(index="jk_baike", body=BAIKE_QUERY, request_timeout=60)
        hits = es_res["hits"]["hits"]
        for hit in hits:
            print "BAIKE:",hit["_source"]["title"]

        SYMPTOM_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = ask_res["title"]
        es_res = es.search(index="jk_symptom", body=SYMPTOM_QUERY, request_timeout=60)
        hits = es_res["hits"]["hits"]
        for hit in hits:
            print "SYMPTOM:",hit["_source"]["title"]

        HOSPITAL_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = ask_res["title"]
        es_res = es.search(index="jk_hospital_relative", body=HOSPITAL_QUERY, request_timeout=60)
        hits = es_res["hits"]["hits"]
        for hit in hits:
            print "HOSPITAL:",hit["_source"]["title"]

        NEWS_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = ask_res["title"]
        es_res = es.search(index="jk_news_relative", body=NEWS_QUERY, request_timeout=60)
        hits = es_res["hits"]["hits"]
        for hit in hits:
            print "NEWS:",hit["_source"]["title"]
        print "---------------------------------------------------------------\n"
        time.sleep(2) 


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    args = parser.parse_args()
    es_host = socket.gethostbyname(socket.gethostname())
    do_relatives(es_host, 9200)
