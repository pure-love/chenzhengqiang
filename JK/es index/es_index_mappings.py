# -*- coding: utf-8 -*-
# __author__ = chenzhengqiang
# __date__ = 2017-09-13



ES_SETTINGS={
              "similarity":{
                  "default":{
                      "type":"jakes"
                  }
              }
    }

ASK_INDEX_PROPERTIES = {
    "id":{"index":"not_analyzed", "type":"integer"},
    "title":{"type":"text","analyzer":"ik_max_word","search_analyzer":"ik_max_word"},
    "content":{"type":"text","analyzer":"ik_max_word","search_analyzer":"ik_max_word"},
    "url":{"type":"text", "index":"not_analyzed"},
    "is_took":{"type":"integer", "index":"not_analyzed"},
    "inputtime":{"type":"text", "index":"not_analyzed"}
}



BAIKE_PROPERTIES = {
    "id":{"index":"not_analyzed", "type":"integer"},
    "title":{"type":"string","analyzer":"ik_max_word","search_analyzer":"ik_max_word"},
    "description":{"type":"string","analyzer":"ik_max_word","search_analyzer":"ik_max_word"},
    "url":{"type":"string", "index":"not_analyzed"},
    "jibing":{"type":"string", "index":"not_analyzed"},
    "symptom":{"type":"string","analyzer":"ik_max_word","search_analyzer":"ik_max_word"}
}


YAOPIN_PROPERTIES = {
    "id":{"index":"not_analyzed", "type":"integer"},
    "title":{"type":"string","analyzer":"ik_max_word","search_analyzer":"ik_max_word"},
    "description":{"type":"string","analyzer":"ik_max_word","search_analyzer":"ik_max_word"},
    "url":{"type":"string", "index":"not_analyzed"},
    "zhengzhang":{"type":"string","analyzer":"ik_max_word","search_analyzer":"ik_max_word"}
}


HOSPITAL_INDEX_PROPERTY = {
    "hospital_id":{"index":"not_analyzed","type":"integer"},
    "hospital_area_code":{"index":"not_analyzed","type":"string"},
    "pcode":{"index":"not_analyzed","type":"string"},
    "doctors":{
        "properties":{
            "name":{"index":"not_analyzed","type":"string"},
            "position":{"index":"not_analyzed","type":"string"},
            "advantage":{"type":"string","analyzer": "ik_smart","search_analyzer": "ik_smart",
                         "term_vector": "with_positions_offsets"},
        }
    },

    "departments":{
        "properties":{
            "ksname":{"type":"string","analyzer": "ik_smart","search_analyzer": "ik_smart",
                      "term_vector": "with_positions_offsets"},
            "catname":{"index":"not_analyzed","type":"string"}
        }
    }
}




