# -*- coding: utf-8 -*-
__author__ = "chenzhengqiang"
__date__ = "2017-11-08"


import sys
sys.path.append("..")

import os
import argparse
import json
import socket
import struct
import redis
import codecs
import time
from pymongo import MongoClient




def add_page(redis_conn, categories, ask_question_data, the_best_answer,ask_doctor_120_set, ask_doctor_xywy_set):
    '''

    :param redis_conn: redis_conn = redis.Redis(host="10.15.0.18", port=6300, password="dev!1512.kang", db=0)
    :param categories: parents of the category id
    :param ask_question_data:
        {
          "_id":1222333,
          "title":u"感冒了怎么办",
          "date":"2017-09-17 08:41:40",
          "answer_accept":1,
          "catid":12
          "answer_num":123
        }
    :param the_best_answer:
         {
          "id":"e1c41e09987bf163",
          "title":u"感冒了怎么办",
          "name":"王八",
          "position":"医师",
          "answer_bqfx":"",
          "answer_zdyj":"",
          "answer_other":"",
          "answer_summary":""
        }

    :param ask_doctor_120_set: the mongo set of ask_doctor_123
    :param ask_doctor_xywy_set: the mongo set of ask_doctor_xywy
    :return:
    '''

    try:
        score = time.mktime(time.strptime(ask_question_data["date"], '%Y-%m-%d %H:%M:%S'))
    except Exception,e:
        print e
        return

    for _id in categories[str(ask_question_data["catid"])]:
        zset_keys = ["%s" % (_id), "%s:%s" % (_id, ask_question_data["answer_accept"])]
        for zset_key in zset_keys:
            count = redis_conn.zcard(zset_key)
            can_insert = True
            if count >= 4000:
                the_first = redis_conn.zrange(zset_key, 0, 0, False, True)
                try:
                    if score > float(the_first[0][1]):
                        redis_conn.zremrangebyrank(zset_key, 0, 0)
                    else:
                        can_insert = False
                except:
                    can_insert = False


            if can_insert is True:
                hospital_level = ""
                hospital_name = ""
                content = the_best_answer["answer_bqfx"] + the_best_answer["answer_zdyj"] + \
                              the_best_answer["answer_other"] + the_best_answer["answer_other"]

                if the_best_answer["id"].strip() != '':
                    doctor_res = ask_doctor_120_set.find_one({"_id":the_best_answer["id"]})
                    if not doctor_res:
                        doctor_res = ask_doctor_xywy_set.find_one({"_id":the_best_answer["id"]})

                    if doctor_res:
                        hospital_level = doctor_res["hospital_level"]
                        if hospital_level.find(u"未知") != -1 or hospital_level.find(u"未评级") != -1:
                            hospital_level = ""
                        hospital_name = doctor_res["hospital"]

                page_key = {
                    "id":ask_question_data["_id"],
                    "title":ask_question_data["title"],
                    "catid":ask_question_data["catid"],
                    "answer_num":ask_question_data["answer_num"],
                    "answer_accept":ask_question_data["answer_accept"],
                    "date":ask_question_data["date"],
                    "answer": {
                                "id":the_best_answer["id"],
                                "img_name": the_best_answer["img_name"],
                                "content":content,
                                "name": the_best_answer["name"],
                                "position":the_best_answer["position"],
                                "hospital_level":hospital_level,
                                "hospital_name":hospital_name 
                              }
                   }

                redis_conn.zadd(zset_key, json.dumps(page_key, ensure_ascii=False), score)

def test_add_list_pages():

    categories = json.loads(open("./cats.json","r").read())
    redis_conn = redis.Redis(host="10.15.0.18", port=6300, password="dev!1512.kang", db=0)
    ask_question_data = {
        "_id": 1222333,
        "title": u"感冒了怎么办",
        "date": "2017-11-11 08:41:40",
        "answer_accept": 1,
        "catid": 1,
        "answer_num": 123
    }

    the_best_answer= {
        "id": "e6453df1f24367fb",
        "img_name": u"感冒了怎么办",
        "name": "王八",
        "position": "医师",
        "answer_bqfx": "你这个病得治",
        "answer_zdyj": "",
        "answer_other": "",
        "answer_summary": ""
    }
    
    mongo_conn = MongoClient("10.5.0.78", 38000)
    doctor = mongo_conn.doctor
    ask_doctor_120_set = doctor.ask_doctor_1204m
    ask_doctor_xywy_set = doctor.ask_doctor_xywy4m
    add_page(redis_conn, categories, ask_question_data, the_best_answer, ask_doctor_120_set, ask_doctor_xywy_set)


if __name__ == "__main__":
    '''parser = argparse.ArgumentParser()
    parser.add_argument("-X", "--threads")
    args = parser.parse_args()'''
    test_add_list_pages()
