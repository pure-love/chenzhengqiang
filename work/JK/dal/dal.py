# -*- coding: utf-8 -*-
# __author__ = chenzhengqiang
# __date__ = 2017-08-25



import pymysql.cursors
from pymongo import MongoClient

def get_mysql_conn(config, db_name=None):
    '''
    :desc:get database connection for mysql
    '''
    
    try:
        connection = pymysql.connect(host=config["DB_HOST"],
                                 port=config["DB_PORT"],
                                 user=config["DB_USER"],
                                 password=config["DB_PASSWD"],
                                 db=config["DB_NAME"] if db_name is None else db_name,
                                 charset="utf8",
                                 cursorclass=pymysql.cursors.DictCursor)
    except Exception,e:
        connection = None

    return connection


def get_mongo_conn(config):
    '''
    :desc:get database connection for mysql
    '''
    try:
        mongo_conn = MongoClient(config['MONGO_HOST'], config['MONGO_PORT'])
    except:
        mongo_conn = None
    return mongo_conn

def get_data_from_mysql(db_conn, sql, vals=None):
    ''''''
    try:
        with db_conn.cursor() as cursor:
            if vals:
                cursor.execute(sql, vals)
            else:
                cursor.execute(sql)
            res = cursor.fetchall()
            cursor.close()
    except Exception,e:
        print e
        res = None

    return res, e


def insert_into_mysql(db_conn, sql, vals):

    ''''''
    ret = True
    try:
        with db_conn.cursor() as cursor:
            cursor.execute(sql, vals)
            db_conn.commit()
            cursor.close()
    except Exception,e:
        ret = False

    return ret


def update_mysql(db_conn, sql, vals):

    ''''''
    ret = True
    try:
        with db_conn.cursor() as cursor:
            cursor.execute(sql, vals)
            db_conn.commit()
            cursor.close()
    except Exception,e:
        ret = False

    return ret
