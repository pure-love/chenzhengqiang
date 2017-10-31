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
    #parser.add_argument("-p", "--processes")

    args = parser.parse_args()
    xml_ids = []
    fr = open("./xml_id.txt", "r")
    for line in fr:
        _id = int(line)
        xml_ids.append(_id)
 
    fw = open("the_lost.txt", "w")
    mongo78 =  MongoClient("10.5.0.78", 38000)
    analysis = mongo78.analysis_data
    ask_good = analysis.ask_good

    mongo14 = MongoClient("10.15.0.14", 38000)
    mongo14.relatives.authenticate("backer", "backer@1512")
    
    relatives = mongo14.relatives
    ask_question = relatives.ask_question

    for _id in xml_ids:
        if not ask_question.find_one({"_id":_id}):  
            fw.write("%s\r\n" % (_id))
            fw.flush()
            print _id
          
    fw.close()
