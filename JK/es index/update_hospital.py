# -*- coding: utf-8 -*-
# __author__ = chenzhengqiang
# __date__ = 2017-09-01

import sys
sys.path.append("..")

import argparse
from dal import dal as DAL
from config.basic_config import CONFIG
import codecs
import phpserialize
import xlwt
import xlrd
from es_index_mappings import *
from es_index_queries import *
from elasticsearch import Elasticsearch
from elasticsearch import helpers



def do_hospital_update(test):

    '''
    :param test: indicates test or official
    :param es_index: the specified elasticsearch index
    :param es_type: the document type of index
    :param min_id: the minimum department id
    :param max_id: the maximum department id
    :param bulks: indicates how many bulks to insert
    :return:
    '''
    test = test.upper()
    mysql_conn = DAL.get_mysql_conn(CONFIG[test])
    if not mysql_conn:
        return 
    data = xlrd.open_workbook('hospital_level.xlsx')
    tables = data.sheets()[0]
    
    for row in range(tables.nrows):
        hospital_names = tables.cell(row, 0).value
        hospital_names = hospital_names.split()
        hospital_level = tables.cell(row, 2).value

        if hospital_level.strip() == '':
            continue
 
        for name in hospital_names:
            name = name.strip()
            sql = "update k_hospital set level=%s where title=%s"
            DAL.update_mysql(mysql_conn, sql, (hospital_level, name))
            print name," updated"

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--test")

    args = parser.parse_args()
    do_hospital_update(args.test)
