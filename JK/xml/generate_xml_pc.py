# -*- coding: utf-8 -*-
__author__ = "chenzhengqiang"
__date__ = "2017-10-17"


import sys
sys.path.append("..")
import sys
reload(sys)
sys.setdefaultencoding('utf8')

import os
import argparse
import json
import socket
import struct
import redis
import codecs
from pymongo import MongoClient
from multiprocessing import Pool
import time
import threading
import threadpool
import copy
import random
from threading import current_thread
import time
from config.basic_config import CONFIG
from xml.dom.minidom import Document
from elasticsearch import Elasticsearch
from datetime import datetime
import re
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
                        "minimum_should_match":"65%"
                    }
                }]
            }
    },
    "_source":["title", "id", "answers"],
    "sort": { "_score":{ "order": "desc" },"score":{"order":"desc"},"answers":{"order":"desc"}},
    "from":0,
    "size":1
}



def gen_xml(es, es_index, ask_set, pid):
    
    sequence = 0
    xml = '<?xml version="1.0" encoding="utf-8"?>\n<DOCUMENT>\n'
    items = []
    times = 0
    mongo_conn = MongoClient("10.5.0.8", 38000)
    xml_db = mongo_conn.xml
    key_question_set = xml_db.key_question
    dr = re.compile(r'<[^>]+>',re.S)
 
    for _id in xrange(1, 1333699):
        if _id % 4 != pid:
            continue

        key_question_res = key_question_set.find_one({"_id":_id})
        if not key_question_res:
            continue
  
        QUESTION_QUERY["query"]["bool"]["must"][0]["multi_match"]["query"] = key_question_res["key"]
        res = es.search(index=es_index, body=QUESTION_QUERY, request_timeout = 60) 
        hits = res["hits"]["hits"]
        title = None
        title_id = -1
        count = 0
        answer = ""
 
        for hit in hits:
            title = hit["_source"]["title"]
            title_id = int(hit["_source"]["id"])
            count = str(hit["_source"]["answers"])

        if title is None:
            continue

        res = ask_set.find_one({"_id":title_id})
        if not res:
            continue
        try:
            date = datetime.strptime(res["date"], "%Y-%m-%d %H:%M:%S")
        except:
            continue 
        
        if len(res["answer"]) > 0:
            best_answer = res["answer"][0]
            max_score = float(res["answer"][0]["score"]) 
            for answer in res["answer"]:
                if float(answer["score"]) > max_score:
                    max_score = float(answer["score"])
                    best_answer = answer
            
            answer = best_answer["answer_bqfx"]+best_answer["answer_zdyj"]+best_answer["answer_summary"]+best_answer["answer_other"]
            answer = dr.sub('', answer)[0:500]
           
        sequence += 1
        the_key = key_question_res["key"].strip()
        the_question = key_question_res["question"].strip()

        if not the_question:
            the_question = the_key
 
        _record={
            "key":re.sub('\s','',re.sub(u'[\u3000,\x08,\xa0,\x3000,\x20]', u'', the_key.strip().replace("\r\n","").replace("\n","").replace("\t",""))),
            "title":"%s_169健康网" % (re.sub('\s','',re.sub(u'[\u3000,\x08,\xa0,\x3000,\x20]', u'',title.strip().replace("\r\n","").replace("\n","").replace("\t","")))),
            "question":re.sub('\s','',re.sub(u'[\u3000,\x08,\xa0,\x3000,\x20]', u'',the_question.strip().replace("\r\n","").replace("\n","").replace("\t",""))),
            "count":count,
            "time":"%s年%s月%s日" % (date.year, date.month, date.day),
            "url":"http://www.169kang.com/question/%s.html?from=sgoap&z=0%s_%s" % (title_id, sequence, count),
            "showurl":"http://www.169kang.com/question/%s.html" % (title_id),
            "answer":re.sub('\s','',re.sub(u'[\u3000,\x08,\xa0,\x3000,\x20]', u'',answer.strip().replace("\r\n","").replace("\n","").replace("\t",""))) 
        }

        item = "<item>\n"
        item += '<key><![CDATA[%s]]></key>\n' % _record["key"]
        item += '<display>\n'
        item += '<title><![CDATA[%s]]></title>\n' % _record["title"]
        item += '<url><![CDATA[%s]]></url>\n' % _record["url"]
        item += '<question><![CDATA[%s]]></question>\n' % _record["question"]
        item += '<count><![CDATA[%s]]></count>\n' % _record["count"]
        item += '<time><![CDATA[%s]]></time>\n' % _record["time"]
        item += '<answer><![CDATA[%s]]></answer>\n' % _record["answer"]
        item += '<showurl><![CDATA[%s]]></showurl>\n' % _record["showurl"]
        item += '</display>\n</item>\n'
        items.append(item)

        if len(items) == 2000:
            fp = codecs.open("./jk_pc_%s_%s.xml" % (pid,times), "w", "utf-8")
            fp.write("%s%s</DOCUMENT>" % (xml,"".join(items)))
            fp.close()
            items = []
            times += 1 

    if len(items) > 0:
        fp = codecs.open("./jk_pc_%s_last.xml" % (pid), "w", "utf-8")
        fp.write("%s" % (xml+"".join(items)+"</DOCUMENT>"))
        fp.close()

def do_xml_pc(test, es_index, ID):

    test = test.upper()
    if test not in ["TEST", "OFFICIAL"]:
        return

    es = Elasticsearch([{'host':CONFIG[test]["ELASTICSEARCH_HOST"], 'port':CONFIG[test]["ELASTICSEARCH_PORT"]}])
    mongo_conn = MongoClient("10.5.0.78", 38000)
    analysis = mongo_conn.analysis_data
    ask_set = analysis.ask_good
    gen_xml(es, es_index, ask_set, ID)
        


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--test")
    parser.add_argument("-e", "--es_index")
    parser.add_argument("-I", "--ID")
    args = parser.parse_args()
    do_xml_pc(args.test, args.es_index, int(args.ID))
