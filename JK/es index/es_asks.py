# -*- coding: utf-8 -*-
# __author__ = chenzhengqiang
# __date__ = 2017-09-01


import argparse
from dal import dal as DAL
import codecs

from elasticsearch import Elasticsearch
from elasticsearch import helpers
from pymongo import MongoClient
import json




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


def do_es_index(test, es_index, es_type, bulks, ID):
    '''

    :param test: indicates test or official
    :param es_index: the specified elasticsearch index
    :param es_type: the document type of index
    :param min_id: the minimum department id
    :param max_id: the maximum department id
    :param bulks: indicates how many bulks to insert
    :return:
    '''
    es = get_es_handler("10.5.0.101", 9200)
    #mongo_conn = DAL.get_mongo_conn(CONFIG[test])
    mongo_conn = MongoClient("10.5.0.78", 38000)
    analysis = mongo_conn.analysis_data
    ask_set = analysis.ask_good

    doctor = mongo_conn.doctor
    doctor_120 = doctor.ask_doctor_1204m
    doctor_xywy = doctor.ask_doctor_xywy4m
  
    count = 0
    records = []
    times = 0
    total = 0
 
    for res in ask_set.find(no_cursor_timeout = True):
        if res["_id"] % 4 != ID:
            continue
        
        score = 0
        answer={}
        the_best_answer = ""
        if len(res["answer"]) > 0:
            the_best_answer = res["answer"][0]
            score = float(res["answer"][0]["score"])
            for _answer in res["answer"]:
                if float(_answer["score"]) > score:
                    score = float(_answer["score"])
                    the_best_answer = _answer
   
            doctor_res = doctor_120.find_one({"_id":the_best_answer["id"]})
            if not doctor_res:
                doctor_res = doctor_xywy.find_one({"_id":the_best_answer["id"]})

            content = ""
            hospital_level = ""

            if doctor_res:
                hospital_level = doctor_res["hospital_level"]
                if hospital_level.find(u'未知') != -1 or hospital_level.find(u'未评级') != -1:
                    hospital_level = ""
  
            content = the_best_answer["answer_bqfx"].strip()
            if not content:
                content = the_best_answer["answer_zdyj"].strip()
            if not content:
                content = the_best_answer["answer_other"].strip()

            answer={"content":content, "name":the_best_answer["name"], "position":the_best_answer["position"], "img_name":the_best_answer["img_name"], "hospital_level": hospital_level}            

        record = {
                     "_index": es_index,
                     "_type": es_type,
                     "_id": res["_id"],
                     "_source": {
                     "id": res["_id"],
                     "title": res["title"],
                     "content": res["content"],
                     "answer_num":res["ans_num"],
                     "answer_accept":res["answer_accept"] if "answer_accept" in res else 0,
                     "score":score,
                     "date":res["date"],
                     "answer":json.dumps(answer, ensure_ascii=False),  
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
    parser.add_argument("-I", "--ID")

    args = parser.parse_args()
    #do_es_mapping(args.index, args.doc_type, args.test.upper())
    do_es_index(args.test.upper(), args.index, args.doc_type, int(args.bulks), int(args.ID))
    #do_es_search(args.test.upper(), args.index, args.query)
