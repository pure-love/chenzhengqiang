# -*- coding: utf-8 -*-
# __author__ = chenzhengqiang
# __date__ = 2017-08-24


import sys
reload(sys)
sys.setdefaultencoding('utf-8')
sys.path.append("..")

from dal import dal as DAL
from config.basic_config import CONFIG

import argparse
import csv
from elasticsearch import Elasticsearch
import json


KEY_TABLES_FILE="./keys_table.csv"


QUERY_BODY={
    "query": {
        "bool": {
            "must": [
                { "match": { "title":""}},
                { "match": { "content":""}}
                ]
        }
    },
    "_source":["url","inputtime","title","content","is_took"],
    "from":0,
    "size":20
}


def get_id_from_url(url):
    '''
    :args : get id from url
    '''
    seps = url.split("/")
    try:
        id= int(seps[len(seps)-1].split(".")[0])
    except:
        id = -1
    return id



def do_aggreation(referred_pages, db_conn):
    '''

    :param referred_pages: aggreation data from elasticsearch
    :return: None
    '''

    for page in referred_pages:
        ask_id = page["id"]
        ask_title = page["title"]
        ask_content = page["content"]
        inputtime = page["inputtime"]
        sql = "select answer from ask_question_data where id=%s"
        ask_answers  = DAL.get_data_from_db(db_conn, sql, (ask_id))
        if not ask_answers:
            print "Error:database error occurred when do select"
            continue

        related_pages=[]
        for p in referred_pages:
            if ask_id == p["id"]:
                continue

            sql = "select answer from ask_question_data where id=%s"
            related_answers = DAL.get_data_from_db(db_conn, sql, (p["id"]))
            if not related_answers:
                print "Error:database error when do select"
                continue

            related_page={"related_id":p["id"], "related_answers":related_answers[0]["answer"],
                          "title":p["title"], "content":p["content"], "is_took":p["is_took"],
                          "url":p["url"], "inputtime":p["inputtime"]}
            related_pages.append(related_page)

        related_pages = json.dumps(related_pages, ensure_ascii=False)
        sql = 'insert into ask_pages_aggreation(ask_id, ask_title, ask_content, ' \
              'ask_answers, inputtime, related_pages) values(%s,%s,%s, %s, %s, %s)'
        DAL.insert_into_db(db_conn, sql, (ask_id, ask_title, ask_content,
                                          ask_answers[0]["answer"], inputtime, related_pages))



def main(test, pages):
    '''
    @args : test indicates TEST or OFFICIAL
    '''

    if test not in ["TEST","OFFICIAL"]:
        return

    # get db connection according to the config
    db_conn = DAL.get_db_conn(CONFIG[test])
    # connect the elasticsearch server
    es = Elasticsearch([{'host':CONFIG[test]["ELASTICSEARCH_HOST"],
                         'port': CONFIG[test]["ELASTICSEARCH_PORT"]}])

    csv_fp = open(KEY_TABLES_FILE, "r")
    all_words_reader = csv.reader(csv_fp)
    titles = []

    # read the key words line by line
    log_fp = open("./aggreation_log.txt", "w")
    for item in all_words_reader:
        if all_words_reader.line_num == 1:
            continue
        log_fp.write("%s\r\n" % (all_words_reader.line_num))
        log_fp.flush()
        title =item[0].decode("gbk")
        if title in titles:
            continue

        titles.append(title)
        QUERY_BODY["query"]["bool"]["must"][0]["match"]["title"]= title
        QUERY_BODY["query"]["bool"]["must"][1]["match"]["content"]= title

        related_pages = []
        res = es.search(index="searchindex", body=QUERY_BODY)
        if "hits" in res:
            _items = res["hits"]["hits"]


            for _item in _items:
                # _item["_source"]["title"],_item["_source"]["content"]))
                ID = get_id_from_url(_item["_source"]["url"])
                if ID != -1:
                    sql = "select answer from ask_question_data where id=%s"
                    related_answer = DAL.get_data_from_db(db_conn, sql, (ID))[0]["answer"]
                    related_pages.append({
                        "ask_id":ID,
                        "ask_url":_item["_source"]["url"],
                        "ask_title":_item["_source"]["title"],
                        "ask_content":_item["_source"]["content"],
                        "ask_answers":related_answer,
                        "ask_inputtime":_item["_source"]["inputtime"]
                        }
                    )


            related_pages = json.dumps(related_pages, ensure_ascii=False)

            sql = 'insert into ask_seo_titles(title) values(%s)'
            DAL.insert_into_db(db_conn, sql, (title))
            sql = 'select id from ask_seo_titles where title=%s'
            title_id = DAL.get_data_from_db(db_conn, sql, (title))[0]["id"]
            sql = 'insert into ask_pages_aggreation(seo_title_id, seo_title, related_pages) values(%s, %s, %s)'
            DAL.insert_into_db(db_conn, sql, (title_id,title, related_pages))

            # do_aggreation(referred_pages, db_conn)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-t","--test")
    parser.add_argument("-p", "--pages")
    args = parser.parse_args()
    if args.test:
        main(args.test.upper(), int(args.pages))
