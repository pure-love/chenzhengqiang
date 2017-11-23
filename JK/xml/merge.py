#-*-coding:utf-8-*-
import sys
reload(sys)
sys.setdefaultencoding('utf-8')
import os
import argparse
import codecs
import time
from pymongo import MongoClient


if __name__ == "__main__":
    # <question><![CDATA[如何保护牙齿预防龋齿]]></question>
    mongo_conn = MongoClient("10.5.0.8",38000)
    xml = mongo_conn.xml
    xml_set = xml.key_question

    count = 0

    fw_key_question = codecs.open("./key_question.txt","r","utf-8")
    fw = codecs.open("invalid.txt","w","utf-8")
    
    for line in fw_key_question:
        data = line.split("###")
        if len(data) != 2:
            fw.write("%s\n" % (line))
            fw.flush() 
            continue

        count += 1
        xml_set.save({"_id":count, "key":data[0].strip(), "question":data[1].strip()})

