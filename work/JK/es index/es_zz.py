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



test_query={
"query":
            {
                "bool": {
                    "must": [{
                            "multi_match":
                                {
                                    "query": "{$ask}",
                                    "type":"best_fields",
                                    "fields": ["title^5","content^2"],
                                    "minimum_should_match": "90%"
                                }
                            }],
                                "should":[
                                                        {"match_phrase":{"title":{"query":"{$ask}"}}},
                                                        {"match_phrase":{"content":{"query":"{$ask}"}}}
                                        ],
                                        "disable_coord": "true"
                }
            },
         "sort": [{"is_took":{"order":"desc"}}, {"_score":{"order":"desc"}}],
         "_source":["id","title","content","is_took","inputtime"],
         "highlight": {
                "pre_tags": [ "&ltem&gt" ],
                "post_tags": [ "&lt/em&gt" ],
                "fields": {
                          "title": {},
                          "content": {}
                }
        }

}

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


def do_es_index(es_index, es_type, test, min_id, max_id, interval):
    '''
    :return:
    '''

    es = get_es_handler(CONFIG[test]["ELASTICSEARCH_HOST"],CONFIG[test]["ELASTICSEARCH_PORT"])
    db_conn = DAL.get_db_conn(CONFIG[test])
    fw_log = open("./%s_%s.log" % (es_index,es_type), "w")
    count = 0

    records = []
    times = 0
    bulks = 1000
    while True:
        if min_id >= max_id:
            break
        sql = "select id,title,description, zhengzhang from k_zhengzhang where id >=%s and id <%s"
        db_res = DAL.get_data_from_db(db_conn, sql, (min_id, min_id+interval))
        min_id = min_id+interval

        if len(db_res) == 0:
            continue

        for res in db_res:
            record = {
                "_index": es_index,
                "_type": es_type,
                "_id": res["id"],
                "_source": {
                    "id": res["id"],
                    "title": res["title"],
                    "description": res["description"],
                    "zhengzhang": res["zhengzhang"]
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
            fwp.write("%s--is took:%s--%s\r\n%s\r\n" % (_item["_source"]["id"],_item["_source"]["is_took"],_item["_source"]["title"], _item["_source"]["content"]))
        fwp.close()
    


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
