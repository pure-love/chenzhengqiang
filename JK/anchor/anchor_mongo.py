# -*- coding: utf-8 -*-
__author__ = "chenzhengqiang"
__date__ = "2017-08-29"
__desc__ = "anchor text function"


import sys
sys.path.append("..")

from dal import dal as DAL
from config.basic_config import CONFIG
import phpserialize
import argparse
import csv
import os
import json
import xlrd
from multiprocessing import Pool
from pymongo import MongoClient
import copy
import time

KEY_TABLES_FILE="./anchor_text.xls"


def gen_mapped_table():
    '''
    :return:anchor table dict
    '''

    data = xlrd.open_workbook(KEY_TABLES_FILE)
    table = data.sheets()[0]
    nrows = table.nrows
    anchors_table = {}
    filter_table= {}

    for i in range(1, nrows):
        the_key = table.cell(i,0).value
        the_anchor = table.cell(i,1).value

        if len(the_key) <=0:
            continue

        if the_key not in anchors_table:
            anchors_table[the_key] = [the_anchor]
        else:
            if the_anchor not in anchors_table[the_key]:
                anchors_table[the_key].append(the_anchor)

        first_word_key = the_key[0]
        if first_word_key not in filter_table:
            filter_table[first_word_key]=[the_key]
        else:
            if the_key not in filter_table[first_word_key]:
                filter_table[first_word_key].append(the_key)
    return anchors_table,filter_table


def remove_anchor(text):
    '''
    :param text:e.g: 今天你<a href="http://123.com">感冒</a>了吗，呵呵是感冒了，有人得了<a href="http://123.com">妇科</a>病
    :return:
    '''
    s_index = text.find("<a href=")
    if s_index !=-1:
        text = text[:s_index]
    return text


def add_anchor_to_text(text, anchors_table, filter_table, anchored_keys):
    '''
    :param anchors_table:
    :param filter_table:
    :return: anchored text
    '''

    anchored_texts = []
    has_been_anchored = False

    if not text or text.strip() == '':
        return text, anchored_keys

    while len(text) > 1:
        text_len = len(text)
        count = 0
        for index in range(0, len(text)):
            count += 1
            if has_been_anchored:
                has_been_anchored = False
                break

            if text[index] in filter_table:
                for anchor_key in filter_table[text[index]]:
                    if text[index:len(anchor_key) + index] == anchor_key and anchor_key not in anchored_keys:

                        if len(anchors_table[anchor_key]) > 0:
                            anchored_text = text[0:index] + '<a href="' + anchors_table[anchor_key].pop() + '">' \
                                            + anchor_key + '</a>'
                            anchored_texts.append(anchored_text)

                        if len(anchors_table[anchor_key]) == 0:
                            anchored_keys.append(anchor_key)

                        text = text[len(anchor_key) + index:]
                        has_been_anchored = True
                        break

        if count == text_len:
            break

    anchored_texts.append(text)

    if not anchored_texts:
        return text, anchored_keys
    else:
        return "".join(anchored_texts),anchored_keys


def test_add_anchor():

    tables = gen_mapped_table()
    anchors_table = tables[0]
    filter_table = tables[1]

    anchored_keys = []
    text1 = u'高血压感冒了怎么办得了高血压和妇科疾病了还有高血压的高血压'
    text2 = u'高血压感冒了怎么办得了高血压和妇科疾病了还有高血压的高血压'
    print "not anchored:",text1
    res = add_anchor_to_text(text1, anchors_table, filter_table, anchored_keys)
    print "anchored later:",res[0]
    print "not anchored:",text2
    res = add_anchor_to_text(text2,anchors_table, filter_table, res[1])
    print "anchored later:",res[0]


def do_anchor_text(ID):
    '''
    :param test: indicates TEST or OFFICIAL
    :return:
    '''
    tables = gen_mapped_table()
    anchors_table = tables[0]
    filter_table = tables[1]


    mongo_conn = MongoClient('10.5.0.78', 38000)
    db = mongo_conn.analysis_data
    ask_set = db.ask_good
    anchor_set = db.anchors

    modulo = 1000
    mongo_find_time = 0
    anchor_time = 0
    start_time = 0
    count = 0
    
    for res in ask_set.find():
         
        '''
        if res["_id"] % 16 != ID:
            continue
        '''   
        '''if anchor_set.find_one({"_id":res["_id"]}):
            print "%s found in anchors set" % (res["_id"])
            continue'''

        anchored_keys = []
        _anchors_table = copy.deepcopy(anchors_table)

        '''anchor_res = add_anchor_to_text(res["content"], _anchors_table, filter_table, anchored_keys)
        content = anchor_res[0]
        anchored_keys = anchor_res[1]'''
        
        start_time = time.time()      
        for answer in res["answer"]:
            anchor_res = add_anchor_to_text(answer["answer_bqfx"], _anchors_table, filter_table, anchored_keys)
            answer["answer_bqfx"] = anchor_res[0]
            anchored_keys = anchor_res[1]

            anchor_res  = add_anchor_to_text(answer["answer_zdyj"], _anchors_table, filter_table,  anchored_keys)
            answer["answer_zdyj"] = anchor_res[0]
            anchored_keys = anchor_res[1]
 
            anchor_res  = add_anchor_to_text(answer["answer_other"], _anchors_table, filter_table, anchored_keys)
            answer["answer_other"] = anchor_res[0]
            anchored_keys = anchor_res[1]
  
            anchor_res  = add_anchor_to_text(answer["answer_summary"], _anchors_table, filter_table, anchored_keys)
            answer["answer_summary"] = anchor_res[0]
            anchored_keys = anchor_res[1]
           
        ask_set.update({"_id":res["_id"]}, {'$set':{"content":content, "answer":res["answer"]}})
          

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-D", "--ID")
    args = parser.parse_args()
    do_anchor_text(int(args.ID))
    #test_add_anchor()
