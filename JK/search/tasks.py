# -*- coding: utf-8 -*-
__author__ = "chenzhengqiang"
__date__ = "2017-10-17"


from elasticsearch import Elasticsearch
from celery import Celery
import jieba
import os
import json
import codecs


app = Celery("search", broker="amqp://guest:guest@localhost:5672")
app.conf.CELERY_RESULT_BACKEND = "amqp"
dir_entry = "./corpus"
jieba.load_userdict(os.path.join(dir_entry, "jk.big"))
suffix=[u'怎么办',u'症状',u'药',u'治疗',u'预防',u'原因',u'治愈',u'难受']
disease = json.loads(codecs.open(os.path.join(dir_entry, "jk.disease.json"),"r", "utf-8").read())
lexicon = json.loads(codecs.open(os.path.join(dir_entry, "jk.lexicon.json"),"r", "utf-8").read())
es_conn ={'host': "10.15.0.27", 'port': 9200}

@app.task
def search(query,from_,size_):
    ask_query = {
        "query": {
            "bool": {
                "must": [
                    {"multi_match": {"query": "", "fields": ["title^50", "content"], "minimum_should_match": "80%"}}],
                "should": [{"match_phrase": {"title": {"query": "", "slop": 2}}}]},
        },
        "_source": ["id", "title", "content", "answer_accept", "date", "answer", "answer_num"],
        "highlight": {"pre_tags": ["<em class=\"c_color\">"], "post_tags": ["</em>"],
                      "fields": {"title": {}, "content": {}}}
    }

    ask_query["query"]["bool"]["must"][0]["multi_match"]["query"] = query
    ask_query["query"]["bool"]["should"][0]["match_phrase"]["title"]["query"] = query
    ask_query["from"] = int(from_)
    ask_query["size"] = int(size_)

    terms = []
    weight = -1
    recommend = []
    tags = jieba.lcut(query, cut_all=True)

    for tag in tags:
        if tag in lexicon:
            if len(terms) < 2:
                terms.append(tag)
                if lexicon[tag] > weight:
                    weight = lexicon[tag]

        if tag in disease and not recommend:
            for word in suffix:
                if word not in tags:
                    recommend.append("%s%s" % (tag, word))

    if len(terms) > 0:
        ask_query["query"]["bool"]["should"].append({"match": {"title": {"query": "".join(terms), "boost": weight}}})

    ES = Elasticsearch([es_conn])
    es_res = ES.search(index="jk_ask", body=ask_query, sort="answer_accept:desc,_score:desc,answer_num:desc")
    return (es_res["hits"], recommend)

