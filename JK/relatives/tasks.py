# -*- coding: utf-8 -*-
__author__ = "chenzhengqiang"
__date__ = "2017-10-17"



import redis
from pymongo import MongoClient
from celery import Celery


app = Celery('jk_relative_tasks', backend='redis://localhost:6379/0', broker='redis://localhost:6379/0')
mongo_conn = MongoClient('10.5.0.3', 38000)
relatives = mongo_conn.relatives


@app.task
def insert_into_mongo(relative, ID):
    mongo_set_key = "relatives_%s" % (ID)
    relatives_new_set = relatives[mongo_set_key]
    relatives_new_set.save(relative)
    print "insert successfull"
