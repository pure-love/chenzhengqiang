# -*- coding: utf-8 -*-
# __author__ = chenzhengqiang
# __date__ = 2017-09-01

import sys
sys.path.append("..")

import argparse
from dal import dal as DAL
from config.basic_config import CONFIG
import codecs
import phpserialize
import json

from es_index_mappings import *
from es_index_queries import *
from elasticsearch import Elasticsearch
from elasticsearch import helpers
from pymongo import MongoClient


def do_mongo_interval():
    '''
    :param test: indicates test or official
    :param es_index: the specified elasticsearch index
    :param es_type: the document type of index
    :param min_id: the minimum department id
    :param max_id: the maximum department id
    :param bulks: indicates how many bulks to insert
    :return:
    '''
    
    mongo16 = MongoClient("10.5.0.16", 38000)
    ask_question_all = mongo16.ask_question_all
    ask_all_set = ask_question_all.ask

    count = 0    
    for res in ask_all_set.find().sort([("_id",1)]):
        if count == 0:
            print res["_id"]

        count += 1
        if count == 3500000:
            print res["_id"]
            count = 0

if __name__ == "__main__":
    do_mongo_interval() 
