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

from dal import dal as DAL
from config.basic_config import CONFIG
from elasticsearch import Elasticsearch
import argparse
import csv
import json
import xlwt
import multiprocessing
import codecs
import math
import copy
from pymongo import MongoClient
from multiprocessing import Pool
from datetime import datetime
import time
import xlrd


QUESTION_QUERY={
    "query":
    {   
        "bool":
            {   
                "must":
                [{  
                    "multi_match":
                    {   
                        "query":"",
                        "fields": ["title"],
                        "type":"most_fields",
                        "operator":"and",
                        "minimum_should_match":"60%"
                    }   
                }]  
            }   
    },  
    "_source":["title", "level"],
    "from":0,
    "size":1
}

def do_relative_hospitals(test):
    '''
    :param test: indicates TEST or OFFICIAL
    :param baikes:
    :param symptom:
    :param param:
    :return:
    '''
    
    test = test.upper()
    es = Elasticsearch([{'host':CONFIG[test]["ELASTICSEARCH_HOST"], 'port':CONFIG[test]["ELASTICSEARCH_PORT"]}])
    workbook = xlwt.Workbook(encoding = 'utf-8')
    worksheet_matched = workbook.add_sheet('hospital matched')
    worksheet_unmatched = workbook.add_sheet('hospital unmatched')
     
    row_matched = 0
    row_unmatched = 0
    files = ["99.xlsx","haodf.xlsx"]
    times  = 0
    for f in files:
        data = xlrd.open_workbook(f)
        tables = data.sheets()[0]

        for row in range(tables.nrows):
            QUESTION_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = tables.cell(row, 0).value
            res = es.search(index="jk_hospital", body=QUESTION_QUERY, request_timeout = 60)
            hits = res["hits"]["hits"]
            title = None
            level = None
            for hit in hits:
                title = hit["_source"]["title"]
                level = hit["_source"]["level"]

            if title is None:
               worksheet_unmatched.write(row_unmatched,0,"%s" % (tables.cell(row, 0).value))
               worksheet_unmatched.write(row_unmatched,1,"%s" % (tables.cell(row, 3).value))
               row_unmatched += 1
               continue

            tags = jieba.cut(tables.cell(row, 1).value)
            count = 0
            for tag in tags:
                if title.find(tag) != -1:
                    count += 1

            tags = jieba.lcut(title)
            worksheet_matched.write(row_matched, 0, "%s" % (tables.cell(row, 0).value))
            worksheet_matched.write(row_matched, 1, "%s" % (tables.cell(row, 3).value))
            worksheet_matched.write(row_matched, 2, "%s" % (title))
            worksheet_matched.write(row_matched, 3, "%s" % (level))
            worksheet_matched.write(row_matched, 4, "%s" % (count))
            row_matched += 1
            times += 1
            print times

        workbook.save('hospital.xls')
        
                    
       
        

def get_hospitals(dir_entry, f):
    hospitals = {}
    lines = codecs.open(os.path.join(dir_entry, f), "r", "utf-8").readlines()
    for line in lines:
       if line.strip() == '':
           continue 
       datas = line.split()
       if len(datas) < 3:
           continue
       hospitals[datas[0]] = datas[2]
       hospitals[datas[1]] = datas[2]
 
    return hospitals


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--test")
    
    args = parser.parse_args()

    dir_entry = "/home/backer/chenzhengqiang/work/JK/relatives/corpus"

    jieba.load_userdict(os.path.join(dir_entry, "jk_lexicon.txt"))
    jieba.analyse.set_stop_words(os.path.join(dir_entry, "stopwords.txt"))
    jieba.analyse.set_idf_path(os.path.join(dir_entry, "jk_idf.big"))

    do_relative_hospitals(args.test)
