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
                    }
                }]
            }
    },
    "_source":["title","description","thumb","url","inputtime"],
    "from":0,
    "size":50
}

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
    labels = []

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

def get_query_tags(lexicon, baike, symptom, stopwords, title, content, answers, tags_size):
    ''' 
    :param filter_tags:
    :param title:
    :param content:
    :param tags_size:
    :return:
    '''

    labels = []

    if title and len(title) > 0:
        tags = jieba.cut(title, cut_all=False)
        for tag in tags:
            if (tag in baike or tag in symptom or tag in lexicon) and tag not in stopwords and len(tag) > 0 and tag not in labels:
                labels.append(tag.strip().replace("\r\n", ""))
                if len(labels) >= tags_size:
                    return labels

    if content and len(content) > 0:
        tags = jieba.cut(content, cut_all=False)
        for tag in tags:
            if (tag in baike or tag in symptom or tag in lexicon) and tag not in stopwords and len(tag) > 0 and tag not in labels:
                labels.append(tag)
                if len(labels) >= tags_size:
                    return labels

    for answer in answers:
        tags = jieba.cut(answer["answer_zdyj"]+answer["answer_bqfx"], cut_all=False)
        for tag in tags:
            if (tag in baike or tag in symptom or tag in lexicon) and tag not in stopwords and len(tag) > 0 and tag not in labels:
                labels.append(tag)
                if len(labels) >= tags_size:
                    return labels
 

    if title and len(title) > 0:  
        tags = jieba.analyse.extract_tags(title+content, topK = tags_size)
        for tag in tags:
            if tag not in labels and len(tag) > 0 and tag not in stopwords:
                labels.append(tag)

    return labels

def get_es_handler(host, port):
    ''' 

    :param host: the elasticsearch server's host
    :param port: the elasticsearch server's port
    :return: the elasticsearch service's handler
    '''
    try:
        es = Elasticsearch([{'host':host, 'port':port}])
    except:
        es = None
    return es


def do_relative_news(test, lexicon, baike, symptom, stopwords, pid, tag_size):
    '''
    :param test: indicates TEST or OFFICIAL
    :param baikes:
    :param symptom:
    :param param:
    :return:
    '''
    
    test = test.upper()
    es = get_es_handler(CONFIG[test]["ELASTICSEARCH_HOST"],CONFIG[test]["ELASTICSEARCH_PORT"])
    mongo_conn = DAL.get_mongo_conn(CONFIG[test])
    analysis = mongo_conn.analysis_data
    ask_set = analysis.ask_good
    
    mongo_conn_8 = MongoClient("10.5.0.8", 38000)
    relatives = mongo_conn_8.relatives 
    news_set = relatives.news

    count = 0
    for ask_res in ask_set.find():

        '''if ask_res["_id"] % 4 != pid:
            continue'''

        if news_set.find_one({"_id":ask_res["_id"]}):
            count += 1 
            print "%s exists in mongo set:%s" % (ask_res["_id"], count)
             
            continue
        
        #start_time = time.time()
        labels = get_query_tags(lexicon, baike, symptom, stopwords, ask_res["title"], ask_res["content"], ask_res["answer"], tag_size)
        relative_news = []
        relative_experience = []

        QUESTION_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = "".join(labels[0:3])
        es_res = es.search(index="jk_experience", body=QUESTION_QUERY, request_timeout = 60)
        hits = es_res["hits"]["hits"]
        titles = []
        for hit in hits:
            if len(relative_experience) < 10:
                if len(hit["_source"]["title"]) >=5 and hit["_source"]["title"] not in titles:
                    titles.append(hit["_source"]["title"])  
                    relative_experience.append(hit["_source"])

        titles = [] 
        es_res = es.search(index="jk_news", body=QUESTION_QUERY, request_timeout = 60)
        hits = es_res["hits"]["hits"]
        for hit in hits:
            if len(relative_news) < 10:
                if len(hit["_source"]["title"]) >=5 and hit["_source"]["title"] not in titles:
                    titles.append(hit["_source"]["title"]) 
                    relative_news.append(hit["_source"])
            
        #news_set.save({"_id":ask_res["_id"], "relative_labels":labels, "relative_news":relative_news, "relative_experience":relative_experience})
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


def get_lexicon(dir_entry, f): 
    lexicon = {}
    fpr = codecs.open(os.path.join(dir_entry, f),"r", "utf-8")
    for line in fpr:
        data = line.split("\r\n")
        if len(data) > 0 and data[0] not in lexicon:
            lexicon[data[0].strip()] = 0    
    return lexicon

def get_stopwords(dir_entry, f): 
    stopwords = {}
    fpr = codecs.open(os.path.join(dir_entry, f),"r", "utf-8")
    for line in fpr:
        stopwords[line.strip()]=0

    return stopwords

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    #parser.add_argument("-t", "--test")
    #parser.add_argument("-m", "--min_id")
    #parser.add_argument("-M", "--max_id")
    #parser.add_argument("-T", "--tag_size")
    #parser.add_argument("-I", "--ID")
    
    args = parser.parse_args()

    dir_entry = "/home/backer/chenzhengqiang/work/JK/relatives/corpus"
    baikes = get_baike_dic(dir_entry, "k_disease.txt")
    symptom = get_symptom_dic(dir_entry, "k_symptom.txt")
    disease = {}
    for k in baikes:
        if k.strip() == '':
            continue
        disease[k] = 0
    for k in symptom:
        if k.strip() == '':
            continue
        disease[k] = 0
    codecs.open("./jk.disease.json", "w", "utf-8").write("%s" % (json.dumps(disease, ensure_ascii= False)))

    '''lexicon = get_lexicon(dir_entry, "jk_lexicon.txt")
    stopwords = get_lexicon(dir_entry, "stopwords.txt")

    jieba.load_userdict(os.path.join(dir_entry, "jk_lexicon.txt"))
    jieba.analyse.set_stop_words(os.path.join(dir_entry, "stopwords.txt"))
    jieba.analyse.set_idf_path(os.path.join(dir_entry, "jk_idf.big"))

    do_relative_news(args.test, lexicon, baikes, symptom, stopwords, int(args.ID), int(args.tag_size))'''
