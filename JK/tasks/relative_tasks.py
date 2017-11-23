# -*- coding: utf-8 -*-
__author__ = "chenzhengqiang"
__date__ = "2017-11-7"


import argparse
import redis
from pymongo import MongoClient
import threadpool
import random
import threading
import requests
import json

G_MUTEX_LOCK = threading.Lock()


def do_relative_tasks(start_id, end_id, tasks, collection):
    redis_conn = redis.Redis(host='10.5.0.90', port=6300, password="123123", db = 12)
    mongo_conn = MongoClient("10.5.0.8", 38000)
    db_tasks = mongo_conn.tasks
    task_set = db_tasks[collection]
    set_key = "%s:no_dispense" % (collection)

    for task_id in xrange(tasks):
        task_set.save({"_id":task_id,"task":{"start_id":start_id, "end_id":end_id, "modulo":tasks, "task_id":task_id}})
        redis_conn.zadd(set_key, str(task_id), task_id)
        print "%s add cuccess" % (set_key)



def test_tasks(param):
    set_key = ""
    tp = param["flag"]
    flag = tp
    collection = param["mongo_collection"]
    if flag == 0:
        set_key = "%s" % (collection)
        tp = 2
    elif flag == 1:
        set_key = "%s" % (collection)
        tp = 2
    else:
        set_key = "%s" % (collection)
        tp = 2

    G_MUTEX_LOCK.acquire()
    the_url='http://10.5.0.10:160/proxy/index.php?type=2&task_id=relative_ask&item_id'
    print the_url
    print json.loads(requests.get(url=the_url).text)
    G_MUTEX_LOCK.release()
    


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--start_id")
    parser.add_argument("-e", "--end_id")
    parser.add_argument("-t", "--tasks")
    parser.add_argument("-c", "--mongo_collection")
    #parser.add_argument("-t", "--threads")

    args = parser.parse_args()
    do_relative_tasks(int(args.start_id), int(args.end_id), int(args.tasks), args.mongo_collection)

    params = []
    '''for i in range(int(args.threads)):
        param = {"mongo_collection":args.mongo_collection, "flag":random.randint(0,2), "item_id":random.randint(0,100)}
        params.append(param)

    pool = threadpool.ThreadPool(int(args.threads))
    reqs = threadpool.makeRequests(test_tasks, params)
    [pool.putRequest(req) for req in reqs]
    pool.wait()'''
