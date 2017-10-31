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


def do_mongo_insert(test):
    '''
    :param test: indicates test or official
    :param es_index: the specified elasticsearch index
    :param es_type: the document type of index
    :param min_id: the minimum department id
    :param max_id: the maximum department id
    :param bulks: indicates how many bulks to insert
    :return:
    '''
    mongo_conn = MongoClient("10.5.0.16", 38000)
    ask_question_all = mongo_conn.ask_question_all
    ask_all_set = ask_question_all.ask

    param = {"analysis_data_120_3":"ask_1204m_3", 
             "analysis_data_120_4":"ask_1204m_4", 
             "analysis_data_xywy_3":"ask_xywy4m_1",
             "analysis_data_xywy_4":"ask_xywy4m_1"}
    count = 0    
    for K in param:
        db = mongo_conn[K]
        ask_set = db[param[K]]
        for res in ask_set.find(no_cursor_timeout = True):
            '''if total <= 5142000:
               print "old data:%s" % (total)
               continue'''
            if res["sign"] != 1:
                continue

            ask_all_set.save(res) 
            count += 1
            print count

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--test")

    args = parser.parse_args()
    do_mongo_insert(args.test.upper())
