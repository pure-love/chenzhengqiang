# -*- coding: utf-8 -*-
__author__ = "chenzhengqiang"
__date__ = "2017-10-09"


import json
import redis
from celery import Celery
import sys



app = Celery('jk_listpages_tasks', backend='redis://localhost:6379/0', broker='redis://localhost:6379/0')
redis_conn = redis.Redis(host="10.15.0.18", port=6300, password="dev!1512.kang",db=0)

count = 0
@app.task
def add_page(zset_key, res, score):
    count = redis_conn.zcard(zset_key)
    content =""
    res = redis_conn.zrange(zset_key, 0, 0, False, True)

    if score > res[0][1]:
        if count >= 4000:
            redis_conn.zremrangebyrank(zset_key, 0, count-4000)

        if len(res["answer"]) > 0 :
            if res["answer"][0]["answer_bqfx"].strip() != '':
                content = res["answer"][0]["answer_bqfx"]

            if not content:
                if res["answer"][0]["answer_zdyj"].strip() != '':
                    content = res["answer"][0]["answer_zdyj"]

            if not content:
                if res["answer"][0]["answer_other"].strip() != '':
                    content = res["answer"][0]["answer_other"] 
    
    
        page_key = {"id": res["_id"], "title": res["title"], "catid": res["catid"],
                "answer_num": res["ans_num"] if "ans_num" in res else 0, 
                "answer_accept": res["answer_accept"] if "answer_accept" in res else 0,
                "date": res["date"] if "date" in res else "",
                "answer": {"img_name": res["answer"][0]["img_name"]  if len(res["answer"]) > 0 else "", "content":content }
               }
       
      
        redis_conn.zadd(zset_key, json.dumps(page_key, ensure_ascii=False), score)
        print "%s add success:%s" % (zset_key, count)
