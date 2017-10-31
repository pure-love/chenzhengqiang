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
from tasks import insert_into_mongo
from datetime import datetime
import time


BAIKE_QUERY={
 "query":
        {
            "bool": {
                "must": [{
                            "multi_match":
                                {
                                    "query": "{$ask}",
                                    "type":"phrase",
                                    "fields": ["title^100", "symptom"]
                                }
                        }]
                }
            },
    "from":0,
    "size":1
  }


RELATIVE_BAIKE_QUERY={
    "query":
        {
            "bool": {
                "must": [{
                            "multi_match":
                                {
                                    "query": "{$ask}",
                                    "fields": ["title^100","symptom"]
                                }
                        }],
                "disable_coord": "true"
                }
        },
    "_source":["id","jibing","title","description"],
    "from":0,
    "size":5
  }


ZZ_QUERY={
    "query":
        {
            "bool": {
                "must": [{
                            "multi_match":
                                {
                                    "query": "{$ask}",
                                    "fields": ["title","description"]
                                }
                        }],
                "disable_coord": "true"
                }
        },
    "_source":["id","title","zhengzhang"],
    "from":0,
    "size":5
  }


YAOPIN_QUERY={
    "query":
        {
            "bool": {
                "must": [{
                            "multi_match":
                                {
                                    "query": "{$ask}",
                                    "type":"most_fields",
                                    "fields": ["indication","suit"]
                                }
                        }],
                "disable_coord": "true"
                }
        },
    "from":0,
    "size":6
  }



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
                        "minimum_should_match":"70%"
                    }
                }]
            }
    },
    "_source":["title"],
    "from":0,
    "size":15
}


def get_es_handler(host, port):
    '''

    :param host: the elasticsearch server's host
    :param port: the elasticsearch server's port
    :return: the elasticsearch service's handler
    '''
    es = Elasticsearch([{'host':host, 'port':port}])
    return es


def get_query_tags(filter_tags, title, content, tags_size):
    '''
    :param filter_tags:
    :param title:
    :param content:
    :param tags_size:
    :return:
    '''

    title_tags = []
    content_tags = []

    if title and len(title) > 0:
        tags = jieba.cut(title, cut_all=False)
        for tag in tags:
            if tag in filter_tags and len(tag) > 0 and tag not in title_tags:
                title_tags.append(tag.strip().replace("\r\n", ""))
                if len(title_tags) >= tags_size:
                    break

    if content and len(content) > 0:
        tags = jieba.cut(content, cut_all=False)
        for tag in tags:
            if tag in filter_tags and tag not in content_tags and tag not in title_tags and len(tag) > 0:
                content_tags.append(tag)


    if len(title_tags) < tags_size:
        if title and len(title) > 0:  
            tags = jieba.analyse.extract_tags(title, topK = tags_size)
            for tag in tags:
                if tag not in title_tags and len(tag) > 0:
                    title_tags.append(tag)
                    if len(title_tags) >= tags_size:
                        break


    return title_tags, content_tags


def is_pretty_title(es_title, db_title):
    '''

    :param es_title:
    :param db_title:
    :return:
    '''

    es_title_len = len(es_title)
    db_title_len = len(db_title)

    if len(es_title) <=1:
        return False

    # bad performance
    '''if es_title.count(u"、") > 1 or es_title.count(u".") > 1 or es_title.count(u"…") > 0 or \
                    es_title.count(u"。") > 1 or es_title.count(u"’") > 1 or es_title.count(u"，") > 2 or \
                    es_title.count(u"！") > 1 or es_title.count(u"？") > 2 or es_title.count(u"~") > 0:

        return False'''

    '''last_ch = es_title[-1]
        if last_ch == u'，' or last_ch == u'.' or last_ch == u'。' or last_ch == u"、"  or last_ch == u'>' or last_ch == u'）':
        return False

    if (db_title_len < 3) and (es_title_len < (db_title_len + 2)):
        return False'''

    return True


def get_sub_titles(title):

    sub_titles = []
    sub_titles = title.split(u"，")
    if (sub_titles[0] == title) or (len(sub_titles) > 1 and sub_titles[1] == u"，"):
        sub_titles = title.split(u"？")
        if (sub_titles[0] == title) or (len(sub_titles) > 1 and sub_titles[1] == u"？"):
            sub_titles = title.split(u"、")
            if (sub_titles[0] == title) or (len(sub_titles) > 1 and sub_titles[1] == u"、"):
                sub_titles = []

    return sub_titles



def get_question_titles(es, es_index, title_id, db_title, title_tags, content_tags, ask_set):
    '''
    :param db_title:
    :return:
    '''
    import time
    titles_count = 15
    start = 0
    question_titles = []
    if title_tags:
        QUESTION_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = db_title
        start = time.time()
        res = es.search(index=es_index, body=QUESTION_QUERY, request_timeout = 60)
        hits = res["hits"]["hits"]
        for hit in hits:
            es_id = int(hit["_id"])
            if int(title_id) == int(es_id):
                continue

            es_title = hit["_source"]["title"]
            if es_title not in question_titles:
                if len(question_titles) < 5:
                    res = ask_set.find_one({"_id":es_id})
                    if not res:
                        continue
                    question_titles.append({"id":es_id,
                                        "title":hit["_source"]["title"],
                                        "content":res["content"],
                                        "answer":res["answer"][0] if len(res["answer"]) > 0 else [],
                                        "date":res["date"]})
                else:
                    question_titles.append( { "id": es_id,
                                              "title": hit["_source"]["title"]
                                            }
                                          )

                if len(question_titles) == titles_count:
                    return question_titles

        '''QUESTION_QUERY2["query"]["bool"]["must"][0]["multi_match"]["query"] = db_title
        res = es.search(index=es_index, body=QUESTION_QUERY2, request_timeout = 60)

        hits = res["hits"]["hits"]
        for hit in hits:
            es_id = int(hit["_id"])
            if title_id == int(es_id):
                continue
            es_title = hit["_source"]["title"]
            if es_title not in question_titles:
                if len(question_titles) < 5:
                    res = ask_set.find_one({"_id":es_id})
                    if not res:
                        print "%s not found, type:%s" % (es_id, type(es_id))
                        continue
                    question_titles.append({"id": es_id,
                                        "title": hit["_source"]["title"],
                                        "content": res["content"],
                                        "answer": res["answer"][0] if len(res["answer"]) > 0 else [],
                                        "date": res["date"]})
                else:
                    question_titles.append({"id": es_id,
                                            "title": hit["_source"]["title"]
                                            }
                                           )
                if len(question_titles) == titles_count:
                    return question_titles'''


    if len(title_tags) ==1:
        if len(title_tags[0]) <=1 and len(content_tags) > 0:
            title_tags.append(content_tags[0])

    if len(title_tags) == 0:
        title_tags = content_tags
        if len(content_tags) >=3:
            title_tags = content_tags[:3]

    for index in range(len(title_tags),0,-1):
        the_query  = "".join(title_tags[:index])
        QUESTION_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = the_query
        res = es.search(index=es_index, body=QUESTION_QUERY, request_timeout=60)
        hits = res["hits"]["hits"]
        for hit in hits:
            es_id = int(hit["_id"])
            if title_id == int(es_id):
                continue
            es_title = hit["_source"]["title"]
            if es_title not in question_titles:
                if len(question_titles) < 5:
                    res = ask_set.find_one({"_id":es_id})
                    if not res:
                        print "%s not found, type:%s" % (es_id, type(es_id))
                        continue
                    question_titles.append({"id": es_id,
                                        "title": hit["_source"]["title"],
                                        "content": res["content"],
                                        "answer": res["answer"][0] if len(res["answer"]) > 0 else [],
                                        "date": res["date"]})
                else:
                    question_titles.append({"id": es_id,
                                            "title": hit["_source"]["title"]
                                            }
                                           )
                if len(question_titles) == titles_count:
                    return question_titles

    return question_titles


def get_baike(es, es_index, all_tags, baike_dict):
    '''
    :param query_tags:
    :param baike_tags:
    :return:
    '''
    jk_baike = None
    for tag in all_tags:
        if tag in baike_dict:
            baike = tag
            ZZ_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = baike
            res = es.search(index="jk_zz", body=ZZ_QUERY, request_timeout=60)
            hits = res["hits"]["hits"]
            zzs = []
            for hit in hits:
                zzs.append({"id":hit["_source"]["id"],
                            "title":hit["_source"]["title"],
                            "zhengzhang":hit["_source"]["zhengzhang"]
                           })

            relative_baike = []
            RELATIVE_BAIKE_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = baike
            res = es.search(index=es_index, body=RELATIVE_BAIKE_QUERY, request_timeout=60)
            hits = res["hits"]["hits"]

            for hit in hits:
                if hit["_source"]["title"] !=baike :
                    relative_baike.append({"id": hit["_source"]["id"],
                                           "jibing":hit["_source"]["jibing"],
                                           "title": hit["_source"]["title"],
                                           })
            jk_baike = {
                "id": int(baike_dict[baike][0]),
                "title": baike,
                "description": baike_dict[baike][1],
                "jibing": baike_dict[baike][3],
                "relative_symptom": zzs,
                "relative_baike": relative_baike}
            break

    if not jk_baike:
        max_score = 0
        the_most_relative_baike = ''
        description = ''
        baike_id = ''
        baike_title = ''
        baike_url = ''
        baike_jibing = ''

        for tag in all_tags:
            BAIKE_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = tag
            res = es.search(index=es_index, body=BAIKE_QUERY, request_timeout=60)
            hits = res["hits"]["hits"]
            for hit in hits:
                if hit["_score"] > max_score:
                    max_score = hit["_score"]
                    the_most_relative_baike = hit["_source"]["title"]
                    description = hit["_source"]["description"]
                    baike_id = hit["_source"]["id"]
                    baike_title = hit["_source"]["title"]
                    baike_jibing = hit["_source"]["jibing"]
            break


        if the_most_relative_baike:
            ZZ_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = the_most_relative_baike
            res = es.search(index="jk_zz", body=ZZ_QUERY, request_timeout=60)
            hits = res["hits"]["hits"]
            zzs = []
            for hit in hits:
                zzs.append({"id": hit["_source"]["id"],
                            "title": hit["_source"]["title"],
                            "zhengzhang": hit["_source"]["zhengzhang"],
                           })

            relative_baike = []
            RELATIVE_BAIKE_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = the_most_relative_baike
            res = es.search(index=es_index, body=RELATIVE_BAIKE_QUERY, request_timeout=60)

            hits = res["hits"]["hits"]
            for hit in hits:
                if hit["_source"]["title"] != the_most_relative_baike:
                    relative_baike.append({"id": hit["_source"]["id"], 
                                           "jibing": hit["_source"]["jibing"],
                                           "title": hit["_source"]["title"]
                                          })
            jk_baike = {
                "id": baike_id,
                "title": baike_title,
                "description": description,
                "jibing": baike_jibing,
                "relative_symptom": zzs,
                "relative_baike": relative_baike
                 }

    return jk_baike

def get_yaopins(es, es_index, disease, query):
    '''
    :param es_index:
    :return:
    '''

    YAOPIN_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = disease
    # YAOPIN_QUERY["query"]["bool"]["should"][0]["match_phrase"]["indication"]["query"] = disease
    # YAOPIN_QUERY["query"]["bool"]["should"][1]["match_phrase"]["suit"]["query"] = disease

    yaopins = []
    res = es.search(index=es_index, body=YAOPIN_QUERY, request_timeout=60)
    count = 0
    hits = res["hits"]["hits"]
    for hit in hits:
        if hit["_source"]["name"] not in yaopins:
            yaopins.append({"id":hit["_source"]["id"],
                            "name":hit["_source"]["name"],
                            "title":hit["_source"]["title"],
                            "url":hit["_source"]["url"],
                            "price":hit["_source"]["price"],
                            "description":hit["_source"]["description"],
                            "thumb":hit["_source"]["thumb"]
                            }
                           )
            count += 1
            if count == 6:
                return yaopins

    YAOPIN_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = query
    #YAOPIN_QUERY["query"]["bool"]["should"][0]["match_phrase"]["indication"]["query"] = query
    #YAOPIN_QUERY["query"]["bool"]["should"][1]["match_phrase"]["suit"]["query"] = query

    yaopins = []
    try:
        res = es.search(index=es_index, body=YAOPIN_QUERY, request_timeout=60)
    except:
        print "%s timedout" (os.getpid())
        raise
    count = 0
    hits = res["hits"]["hits"]
    for hit in hits:
        if hit["_source"]["name"] not in yaopins:
            yaopins.append({"id": hit["_source"]["id"],
                            "name": hit["_source"]["name"],
                            "title": hit["_source"]["title"],
                            "url": hit["_source"]["url"],
                            "price": hit["_source"]["price"],
                            "description": hit["_source"]["description"],
                            "thumb": hit["_source"]["thumb"]
                            }
                           )
            count += 1
            if count == 6:
                return yaopins

    return yaopins



def do_relatives(test, lexcion_tags, baike_dict, tag_size, ID, remain_ids):
    '''
    :param lexcion_tags:
    :param baike_tags:
    :param tag_size:
    :param param:
    :return:
    '''
    
    test = test.upper()
    es = get_es_handler(CONFIG[test]["ELASTICSEARCH_HOST"], CONFIG[test]["ELASTICSEARCH_PORT"])

    total = 0
    mongo_conn = DAL.get_mongo_conn(CONFIG[test])
    analysis = mongo_conn.analysis_data
    ask_question_set = analysis.ask_good
       
    mongo_ask_all = MongoClient("10.5.0.16", 38000)
    ask_question_all = mongo_ask_all.ask_question_all
    ask_all_set = ask_question_all.ask
   
    
    mongo_relatives = MongoClient("10.5.0.88", 38000)
    relatives = mongo_relatives["relatives_new"]
    mongo_set_key = "relatives_%s" % (ID)
    relatives_set = relatives[mongo_set_key]

    '''fp = open("%s_%s.log" % (mongo_db,ID), "a+")
    try:
        the_last_catid = int(fp.readlines()[-1])
    except:
        the_last_catid = 0

    start = 0
    end = max_id

    if the_last_catid > min_id:
        start = the_last_catid
    else:
        start = min_id'''

    #start = min_id
    #end = max_id
    
    modulo = 100
    start_time = 0
    count = 0
    mongo_search_elapsed = 0
    extract_tags_elapsed = 0
    es_ask_elapsed = 0
    es_baike_elapsed = 0
    records = []   
    
    for _id in remain_ids:
        
        if _id % 4 != ID:
            continue

        #start_time = time.time()
        res = ask_question_set.find_one({"_id":_id})
        if not res:
            continue

        #mongo_search_elapsed += time.time() - start_time

        '''if res["sign"] != 1:
            continue'''
        
        count += 1
        db_title = res["title"]
        db_content = res["content"]
        
        #start_time = time.time()
        tags = get_query_tags(lexcion_tags, db_title, db_content, tag_size)
        extract_tags_elapsed += time.time() - start_time          
        
        title_tags = tags[0]
        content_tags = tags[1]
        
        #start_time = time.time() 
        related_questions = get_question_titles(es, "jk_search", res["_id"], db_title, title_tags, content_tags, ask_all_set)
        es_ask_elapsed += time.time() - start_time

        all_tags = title_tags + content_tags

        #start_time = time.time() 
        related_baike = get_baike(es, "jk_baike", all_tags, baike_dict)
        #es_baike_elapsed = time.time() - start_time
        
        relatives_set.save({
                                    "_id":res["_id"],
                                    "title":db_title,
                                    "relatives":{"questions":related_questions,
                                                 "baike":related_baike,
                                                 "yaopins":[]
                                                }
                             })

        '''if count % modulo == 0:
            #print "%s results:mongo search elapsed %s seconds, extract tags elapsed %s seconds, question search elapsed %s seconds, baike search elapsed %s seconds" % (count, mongo_search_elapsed, extract_tags_elapsed, es_ask_elapsed, es_baike_elapsed)
            count = 0
            es_baike_elapsed = 0
            es_ask_elapsed = 0
            extract_tags_elapsed = 0
            mongo_search_elapsed = 0
            relatives_set.insert_many(records)
            records = []''' 

        # insert relative asynchronously
        '''insert_into_mongo.delay({
                                    "_id":res["_id"],
                                    "title":db_title,
                                    "relatives":{"questions":related_questions,
                                                 "baike":related_baike,
                                                 "yaopins":[]
                                                }
                                }, ID)'''
        '''total += 1 
        if total % modulo == 0:
            if modulo < 100:
                modulo *= 5
            else:
               modulo += 100
            print "%s results elapsed %s seconds" % (total, time.time() -start_time)'''


def get_lexicon(dir_entry, f):
    lexicon = {}
    fpr = codecs.open(os.path.join(dir_entry, f),"r", "utf-8")
    for line in fpr:
        data = line.split("\r\n")
        if len(data) > 0 and data[0] not in lexicon:
            lexicon[data[0].strip()] = 0     
    return lexicon

def get_baike_dic(dir_entry, f):
    baike = {}
    fpr = codecs.open(os.path.join(dir_entry, f), "r", "utf-8")
    for line in fpr:
       datas = line.split("$")
       if datas[1] not in baike:
           baike[datas[1].strip()]=[datas[0],datas[2],datas[3],datas[4].replace("\r\n","")]
    return baike

def make_params(min_id, max_id, processes):
    '''
    :param min_id:
    :param max_id:
    :param processes:
    :return:
    '''
    params = []
    count = (max_id-min_id) / processes
    start = min_id
    for index in range(processes):
        params.append({"start":start, "stop":start+count})
        start+=count
    return params

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--test")
    #parser.add_argument("-m", "--min_id")
    #parser.add_argument("-M", "--max_id")
    parser.add_argument("-g", "--tags")
    parser.add_argument("-i", "--ID")
    #parser.add_argument("-d", "--mongo_db")
    #parser.add_argument("-s", "--mongo_set")
    
    #parser.add_argument("-p", "--processes")

    args = parser.parse_args()

    dir_entry = "/home/backer/work/relatives/corpus"
    lexicon = get_lexicon(dir_entry, "jk_lexicon.txt")
    baikes = get_baike_dic(dir_entry, "k_disease.txt")
    remain_ids = []
    fr = open("./lost_ids.txt", "r")
    for line in fr:
        remain_ids.append(int(line))

    jieba.load_userdict(os.path.join(dir_entry, "jk_lexicon.txt"))
    jieba.analyse.set_stop_words(os.path.join(dir_entry, "stopwords.txt"))
    jieba.analyse.set_idf_path(os.path.join(dir_entry, "jk_idf.big"))

    #params = make_params(int(args.min_id), int(args.max_id), int(args.processes))
    '''pool = Pool()
    for param in params:
        pool.apply_async(do_recommend, args=(args.test, lexicon, baikes, int(args.tags), param, ))
        pool.apply_async(do_recommend, args=(args.test, lexicon, baikes, int(args.tags), param, ))
    pool.close()
    pool.join()'''
    do_relatives(args.test, lexicon, baikes, int(args.tags), int(args.ID), remain_ids)
