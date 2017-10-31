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
import xlrd
import multiprocessing
import codecs
import math
import copy
from pymongo import MongoClient
from multiprocessing import Pool
from tasks import insert_into_mongo
from datetime import datetime
import time


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-D", "--ID")
    #parser.add_argument("-p", "--processes")

    args = parser.parse_args()

    fw = open("the_lost_%s.ids.txt" % (args.ID), "w")
    mongo78 =  MongoClient("10.5.0.78", 38000)
    analysis = mongo78.analysis_data
    ask_good = analysis.ask_good

    mongo14 = MongoClient("10.15.0.14", 38000)
    mongo14.relatives.authenticate("backer", "backer@1512")
    
    relatives = mongo14.relatives
    ask_question = relatives.ask_question
    count = 0
    elapsed_time = 0
    start_time = time.time()
    ID = int(args.ID)

    for res in ask_good.find():
        if res["_id"] % 6 != ID:
            continue

        if not ask_question.find_one({"_id":res["_id"]}):  
            fw.write("%s\r\n" % (res["_id"]))
            fw.flush()
          
    fw.close()
