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
import xlrd
import multiprocessing
import codecs
import math
import copy
from pymongo import MongoClient
from multiprocessing import Pool
from datetime import datetime
import time




def get_labels(db, baike, symptom):
    '''
    :param filter_tags:
    :param title:
    :param content:
    :param tags_size:
    :return:
    '''

    title = db["title"]
    content = db["content"]
    answers = db["answer"]
    labels = {}

    if title  and len(title) > 0:
        tags = jieba.cut(title, cut_all=True)
        for tag in tags:
            if (tag in baike) or (tag in symptom):
                labels[tag] = 0 if tag not in labels else labels[tag]+1 

    if content and len(content) > 0:
        tags = jieba.cut(content, cut_all=True)
        for tag in tags:
            if (tag in baike) or (tag in symptom):
                labels[tag] = 0 if tag not in labels else labels[tag]+1 

    for answer in answers:
        content = answer["answer_bqfx"]+answer["answer_zdyj"]+answer["answer_summary"]+answer["answer_other"]
        if content and len(content) > 0:
            tags = jieba.cut(content, cut_all=True)
            for tag in tags:
                if (tag in baike) or (tag in symptom):
                    labels[tag] = 0 if tag not in labels else labels[tag]+1 

    return labels



def do_relative_labels(test, baike, symptom):
    '''
    :param test: indicates TEST or OFFICIAL
    :param baikes:
    :param symptom:
    :param param:
    :return:
    '''
    
    test = test.upper()
    mongo_conn = DAL.get_mongo_conn(CONFIG[test])
    analysis = mongo_conn.analysis_data
    ask_set = analysis.ask_good
    
    relatives = mongo_conn.relatives 
    labels_set = relatives.labels

    mysql_conn = DAL.get_mysql_conn(CONFIG[test])
    count = 0
    
       
    for res in ask_set.find():
        labels = get_labels(res, baikes, symptom)
        labels = sorted(labels.iteritems(), key = lambda d:d[1], reverse=True)  
        relative_labels = []
        relative_articles = []
        relative_experience = []
        relative_articles_count = 0
        relative_experience_count = 0 
          
        for label in labels:
            if label[0].strip() == '':
                continue

            relative_labels.append(label[0])
            if relative_experience_count < 10:
                _id = baike[label[0]] if label[0] in baike else -1

                if _id != -1:
                    sql = "select title, description, thumb, url, inputtime as timestamp from k_jingyan where jibing_id=%s order by inputtime desc limit 10"
                    db_res = DAL.get_data_from_mysql(mysql_conn, sql, (_id))

                    try:
                        for data in db_res:
                            relative_experience.append(data)
                            relative_experience_count += 1
                    except:
                        pass   
                                 

            if relative_articles_count < 10:
                is_baike_id = False
                is_symptom_id = False

                ID = baike[label[0]] if label[0] in baike else -1

                if ID == -1:
                    ID = symptom[label[0]] if label[0] in symptom else -1
                    is_symptom_id = True 
                else:
                    is_baike_id = True
		
                   
                if ID != -1:
                    # get relative articles
                    sql=None
                    if is_baike_id:
                        sql = "select title, thumb, description, url, inputtime as timestamp from k_news where jibing_id=%s order by inputtime desc limit 10"
                    if is_symptom_id:
                        sql = "select title, thumb, description, url, inputtime as timestamp from k_zhengzhuang_news where zhengzhang_id=%s order by inputtime desc limit 10"

                    db_res = DAL.get_data_from_mysql(mysql_conn, sql, (ID))
                    for data in db_res:
                        relative_articles.append(data)
                        relative_articles_count += 1

                 
        labels_set.insert_one({"_id":res["_id"],"relative_lebels":relative_labels, "relative_articles":relative_articles, "relative_experience":relative_experience})
        count += 1
        print count
        
        

def get_baike_dic(dir_entry, f):
    baike = {}
    fpr = codecs.open(os.path.join(dir_entry, f), "r", "utf-8")
    for line in fpr:
       datas = line.split("$")
       if datas[1] not in baike:
           baike[datas[1].strip()]= int(datas[0])
    return baike

def get_symptom_dic(dir_entry, f):
    symptom = {}
    fpr = codecs.open(os.path.join(dir_entry, f), "r", "utf-8")
    for line in fpr:
       datas = line.split()
       if datas[1] not in symptom:
           symptom[datas[1].strip().replace("\r\n","")] = int(datas[0])  

    return symptom


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--test")
    
    args = parser.parse_args()

    dir_entry = "/home/backer/work/relatives/corpus"
    baikes = get_baike_dic(dir_entry, "k_disease.txt")
    symptom = get_symptom_dic(dir_entry, "k_symptom.txt")

    jieba.load_userdict(os.path.join(dir_entry, "jk_lexicon.txt"))
    jieba.analyse.set_stop_words(os.path.join(dir_entry, "stopwords.txt"))
    jieba.analyse.set_idf_path(os.path.join(dir_entry, "jk_idf.big"))

    do_relative_labels(args.test, baikes, symptom)
