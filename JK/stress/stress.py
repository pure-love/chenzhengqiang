# -*- coding: utf-8 -*-
__author__ = "chenzhengqiang"
__date__ = "2017-10-17"


import sys
sys.path.append("..")

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


G_MUTEX_LOCK = threading.Lock()


def do_stress(param):
    '''
    :param category_relationship: ip address
    :return:
    '''

    mongo_conn = MongoClient('10.5.0.53', 38000)
    jk_ask = mongo_conn.analysis_data
    ask_set = jk_ask.ask_1204m
    start = 0
    count = 0
    modulo = 100
    thread_name = current_thread().getName()

    total_elapsed_in = 0
    total_elapsed_not_in = 0
    count_not_in = 0

    while True:
        _id = random.randint(param["min_id"], param["max_id"])
        start = time.time()
        if ask_set.find_one({"_id":_id}): 
            count += 1
            total_elapsed_in += time.time() - start

            if count % modulo == 0:
                count = 0
                G_MUTEX_LOCK.acquire()
                print("\033[1;33;40m%s:\033[0m  \033[1;31;40m%s\033[0m results elapsed \033[1;31;40m%s\033[0m seconds \033[1;33;40m%s\033[0m results not found elapsed \033[1;31;40m%s\033[0m seconds"
                      % (thread_name, modulo, total_elapsed_in, count_not_in, total_elapsed_not_in))
                count_not_in = 0
                total_elapsed_in = 0
                G_MUTEX_LOCK.release()


        else:
            total_elapsed_not_in += time.time()-start
            count_not_in += 1

                


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-X", "--threads")
    parser.add_argument("-m", "--min_id")
    parser.add_argument("-M", "--max_id")
    args = parser.parse_args()

    param = {"min_id":int(args.min_id), "max_id":int(args.max_id)}
    params = []
    for i in range(int(args.threads)):
        params.append(param)

    pool = threadpool.ThreadPool(int(args.threads))

    requests = threadpool.makeRequests(do_stress, params)
    [pool.putRequest(req) for req in requests]
    pool.wait()
