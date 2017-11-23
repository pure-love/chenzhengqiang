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


def do_es_mapping(es_index, es_type, test):
    '''

    :param es:
    :param es_index:
    :param es_type:
    :return:
    '''

    es = get_es_handler(CONFIG[test]["ELASTICSEARCH_HOST"], CONFIG[test]["ELASTICSEARCH_PORT"])
    es.indices.delete(index=es_index, ignore=[400, 404])
    es.indices.create(index=es_index, ignore=[400, 404])
    es.indices.put_mapping(es_type, {'properties': ASK_INDEX_PROPERTIES, 'settings': ES_SETTINGS}, [es_index])


def do_es_index(test, es_index, es_type, bulks):
    '''

    :param test: indicates test or official
    :param es_index: the specified elasticsearch index
    :param es_type: the document type of index
    :param min_id: the minimum department id
    :param max_id: the maximum department id
    :param bulks: indicates how many bulks to insert
    :return:
    '''
    es = get_es_handler(CONFIG[test]["ELASTICSEARCH_HOST"],CONFIG[test]["ELASTICSEARCH_PORT"])
    fw_log = open("./%s_%s.log" % (es_index, es_type), "w")
    #mongo_conn = DAL.get_mongo_conn(CONFIG[test])
    mongo_conn = MongoClient("10.5.0.19", 38000)

    param = {"analysis_data_xywy_8":"ask_xywy4m_1",
             "analysis_data_xywy_7":"ask_xywy4m_1",
             "analysis_data_120_8":"ask_1204m_8", 
             "analysis_data_120_7":"ask_1204m_7"}
   
    records = []
    for K in param:
        db = mongo_conn[K]
        ask_set = db[param[K]]
        count = 0
        times = 0
        total = 0 
        for res in ask_set.find(no_cursor_timeout = True):
            total += 1   
            '''if total <= 5142000:
               print "old data:%s" % (total)
               continue'''
            if res["sign"] != 1:
                continue
            record = {
                    "_index": es_index,
                    "_type": es_type,
                    "_id": res["_id"],
                    "_source": {
                    "id": res["_id"],
                    "title": res["title"],
                    "content": res["content"],
                    "catid":res["catid"],
                    "date":res["date"],
                    "answer_accept":res["answer_accept"] if "answer_accept" in res else 0,
                    "type":0,
                    "answer":json.dumps(res["answer"][0]) if len(res["answer"]) > 0 else ""
                    }
                }
            records.append(record)
            count +=1
            if count == bulks:
                times +=1
                helpers.bulk(es, records)
                count = 0
                info = "%d---%d indices created\r\n" % (res["_id"],times * bulks)
                print info
                fw_log.write(info)
                records = []

    if len(records) > 0:
        helpers.bulk(es, records)


def do_es_search(test, es_index, query):
    es = Elasticsearch([{'host': CONFIG[test]["ELASTICSEARCH_HOST"],
                         'port': CONFIG[test]["ELASTICSEARCH_PORT"]}])
    # QUERY_BODY["query"]["bool"]["must"][0]["match"]["title"] = u'安全套能防止性病吗'
    # QUERY_BODY["query"]["bool"]["must"][1]["match"]["content"] = u'安全套能防止性病吗'
    # QUERY_BODY["query"]["bool"]["should"][1]["match"]["departments"]["query"] = "{%s}" % (u'内科')

    test_query["query"]["bool"]["must"][0]["multi_match"]["query"] = query
    test_query["query"]["bool"]["should"][0]["match_phrase"]["title"]["query"] = query
    test_query["query"]["bool"]["should"][1]["match_phrase"]["content"]["query"] = query
    res = es.search(index=es_index, doc_type="details", body=test_query)
    fwp = codecs.open("./results.txt","w","utf8")
    if "hits" in res:
        _items = res["hits"]["hits"]
        for _item in _items:
            # _item["_source"]["title"],_item["_source"]["content"]))
            fwp.write("%s--is took:%s--%s\r\n%s\r\n" % (_item["_source"]["id"],_item["_source"]["is_took"],
                                                        _item["_source"]["title"], _item["_source"]["content"]))
        fwp.close()
    


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--test")
    parser.add_argument("-i", "--index")
    parser.add_argument("-d", "--doc_type")
    parser.add_argument("-b", "--bulks")

    args = parser.parse_args()
    #do_es_mapping(args.index, args.doc_type, args.test.upper())
    do_es_index(args.test.upper(), args.index, args.doc_type, int(args.bulks))
    #do_es_search(args.test.upper(), args.index, args.query)
