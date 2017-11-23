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

from es_index_mappings import *
from es_index_queries import *
from elasticsearch import Elasticsearch
from elasticsearch import helpers


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
    es.indices.put_mapping(es_type, {'properties': BAIKE_PROPERTIES}, [es_index])


def do_es_index(es_index, es_type, test, min_id, max_id, interval):
    '''
    :return:
    '''

    es = get_es_handler(CONFIG[test]["ELASTICSEARCH_HOST"],CONFIG[test]["ELASTICSEARCH_PORT"])
    db_conn = DAL.get_mysql_conn(CONFIG[test])
    if not db_conn:
        return
    count = 0
    records = []
    times = 0
    bulks = 1000
    while True:
        if min_id >= max_id:
            break
        sql = "select id, title,description, jibing as name from k_disease where id >=%s and id <%s"
        db_res = DAL.get_data_from_mysql(db_conn, sql, (min_id, min_id+interval))
        min_id = min_id+interval

        if db_res is None:
            continue

        if len(db_res) == 0:
            continue
        
        for res in db_res:
            sql = "select zhengzhang_content from k_disease_data where id = %s"
            _db_res = DAL.get_data_from_mysql(db_conn, sql, (res["id"]))
            symptom = ""
            if _db_res and len(_db_res) > 0:
               symptom = _db_res[0]["zhengzhang_content"]
 
            record = {
                "_index": es_index,
                "_type": es_type,
                "_source": {
                    "title": res["title"],
                    "description": res["description"],
                    "name": res["name"],
                    "symptom":symptom,
                    "type":0
                }
            }
            records.append(record)
            count +=1
            if count == bulks:
                helpers.bulk(es, records)
                times +=1
                count = 0
                info = "%d---%d indices created\r\n" % (res["id"],times * bulks)
                print info 
                records = []

    if len(records) > 0:
        helpers.bulk(es, records)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--test")
    parser.add_argument("-i", "--index")
    #parser.add_argument("-q", "--query")
    parser.add_argument("-d", "--doc_type")
    parser.add_argument("-x", "--min_id")
    parser.add_argument("-m", "--max_id")
    parser.add_argument("-v", "--interval") 
    args = parser.parse_args()
    #do_es_mapping(args.index, args.doc_type, args.test.upper())
    do_es_index(args.index, args.doc_type, args.test.upper(), int(args.min_id), int(args.max_id), int(args.interval))
    #do_es_search(args.test.upper(), args.index, args.query)
