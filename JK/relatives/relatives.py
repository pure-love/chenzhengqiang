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
import socket
import requests
import time

G_WEIGHT={}
G_DISEASE={}
BAIKE_QUERY={
 "query":
        {
            "bool": {
                "must": [{
                            "multi_match":
                                {
                                    "query": "",
                                    "fields": ["title^100", "symptom"]
                                }
                        }],
                "disable_coord": "true"
                }
            },
    "from":0,
    "size":5
  }


SYMPTOM_QUERY={
    "query":
        {
            "bool": {
                "must": [{
                            "multi_match":
                                {
                                    "query": "",
                                    "fields": ["title","description"]
                                }
                        }],
                "disable_coord": "true"
                }
        },
    "_source":["title","name"],
    "from":0,
    "size":5
  }


def get_es_handler(host, port):
    '''
    :param host: the elasticsearch server's host
    :param port: the elasticsearch server's port
    :return: the elasticsearch service's handler
    '''
    es = Elasticsearch([{'host':host, 'port':port}])
    return es


def analyse_query(lexicon, title, content, tags_size):
    '''
    :param filter_tags:
    :param title:
    :param content:
    :param tags_size:
    :return:
    '''

    title_tags = []
    content_tags = []
    relative_baike = {}
    baike_key = ""

    if title and len(title) > 0:
        tags = jieba.cut(title, cut_all=False)
        for tag in tags:
            if not baike_key and tag in lexicon:
                baike_key = tag
            if tag in lexicon and len(tag) > 0 and tag not in title_tags:
                if tag in G_DISEASE and not relative_baike:
                    relative_baike = G_DISEASE[tag]
                title_tags.append(tag.strip())
                if len(title_tags) >= tags_size:
                    break

    if content and len(content) > 0:
        tags = jieba.cut(content, cut_all=False)
        for tag in tags:
            if not baike_key and tag in lexicon:
                baike_key = tag
            if tag in lexicon and tag not in content_tags and tag not in title_tags and len(tag) > 0:
                if tag in G_DISEASE and not relative_baike:
                    relative_baike = G_DISEASE[tag]
                content_tags.append(tag)

    if len(title_tags) < tags_size:
        if title and len(title) > 0:  
            tags = jieba.analyse.extract_tags(title, topK = tags_size)
            for tag in tags:
                if tag not in title_tags and len(tag) > 0:
                    title_tags.append(tag)
                    if len(title_tags) >= tags_size:
                        break


    return title_tags, content_tags, relative_baike, baike_key



def get_question_titles(es, es_index, query, title_id, title_tags, content_tags, ask_good_set, doctors_set):
    '''
    :param es:
    :param es_index:
    :param db_title:
    :param title_tags:
    :param content_tags:
    :return:
    '''

    titles_count = 15
    question_titles = []
    all_ids = {title_id:0}

    ask_query = {
        "query": {
            "bool": {
                "must": [
                    {"multi_match": {"query": "", "fields": ["title"], "minimum_should_match": "75%"}}],
                "should":[{"match_phrase":{"title":{"query":"","slop":2}}}]
                }
        },
        "from":0,
        "size":15
    }

    ask_query["query"]["bool"]["must"][0]["multi_match"]["query"] = query
    ask_query["query"]["bool"]["should"][0]["match_phrase"]["title"]["query"] = query

    terms = []
    weight = -1
    for tag in title_tags:
        if tag in G_WEIGHT:
            terms.append(tag)
            if G_WEIGHT[tag] > weight:
                weight = G_WEIGHT[tag]
            if len(terms) == 2:
                break

    if len(terms) > 0:
        ask_query["query"]["bool"]["should"].append({"match": {"title": {"query": "".join(terms), "boost": weight}}})

    es_res = es.search(index=es_index, body=ask_query, request_timeout = 60, sort="_score:desc,answers:desc")
    hits = es_res["hits"]["hits"]
    for hit in hits:
        es_id = int(hit["_id"])
        if es_id in all_ids:
            continue
        all_ids[es_id] = 0
        ask_res = ask_good_set.find_one({"_id": es_id})
        if not ask_res:
            continue
        max_score = -1
        best_answer = []
        for answer in ask_res["answer"]:
            if answer["id"].strip() == "":
                continue
            doctor_res = doctors_set.find_one({"_id":answer["id"]})
            if not doctor_res:
                continue
            if doctor_res["doctor_score"] > max_score:
                max_score = doctor_res["doctor_score"]
                best_answer = answer

        if len(question_titles) < 5:
            question_titles.append({"id":es_id,
                                    "title":hit["_source"]["title"],
                                    "content":ask_res["content"],
                                    "answer":best_answer,
                                    "date":ask_res["date"]})
        else:
            question_titles.append( { "id": es_id,
                                      "title": hit["_source"]["title"]
                                    }
                                  )

        if len(question_titles) == titles_count:
            return question_titles


    for index in range(len(title_tags),0,-1):
        the_query  = "".join(title_tags[:index])
        ask_query["query"]["bool"]["must"][0]["multi_match"]["query"] = the_query
        es_res = es.search(index=es_index, body=ask_query, request_timeout=60, sort="_score:desc,answers:desc")
        hits = es_res["hits"]["hits"]
        for hit in hits:
            es_id = int(hit["_id"])
            if es_id in all_ids:
                continue
            all_ids[es_id] = 0
            ask_res = ask_good_set.find_one({"_id": es_id})
            if not ask_res:
                continue
            max_score = -1
            best_answer = []
            for answer in ask_res["answer"]:
                doctor_res = doctors_set.find_one({"_id": answer["id"]})
                if not doctor_res:
                    continue
                if doctor_res["doctor_score"] > max_score:
                    max_score = doctor_res["doctor_score"]
                    best_answer = answer

            if len(question_titles) < 5:
                question_titles.append({"id": es_id,
                                        "title": hit["_source"]["title"],
                                        "content":ask_res["content"],
                                        "answer":best_answer,
                                        "date":ask_res["date"]})
            else:
                question_titles.append({"id": es_id,
                                        "title": hit["_source"]["title"]
                                        }
                                       )
            if len(question_titles) == titles_count:
                return question_titles

    return question_titles


def get_baike(es, es_index, relative_baike, baike_key):
    '''
    :param query_tags:
    :param baike_tags:
    :return:
    '''
    if relative_baike:
        SYMPTOM_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = relative_baike["title"]
        es_res = es.search(index="jk_symptom", body=SYMPTOM_QUERY, request_timeout=60)
        hits = es_res["hits"]["hits"]
        symptoms = []
        for hit in hits:
            symptoms.append({
                        "title":hit["_source"]["title"],
                        "name":hit["_source"]["name"]
                       })

        baikes = []
        BAIKE_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = relative_baike["title"]
        es_res = es.search(index=es_index, body=BAIKE_QUERY, request_timeout=60)
        hits = es_res["hits"]["hits"]
        for hit in hits:
            if hit["_source"]["title"] != relative_baike["title"]:
                baikes.append({
                                "name":hit["_source"]["name"],
                                "title": hit["_source"]["title"],
                              })

        relative_baike["relative_symptom"]=symptoms
        relative_baike["relative_baike"]=baikes


    if not relative_baike and baike_key:
        BAIKE_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = baike_key
        es_res = es.search(index=es_index, body=BAIKE_QUERY, request_timeout=60)
        hits = es_res["hits"]["hits"]
        for hit in hits:
            relative_baike["title"] = hit["_source"]["title"]
            relative_baike["description"] = hit["_source"]["description"]
            relative_baike["name"] = hit["_source"]["name"]
            break

        if relative_baike:
            SYMPTOM_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = relative_baike["title"]
            es_res = es.search(index="jk_symptom", body=SYMPTOM_QUERY, request_timeout=60)
            hits = es_res["hits"]["hits"]
            symptoms = []
            for hit in hits:
                symptoms.append({
                            "title": hit["_source"]["title"],
                            "name": hit["_source"]["name"],
                           })

            baikes = []
            BAIKE_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = relative_baike["title"]
            res = es.search(index=es_index, body=BAIKE_QUERY, request_timeout=60)

            hits = res["hits"]["hits"]
            for hit in hits:
                if hit["_source"]["title"] != relative_baike["title"]:
                    baikes.append({
                                    "name": hit["_source"]["name"],
                                    "title": hit["_source"]["title"]
                                  })

            relative_baike["relative_symptom"] = symptoms
            relative_baike["relative_baike"] = baikes

    return relative_baike


def do_relatives(es_host, es_port, lexcion, task, tag_size=3):
    '''
    :param lexcion_tags:
    :param baike_tags:
    :param tag_size:
    :param param:
    :return:
    '''
    
    es = get_es_handler(es_host, es_port)
    mongo_conn = MongoClient(task["mongo_from"], task["mongo_from_port"])
    analysis = mongo_conn.analysis_data
    ask_good_set = analysis.ask_good
    doctors = mongo_conn.doctor_new
    doctors_set = doctors.ask_doctor_copy
    
    mongo_relatives = MongoClient(task["mongo_to"], task["mongo_to_port"])
    relatives = mongo_relatives.relatives
    relatives_set = relatives.ask_question_test

    count = 0
    start_time = 0
    split_time = 0
    es_time = 0
    mongo_save_time = 0

    for ask_res in ask_good_set.find(no_cursor_timeout=True):

        '''if ask_res["_id"] % task["modulo"] != task["task_id"]:
            continue'''

        db_title = ask_res["title"]
        db_content = ask_res["content"]
        title_id = ask_res["_id"]
        start_time = time.time()
        results = analyse_query(lexcion, db_title, db_content, tag_size)
        title_tags = results[0]
        content_tags = results[1]
        relative_baike = results[2]
        baike_key = results[3]

        if not relative_baike:
            for answer in ask_res["answer"]:
                if relative_baike:
                    break
                tags = jieba.cut(answer["answer_bqfx"]+answer["answer_zdyj"]+answer["answer_other"])
                for tag in tags:
                    if tag in G_DISEASE and not relative_baike:
                        relative_baike = G_DISEASE[tag]
                        break
                    if tag in lexicon and not baike_key:
                        baike_key = tag

        split_time += (time.time()-start_time)
        start_time = time.time() 
        relative_questions = get_question_titles(es, "jk_search", db_title, int(title_id), title_tags, content_tags, ask_good_set, doctors_set)
        es_time +=(time.time()-start_time)

        relative_baike = get_baike(es, "jk_disease", relative_baike, baike_key)
        start_time = time.time()
        relatives_set.save({
                                "_id":ask_res["_id"],
                                "title":db_title,
                                "relatives":{
                                                "questions":relative_questions,
                                                "baike":relative_baike,
                                                "yaopins":[]
                                            }
                             })

        mongo_save_time += (time.time()-start_time)
        count += 1 
        if count % 100 == 0:
            print "100 results, split tags took %s seconds, es query took %s seconds, mongo took %s seconds" % (split_time, es_time, mongo_save_time)
            split_time = 0
            es_time = 0
            mongo_save_time = 0	             



def get_lexicon(dir_entry, f):
    lexicon = {}
    lines = codecs.open(os.path.join(dir_entry, f), "r", "utf-8").readlines()
    for line in lines:
        data = line.split()
        if len(data) != 2:
            continue
        if data[0].strip() == '':
            continue
        lexicon[data[0].strip()] = int(data[1])
    return lexicon


def get_baike_dic(dir_entry, f):
    fpr = codecs.open(os.path.join(dir_entry, f), "r", "utf-8")
    for line in fpr:
       datas = line.split("$")
       G_DISEASE[datas[0].strip()]={"title":datas[0], "description":datas[1], "name":datas[2].strip(), "type":0}


def get_symptom_dic(dir_entry, f):
    fpr = codecs.open(os.path.join(dir_entry, f), "r", "utf-8")
    for line in fpr:
       datas = line.split("$")
       G_DISEASE[datas[0].strip()]={"title":datas[0], "description":datas[1], "name":datas[2].strip(), "type":1}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    args = parser.parse_args()
    the_url='http://10.5.0.10:160/proxy/index.php?type=1&task_id=ask_question_b&time=72000'
    task_id = -1
    while True:
        rep  = json.loads(requests.get(url=the_url).text)
        if rep["status"] in ["ok","null"]:
            task_id = int(rep["item_id"])
            break
        elif rep["status"] == "lock":
            time.sleep(3)
        else:
            break

    if task_id >= 0:
        mongo_conn = MongoClient("10.5.0.78", 38000)
        tasks = mongo_conn.tasks
        task_set = tasks.ask_question_b
        task_res = task_set.find_one({"_id":task_id})

        if task_res:
            dir_entry = "./corpus"
            lexicon = get_lexicon(dir_entry, "jk.big")
            get_baike_dic(dir_entry, "jk_baike.txt")
            get_symptom_dic(dir_entry, "jk_symptom.txt")
            G_WEIGHT = json.loads(codecs.open(os.path.join(dir_entry, "jk.disease.json"),"r", "utf-8").read())
            jieba.load_userdict(os.path.join(dir_entry, "jk.big"))
            jieba.analyse.set_stop_words(os.path.join(dir_entry, "stopwords.txt"))
            jieba.analyse.set_idf_path(os.path.join(dir_entry, "jk_idf.big"))
            es_host = socket.gethostbyname(socket.gethostname())
            do_relatives(es_host, 9200, lexicon, task_res["task"])

            the_url='http://10.5.0.10:160/proxy/index.php?type=2&task_id=ask_question&item_id=%s' % (task_id)
            while True:
                rep  = json.loads(requests.get(url=the_url).text)
                if rep["status"] in ["ok","err"]:
                    break
                elif rep["status"] == "retry":
                    time.sleep(1)

