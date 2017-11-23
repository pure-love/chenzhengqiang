# -*- coding: utf-8 -*-
# __author__ = chenzhengqiang
# __date__ = 2017-08-29
# __desc__ = anchor text function

import sys
reload(sys)
sys.setdefaultencoding('utf8')
sys.path.append("..")

from pymongo import MongoClient

if __name__ == "__main__":
    mongo_online_conn = MongoClient("10.15.0.14", 38000)
    relatives = mongo_online_conn.relatives
    ask_question_set = relatives.ask_question
    mongo_78_conn = MongoClient("10.5.0.78", 38000)
    doctors = mongo_78_conn.doctor_new
    doctors_set = doctors.ask_doctor
    analysis = mongo_78_conn.analysis_data
    ask_good_set = analysis.ask_good
 
    for ask_res in ask_question_set.find(no_cursor_timeout = True):
        for index in range(len(ask_res["questions"])):
            ask_good_res = ask_good_set.find_one({"_id":ask_res["questions"][index]["id"]})
            if not as_good_res:
                continue

            max_score = -1
            for answer in ask_good_res["answer"]:
                doctor_res = doctors_set.find_one({"_id"})
                if not doctor_res:
                    continue

                if doctor_res["doctor_score"] > max_score:
                    max_score = doctor_res["doctor_score"]
                    ask_res["questions"][index] = answer

                                  
             
  
            
