# -*- coding: utf-8 -*-
# __author__ = chenzhengqiang
# __date__ = 2017-08-29
# __desc__ = anchor text function


import sys
sys.path.append("..")

from dal import dal as DAL
from config.basic_config import CONFIG
import phpserialize
import argparse
import csv
import json
import xlrd
import multiprocessing
import os
import copy

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


def remove_anchor_text(filter_table, text):
    '''
    :param anchors_table:
    :param filter_table:
    :return: anchored text
    '''

    anchored_texts = []
    anchor_keys = []
    has_been_anchored = False

    if not text or text.strip() == '':
        return text

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
                    if text[index:len(anchor_key) + index] == anchor_key and anchor_key not in anchor_keys:
                        if text[len(anchor_key) + index:len(anchor_key) + index + 4] == '</a>':
                            anchor_removed_text = remove_anchor(text[:index + len(anchor_key)])
                            text = text[index + len(anchor_key) + 4:]
                            has_been_anchored = True
                            anchored_texts.append(anchor_removed_text)
                            anchored_texts.append(anchor_key)
                            break

        if count == text_len:
            break

    anchored_texts.append(text)

    if not anchored_texts:
        return text
    else:
        return "".join(anchored_texts)


def test_add_anchor():
    
    tables = gen_mapped_table()
    anchors_table = tables[0]
    filter_table = tables[1]
    
    anchored_keys = []
    text1 = u'感冒了怎么办得了高血压和妇科疾病了还有高血压的高血压'
    text2 = u'感冒了怎么办得了高血压和妇科疾病了还有高血压的高血压'
    print "not anchored:",text1
    res = add_anchor_to_text(text1, anchors_table, filter_table, anchored_keys)
    print "anchored later:",res[0]
    print "not anchored:",text2
    res = add_anchor_to_text(text2,anchors_table, filter_table, res[1])
    print "anchored later:",res[0]


def do_anchor_text(test, interval):
    '''
    :param test: indicates TEST or OFFICIAL
    :return:
    '''

    if test not in ["TEST", "OFFICIAL"]:
        return

    tables = gen_mapped_table()
    anchors_table = tables[0]
    filter_table = tables[1]
    
    
    db_conn = DAL.get_db_conn(CONFIG[test])
    _min = interval["min"]
    _max = interval["max"]
    print interval
    distance = interval["distance"]
    fw_error = open("./anchor_log_%s.txt" % (os.getpid()),"w")
    fw_error.write("%s\n" % (str(interval)))
    while True:
        if _min >= _max:
            fw_error.write("min large equal than max\n")
            fw_error.flush()
            break
        sql = "select id, content, answer from ask_question_data where id >=%s and id <%s"
        db_res = DAL.get_data_from_db(db_conn, sql,(_min,_min+distance))
        _min = _min+distance
        if db_res is None:
            fw_error.write("db error occurred\n")
            fw_error.flush()
            break
        if len(db_res) == 0:
            continue
         
        for res in db_res:
            anchored_keys = []
            _anchors_table = copy.deepcopy(anchors_table)         
            anchor_res= add_anchor_to_text(res["content"], _anchors_table, filter_table, anchored_keys)
            content = anchor_res[0]
            anchored_keys = anchor_res[1]

            try: 
                answers = phpserialize.loads(res['answer'].encode('utf-8'))
            except:
                continue

            for k in answers:
                if answers[k]["content"]:
                    answer_content = answers[k]["content"].decode('utf-8')
                    anchor_res= add_anchor_to_text(answer_content, _anchors_table, filter_table, anchored_keys)
                    answers[k]["content"] = anchor_res[0]
                    anchored_keys = anchor_res[1]
            try:  
                answers = phpserialize.dumps(answers)
            except:
                continue 
            sql = "update ask_question_data set content=%s, answer=%s where id=%s"
            DAL.update_db(db_conn, sql, (content, answers, res["id"]))
            fw_error.write("%s\r\n" % (res["id"]))
            fw_error.flush()


def do_anchor_remove(test, interval):
    '''
    :param test: indicates TEST or OFFICIAL
    :return:
    '''

    if test not in ["TEST", "OFFICIAL"]:
        return

    tables = gen_mapped_table()
    anchors_table = tables[0]
    filter_table = tables[1]

    # print remove_anchor_text(filter_table, u'今天你<a href="http://123.com">感冒</a>了吗，
    # 呵呵是感冒了，有人得了<a href="http://123.com">妇科</a>病<a href="">why</a>')

    db_conn = DAL.get_db_conn(CONFIG[test])
    _min = interval["min"]
    _max = interval["max"]
    distance = interval["distance"]
    fw_error = open("./remove.txt", "w")
    total = 0
    while True:
        if _min >= _max:
            print "all done."
            break
        sql = "select id, content, answer from ask_question_data where id >=%s and id <=%s"
        #db_res = DAL.get_data_from_db(db_conn, sql)
        db_res = DAL.get_data_from_db(db_conn, sql, (_min, _min + distance))
        _min = _min + distance
        if db_res is None:
            print "db error"
            break
        if len(db_res) == 0:
            continue

        for res in db_res:
            content = remove_anchor_text(filter_table, res['content'])
            try:
                answers = phpserialize.loads(res['answer'].encode('utf-8'))
            except:
                continue
            
            total +=1
            for k in answers:
                if answers[k]["content"]:
                    answer_content = answers[k]["content"].decode('utf-8')
                    answers[k]["content"]= remove_anchor_text( filter_table, answer_content)


            answers = phpserialize.dumps(answers)
            sql = "update ask_question_data set content=%s, answer=%s where id=%s"
            DAL.update_db(db_conn, sql, (content, answers, res["id"]))
            fw_error.write("%s\r\n" % (res["id"]))
            fw_error.flush()
            print "%d records updated" % (total)
        


def make_intervals(min_id, max_id, size, distance):
    intervals = []
    #_min = 118255410
    _min = min_id
    interval = (max_id - min_id)/size
    for i in range(size):
        _max = _min+interval
        intervals.append({"min":_min,"max":_max,"distance":distance})
        _min = _max
    return intervals


if __name__ == "__main__":
    '''parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--test")
    parser.add_argument("-m", "--min_id")
    parser.add_argument("-M", "--max_id")
    parser.add_argument("-s", "--size")
    parser.add_argument("-d", "--distance")
    args = parser.parse_args()
    intervals = make_intervals(int(args.min_id),int(args.max_id),int(args.size),int(args.distance))
    for interval in intervals:
        p = multiprocessing.Process(target=do_anchor_text, args=(args.test.upper(), interval))
        p.start()'''
    test_add_anchor()

